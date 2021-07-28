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

#define MAX_BUFFER 8096

unsigned short csum(unsigned short *buf, int nwords) {
  unsigned long sum;
  for(sum=0; nwords>0; nwords--)
    sum += *buf++;
  sum = (sum >> 16) + (sum &0xffff);
  sum += (sum >> 16);
  return (unsigned short)(~sum);
}

// IPv4 header
// typedef struct ip_hdr {
//     unsigned char  ip_verlen;        // 4-bit IPv4 version 4-bit header length (in 32-bit words)
//     unsigned char  ip_tos;           // iphdr type of service
//     unsigned short ip_totallength;   // Total length
//     unsigned short ip_id;            // Unique identifier
//     unsigned short ip_offset;        // Fragment offset field
//     unsigned char  ip_ttl;           // Time to live
//     unsigned char  ip_protocol;      // Protocol(TCP,UDP etc)
//     unsigned short ip_checksum;      // iphdr checksum
//     unsigned int   ip_srcaddr;       // Source address
//     unsigned int   ip_destaddr;      // Source address
// } IPV4_HDR, *PIPV4_HDR;

// // Define the UDP header
// typedef struct udp_hdr{
//     unsigned short src_portno;       // Source port no.
//     unsigned short dst_portno;       // Dest. port no.
//     unsigned short udp_length;       // Udp packet length
//     unsigned short udp_checksum;     // Udp checksum (optional)
// } UDP_HDR, *PUDP_HDR;

// use this programs to fabricate udp packets
int main(int argc, char const *argv[]) {
    int i;
	int sockfd, sockfd2; 
    int opt = 1;    
    char buffer[MAX_BUFFER];
    struct ip *iphdr = (struct ip *) buffer;
    struct udphdr *udp = (struct udphdr *) (buffer + sizeof(struct ip));
    char * data = buffer + sizeof(struct ip) + sizeof(struct udphdr);
    u_int16_t src_port, dst_port;
    u_int32_t src_addr, dst_addr;
    char msg[MAX_BUFFER];
    src_addr = inet_addr(argv[1]);
    dst_addr = inet_addr(argv[3]);
    src_port = atoi(argv[2]);
    dst_port = atoi(argv[4]);
    if (argc > 5) strcpy(msg, argv[5]);
    else strcpy(msg, "");

	struct sockaddr_in sin; 

    // initialize buffer
    memset(&buffer, 0, MAX_BUFFER);
    // create a raw socket with UDP protocol
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sockfd < 0) {
        perror("socket() error");
        exit(2);
    }
    // set IP_HDRINCL
    if(setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt)) < 0) {
        perror("setsockopt() error");
        exit(2);
    }
    // // setup addr and ports
    // src_port = 19302;
    // dst_port = 6029;
    // src_addr = inet_addr("172.21.247.67");
    // dst_addr = inet_addr("140.82.1.27");
    // with well-formatted header the servaddr is no longer needed
    memcpy(data, msg, strlen(msg));
    // initialize header
    iphdr->ip_hl = 5;
    iphdr->ip_v = 4;
    iphdr->ip_tos = 0; // low delay
    iphdr->ip_len = sizeof(struct ip) + sizeof(struct udphdr) + strlen(data) + 1;
    iphdr->ip_id = htons(81);
    // iphdr->ip_off = 0;    // fragment offset
    iphdr->ip_ttl = 64; // hops
    iphdr->ip_p = IPPROTO_UDP; // UDP - 17
    // source iphdr address, can use spoofed address here
    iphdr->ip_src.s_addr = src_addr;
    iphdr->ip_dst.s_addr = dst_addr;
    // fabricate the UDP header
    udp->uh_sport = htons(src_port);
    // destination port number
    udp->uh_dport = htons(dst_port);
    udp->uh_ulen = htons(sizeof(struct udphdr));
    // udp chksum left empty
    // calculate the checksum for integrity
    iphdr->ip_sum = csum((unsigned short *)buffer,
                    sizeof(struct ip) + sizeof(struct udphdr));

    if (sendto(sockfd, buffer, iphdr->ip_len, 0,
             (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        perror("sendto()");
        exit(3);
    
    } else printf("Success: one packet is sent.\n");

    return 0;
}