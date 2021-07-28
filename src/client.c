#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <netdb.h>
#include <time.h>
#include <sys/mman.h>
#include <signal.h>

// connect to the server
	// timeout three times - try a new one
	// all failed - could not connect to a STUN server
	// if error, not reachable
// succeeded, send a change-request
	// if a response arrives
		// may be reached from this addr:port
	// if it doesn't arrive after timeout
		// probably unreachable
	

#define PORT 3478
#define LISTEN_PORT 9979
#define MAXLINE 1024

typedef struct {
	unsigned short msgtype;
	unsigned short msglen;
	unsigned int magic_cookie;
	char id[12];
	unsigned short type;
	unsigned short length;
	unsigned addr;
	unsigned addr2;
} stun_header;

int main(int argc, char const *argv[]) { 
	int i;
	int sockfd; 
	struct hostent *server, *server2;
	struct sockaddr_in servaddr, stun_servaddr, cliaddr;	// cliaddr for saving responses
	struct sockaddr_in * retaddr = mmap(NULL, sizeof(struct sockaddr_in), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0); // returned address:port
	struct sockaddr_in * retaddr2;
	socklen_t len = sizeof(struct sockaddr_in); 
	struct timespec start, curr;
	int timeout = 6;
	int timeout_count = 0;
	int pid;
	char response[MAXLINE];
	void * pt;

	stun_header * sh, * tmp_sh;
	char buf[MAXLINE];

	memset(&buf, 0, sizeof(MAXLINE));
	memset(retaddr, 0, sizeof(struct sockaddr_in));
	sh = (stun_header *)buf;
	memset(sh, 0, sizeof(stun_header));

	setbuf(stdout, (char *)0);

	// h to n SHORT: 2 bytes
	sh->msgtype = htons(1);	// binding request
	sh->msglen = htons(0);
	sh->magic_cookie = htonl(0x2112a442);	// magic cookie
	memcpy(&sh->id, "I am groot", 12);	// arbitrary id
	sh->type = htons(3);	// 0x0003 - change-request
	sh->length = htons(4);	// 4-byte value
	sh->addr = htonl(6);	// change both ip and port
	
	// no response-address support
	// server = gethostbyname("stun.ideasip.com");
	// server = gethostbyname("stun.l.google.com");	// does not support change-request
	// server = gethostbyname("stun.rixtelecom.se");	// no response
	server = gethostbyname("stun.voiparound.com");
	server2 = gethostbyname("stun.voipstunt.com");	
	if (!server || !server2) {
        fprintf(stderr,"Error: server not found\n");
        exit(0);
  }
	// load first stun_servaddr
	memcpy(&stun_servaddr.sin_addr.s_addr, server->h_addr, server->h_length);
	stun_servaddr.sin_family = AF_INET; 
	stun_servaddr.sin_port = htons(PORT);
	// Creating socket file descriptor 
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
		perror("socket creation failed"); 
		exit(EXIT_FAILURE); 
	} 
	// bind the socket to a port
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_family = AF_INET; 
	servaddr.sin_port = htons(LISTEN_PORT);
	bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr));
	// try to connect to the stun_server first
	pid = fork();
	if (pid) {
		clock_gettime(CLOCK_REALTIME, &start);
		while (!timeout_count) {
			// simple request - 28 for a change-request, sizeof(*sh) for full request
			if (sendto(sockfd, sh, 20, 0, (const struct sockaddr *) &stun_servaddr, sizeof(stun_servaddr)) == -1) {
				perror("sendto");
			}
			printf("Connecting to stun server...\n");
			sleep(2);
			// parent keeps checking
			if (retaddr->sin_addr.s_addr) {
				printf("Success response collected.\n");
				kill(pid, SIGKILL);
				break;
			}

			clock_gettime(CLOCK_REALTIME, &curr);
			// printf("Elapsed time: %ld.%.9ld\n", (long)curr.tv_sec - (long)start.tv_sec, curr.tv_nsec - start.tv_nsec);
			if ((long)curr.tv_sec - (long)start.tv_sec >= timeout) timeout_count++;
				
		}
	} else {
		// child process will listen to port
		while (recvfrom(sockfd, (char *)response, MAXLINE, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len) != -1) {
			tmp_sh = (stun_header *)&response;
			// only save success response		
			if (tmp_sh->msgtype == htons(0x0101)) {
				// printf("A success response!\n");
				pt = &tmp_sh->addr2;
				retaddr->sin_addr.s_addr = *((int *)pt);
				retaddr->sin_family = AF_INET;
				retaddr->sin_port = *((short *)pt - 1);
				// printf("Addr: %x\n", ntohl(retaddr->sin_addr.s_addr));
				// printf("Port: %d\n", ntohs(retaddr->sin_port));
			}

		}
	}
	// this might happen when
			// the previous server send back error response
			// the server couldn't be reached
	if (timeout_count) {
		printf("External addr:port undetermined, switching to alternative server.\n");
		// terminate remaining child process
		kill(pid, SIGKILL);
		// try the alternative server
		// load second stun_servaddr
		memcpy(&stun_servaddr.sin_addr.s_addr, server2->h_addr, server2->h_length);
		// try again
		pid = fork();
		timeout_count = 0;
		if (pid) {
			clock_gettime(CLOCK_REALTIME, &start);
			while (!timeout_count) {
				if (sendto(sockfd, sh, 20, 0, (const struct sockaddr *) &stun_servaddr, sizeof(stun_servaddr)) == -1) {
					perror("sendto");
				}
				printf("Connecting to stun server...\n");
				sleep(2);

				// parent keeps checking
				if (retaddr->sin_addr.s_addr) {
					printf("Success response collected.\n");
					kill(pid, SIGKILL);
					break;
				}

				clock_gettime(CLOCK_REALTIME, &curr);
				// printf("Elapsed time: %ld.%.9ld\n", (long)curr.tv_sec - (long)start.tv_sec, curr.tv_nsec - start.tv_nsec);
				if ((long)curr.tv_sec - (long)start.tv_sec >= timeout) timeout_count++;
					
			}
		} else {
			// child process will listen to port
			while (recvfrom(sockfd, (char *)response, MAXLINE, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len) != -1) {
				tmp_sh = (stun_header *)&response;
				// only save success response		
				if (tmp_sh->msgtype == htons(0x0101)) {
					// printf("A success response!\n");
					pt = &tmp_sh->addr2;
					retaddr->sin_addr.s_addr = *((int *)pt);
					retaddr->sin_family = AF_INET;
					retaddr->sin_port = *((short *)pt - 1);
				}

			}
		}
	}
	
	// servers unreachable
	if (timeout_count) {
		printf("Could not connect to a STUN server.\n");
		kill(pid, SIGKILL);
		exit(1);
	}

	// got the first mapping
	printf("Finding out reachability...\n");
	retaddr2 = mmap(NULL, sizeof(struct sockaddr_in), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	memset(retaddr2, 0, sizeof(struct sockaddr_in));
	pid = fork();
	timeout_count = 0;
	if (pid) {
		clock_gettime(CLOCK_REALTIME, &start);
		while (!timeout_count) {
			// this time send a change-request
			if (sendto(sockfd, sh, 28, 0, (const struct sockaddr *) &stun_servaddr, sizeof(stun_servaddr)) == -1) {
				perror("sendto");
			}
			sleep(2);
			// parent keeps checking
			if (retaddr2->sin_addr.s_addr) {
				kill(pid, SIGKILL);
				break;
			}

			clock_gettime(CLOCK_REALTIME, &curr);
			// printf("Elapsed time: %ld.%.9ld\n", (long)curr.tv_sec - (long)start.tv_sec, curr.tv_nsec - start.tv_nsec);
			if ((long)curr.tv_sec - (long)start.tv_sec >= timeout) timeout_count++;
				
		}
	} else {
		// child process will listen to port
		while (recvfrom(sockfd, (char *)response, MAXLINE, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len) != -1) {
			tmp_sh = (stun_header *)&response;
			// only save success response		
			if (tmp_sh->msgtype == htons(0x0101)) {
				pt = &tmp_sh->addr2;
				retaddr2->sin_addr.s_addr = *((int *)pt);
				retaddr2->sin_family = AF_INET;
				retaddr2->sin_port = *((short *)pt - 1);
			}

		}
	}

	if (timeout_count) {
		kill(pid, SIGKILL);
		// likely unreachable - since a host with a different addr:port combination cannot send packets through
		printf("I am %s:%d, but probably unreachable.\n", inet_ntoa(retaddr->sin_addr), ntohs(retaddr->sin_port));
	}
	else printf("Peer may connect to %s:%d.\n", inet_ntoa(retaddr->sin_addr), ntohs(retaddr->sin_port));

	return 0;
} 
