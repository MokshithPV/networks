#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<malloc.h>
#include<errno.h>
#include<unistd.h>

#include<sys/socket.h>
#include<sys/types.h>
#include<sys/ioctl.h>

#include<net/if.h>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<netinet/if_ether.h>
#include<netinet/udp.h>

#include<linux/if_packet.h>

#include<arpa/inet.h>
#include <netdb.h>

#define PROB 0.1

int dropMessage()
{
    if((rand() % 100) < PROB * 100)
    {
        return 1;
    }
    return 0;
}

typedef struct {
    int size;
    char query[32];
} queryString;

typedef struct {
    uint16_t id;
    uint8_t type;
    uint8_t qcount;
    queryString queries[8];
} queryPacket;

typedef struct {
    uint8_t flag;
    uint32_t ipad;
} response;

typedef struct {
    uint16_t id;
    uint8_t type;
    uint8_t rcount;
    response responses[8];
} responsePacket;

unsigned short checksum(unsigned short* buff, int _16bitword)
{
	unsigned long sum;
	for(sum=0;_16bitword>0;_16bitword--)
		sum+=htons(*(buff)++);
	do
	{
		sum = ((sum >> 16) + (sum & 0xFFFF));
	}
	while(sum & 0xFFFF0000);

	return (~sum);


	
}

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        printf("Usage: %s <interface>\n", argv[0]);
        exit(1);
    }
    int sockfd;
    sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    int sendsock;
    sendsock = sockfd;
    if(sockfd < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }
    printf("Socket created successfully\n");
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, argv[1], IFNAMSIZ-1);
    if(ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0)
    {
        perror("ioctl failed");
        close(sockfd);
        exit(1);
    }
    if(setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0)
    {
        perror("Bind to device failed");
        close(sockfd);
        exit(1);
    }
    printf("Bind to device successful\n");
    printf("Interface index is %d\n", ifr.ifr_ifindex);
    struct sockaddr_ll saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sll_family = AF_PACKET;
    saddr.sll_protocol = htons(ETH_P_ALL);
    saddr.sll_ifindex = ifr.ifr_ifindex;
    if(bind(sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
    {
        perror("Bind failed");
        close(sockfd);
        exit(1);
    }
    printf("Bind successful\n");
    struct ifreq ifr_ip;
    memset(&ifr_ip, 0, sizeof(ifr_ip));
    strncpy(ifr_ip.ifr_name, argv[1], IFNAMSIZ-1);
    if(ioctl(sendsock, SIOCGIFADDR, &ifr_ip) < 0)
    {
        perror("ioctl failed");
        close(sendsock);
        exit(1);
    }
    struct ifreq ifr_mac;
    memset(&ifr_mac, 0, sizeof(ifr_mac));
    strncpy(ifr_mac.ifr_name, argv[1], IFNAMSIZ-1);
    if(ioctl(sendsock, SIOCGIFHWADDR, &ifr_mac) < 0)
    {
        perror("ioctl failed");
        close(sendsock);
        exit(1);
    }
    char *sendbuffer;
    sendbuffer = (char *) malloc(1024);
    memset(sendbuffer, 0, 1024);
    char *recvbuffer;
    recvbuffer = (char *) malloc(1024);
    struct ethhdr *eth = (struct ethhdr *)(sendbuffer);
    struct iphdr *iph = (struct iphdr *)(sendbuffer + sizeof(struct ethhdr));
    eth->h_proto = htons(ETH_P_IP);
    eth->h_source[0] = (unsigned char)(ifr_mac.ifr_hwaddr.sa_data[0]);
    eth->h_source[1] = (unsigned char)(ifr_mac.ifr_hwaddr.sa_data[1]);
    eth->h_source[2] = (unsigned char)(ifr_mac.ifr_hwaddr.sa_data[2]);
    eth->h_source[3] = (unsigned char)(ifr_mac.ifr_hwaddr.sa_data[3]);
    eth->h_source[4] = (unsigned char)(ifr_mac.ifr_hwaddr.sa_data[4]);
    eth->h_source[5] = (unsigned char)(ifr_mac.ifr_hwaddr.sa_data[5]);
    //sscanf(argv[2], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &eth->h_dest[0], &eth->h_dest[1], &eth->h_dest[2], &eth->h_dest[3], &eth->h_dest[4], &eth->h_dest[5]);
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 16;
    //printf("Destination mac is %02x:%02x:%02x:%02x:%02x:%02x\n", eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
    iph->tot_len = 20*sizeof(char) + sizeof(responsePacket);
    iph->id = htonl(54321);
    iph->ttl = 64;
    iph->protocol = 254;
    iph->saddr = ((struct sockaddr_in *)&ifr_ip.ifr_addr)->sin_addr.s_addr;
    iph->check = 0;
    iph->check = checksum((unsigned short *)iph, sizeof(struct iphdr)/2);
    responsePacket *rp = (responsePacket *)(sendbuffer + sizeof(struct ethhdr) + sizeof(struct iphdr));
    struct sockaddr_ll dest;
    memset(&dest, 0, sizeof(dest));
    dest.sll_ifindex = ifr.ifr_ifindex;
    dest.sll_halen = ETH_ALEN;
    struct hostent *host;
    while(1)
    {
        int buflen = 0;
        memset(recvbuffer, 0, 1024);
        buflen = recvfrom(sockfd, recvbuffer, 1024, 0, NULL, NULL);
        if(buflen < 0)
        {
            perror("Recieve failed");
            close(sockfd);
            exit(1);
        }
        struct ethhdr *eth1 = (struct ethhdr *)(recvbuffer);
        struct iphdr *iph1 = (struct iphdr *)(recvbuffer + sizeof(struct ethhdr));
        //printf("Recieved %d bytes\n", buflen);
        if(iph1->protocol != 254){
            continue;
        }
        queryPacket *qp = (queryPacket *)(recvbuffer + sizeof(struct ethhdr) + sizeof(struct iphdr));
        if(qp->type != 0)
        {
            continue;
        }
        /*printf("Query recieved for id: %d\n", qp->id);
        printf("Query count: %d\n", qp->qcount);
        printf("Queries:\n");
        for(int i = 0; i < qp->qcount; i++)
        {
            printf("%s\n", qp->queries[i].query);
        }*/
        if(dropMessage())
        {
            printf("Dropped message for id: %d\n", qp->id);
            continue;
        }
        rp->id = qp->id;
        rp->type = 1;
        rp->rcount = qp->qcount;
        for(int i = 0; i < qp->qcount; i++)
        {
            host = gethostbyname(qp->queries[i].query);
            if(host == NULL){
                rp->responses[i].flag = 0;
            }
            else{
                rp->responses[i].flag = 1;
                int a,b,c,d;
                sscanf(inet_ntoa(*((struct in_addr *)host->h_addr)),"%d.%d.%d.%d",&a,&b,&c,&d);
                //printf("IP Address: %d.%d.%d.%d\n", a,b,c,d);
                rp->responses[i].ipad = htonl((a << 24) | (b << 16) | (c << 8) | d);
            }
        }
        iph->daddr = iph1->saddr;
        iph->check = 0;
        iph->check = checksum((unsigned short *)iph, sizeof(struct iphdr)/2);
        eth->h_dest[0] = eth1->h_source[0];
        eth->h_dest[1] = eth1->h_source[1];
        eth->h_dest[2] = eth1->h_source[2];
        eth->h_dest[3] = eth1->h_source[3];
        eth->h_dest[4] = eth1->h_source[4];
        eth->h_dest[5] = eth1->h_source[5];
        dest.sll_addr[0] = eth->h_dest[0];
        dest.sll_addr[1] = eth->h_dest[1];
        dest.sll_addr[2] = eth->h_dest[2];
        dest.sll_addr[3] = eth->h_dest[3];
        dest.sll_addr[4] = eth->h_dest[4];
        dest.sll_addr[5] = eth->h_dest[5];
        if(sendto(sockfd, sendbuffer, 1024, 0, (struct sockaddr *)&dest, sizeof(dest)) < 0)
        {
            perror("Send failed");
            close(sockfd);
            exit(1);
        }
        //printf("Response sent for id: %d\n", rp->id);
    }
}
