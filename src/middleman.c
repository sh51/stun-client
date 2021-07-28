#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#define PORT 81
#define MAX_BUFFER 8096

// record the first sender's address, then forward all packets to the first sender
int main() {
    int i;
	int sockfd; 
    char buffer[MAX_BUFFER];
    int connected = 0;

    socklen_t len = sizeof(struct sockaddr_in); 
	struct sockaddr_in cliaddr, chosen, servaddr;

    memset(&cliaddr, 0, sizeof(struct sockaddr_in));
    memset(&servaddr, 0, sizeof(struct sockaddr_in));
    memset(&chosen, 0, sizeof(struct sockaddr_in));

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_family = AF_INET; 
	servaddr.sin_port = htons(PORT);

    bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr));

    while(recvfrom(sockfd, (char *)buffer, MAX_BUFFER, 
				MSG_WAITALL, (struct sockaddr *)&cliaddr, 
				&len) != -1) {
        printf("1 packet received.\n");
        if (!connected) {
            printf("Target acquired.\n");
            // printf("Port: %d\n", ntohs(cliaddr.sin_port));
            chosen.sin_addr.s_addr = cliaddr.sin_addr.s_addr;
            chosen.sin_family = AF_INET; 
            chosen.sin_port = cliaddr.sin_port;
            connected = 1;
        }
        if (connected && !(cliaddr.sin_port == chosen.sin_port && cliaddr.sin_addr.s_addr == chosen.sin_addr.s_addr)) {
            printf("-> %s:%d\n", inet_ntoa(chosen.sin_addr), (chosen.sin_port));
            if (sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *)&chosen, sizeof(chosen)) < 0) {
                perror("sendto()");
                exit(3);
            } else printf("OK: one packet is forwared.\n");
        }
            
        memset(&buffer, 0, MAX_BUFFER);

    }

    return 0;
}