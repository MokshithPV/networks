#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<malloc.h>
#include<errno.h>
#include<unistd.h>
#include<time.h>

#include<sys/socket.h>
#include<sys/types.h>
#include<sys/ioctl.h>
#include<sys/select.h>

#include<net/if.h>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<netinet/if_ether.h>
#include<netinet/udp.h>

#include<linux/if_packet.h>

#include<arpa/inet.h>

#define PORT 0
#define TIMEOUT 5

unsigned short checksum(unsigned short* buff, int len)
{
	unsigned long sum;
	for(sum=0;len>0;len--)
		sum+=htons(*(buff)++);
	do
	{
		sum = ((sum >> 16) + (sum & 0xFFFF));
	}
	while(sum & 0xFFFF0000);

	return (~sum);
}

int validate(char *str);

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

typedef struct queryTable{
    uint16_t id;
    int numTimeOuts;
    char queries[400];
    struct queryTable *next;
}queryTable;

int main(int argc, char *argv[])
{
    //printf("Size of queryPacket: %ld\n", ntohl(312));
    if(argc != 4){
        printf("Usage: %s <ip> <mac> <interface>\n", argv[0]);
        exit(1);
    }
    int sendsock;
    sendsock = socket(AF_PACKET, SOCK_RAW, ETH_P_ALL);
    if(sendsock < 0)
    {
        perror("Send Socket creation failed");
        exit(1);
    }
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, argv[3], IFNAMSIZ-1);
    if(ioctl(sendsock, SIOCGIFINDEX, &ifr) < 0)
    {
        perror("ioctl failed");
        close(sendsock);
        exit(1);
    }
    if(setsockopt(sendsock, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0)
    {
        perror("Bind to device failed");
        close(sendsock);
        exit(1);
    }
    printf("Bind to device successful\n");
    printf("Interface index is %d\n", ifr.ifr_ifindex);
    struct sockaddr_ll saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sll_family = AF_PACKET;
    saddr.sll_protocol = htons(ETH_P_ALL);
    saddr.sll_ifindex = ifr.ifr_ifindex;
    if(bind(sendsock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
    {
        perror("Bind failed");
        close(sendsock);
        exit(1);
    }
    printf("Bind successful\n");
    struct ifreq ifr_ip;
    memset(&ifr_ip, 0, sizeof(ifr_ip));
    strncpy(ifr_ip.ifr_name, argv[3], IFNAMSIZ-1);
    if(ioctl(sendsock, SIOCGIFADDR, &ifr_ip) < 0)
    {
        perror("ioctl failed");
        close(sendsock);
        exit(1);
    }
    struct ifreq ifr_mac;
    memset(&ifr_mac, 0, sizeof(ifr_mac));
    strncpy(ifr_mac.ifr_name, argv[3], IFNAMSIZ-1);
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
    sscanf(argv[2], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &eth->h_dest[0], &eth->h_dest[1], &eth->h_dest[2], &eth->h_dest[3], &eth->h_dest[4], &eth->h_dest[5]);
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 16;
    printf("Destination mac is %02x:%02x:%02x:%02x:%02x:%02x\n", eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
    iph->tot_len = 20*sizeof(char) + sizeof(queryPacket);
    iph->id = htonl(54321);
    iph->ttl = 64;
    iph->protocol = 254;
    iph->daddr = inet_addr(argv[1]);
    iph->saddr = ((struct sockaddr_in *)&ifr_ip.ifr_addr)->sin_addr.s_addr;
    iph->check = 0;
    iph->check = checksum((unsigned short *)iph, sizeof(struct iphdr)/2);
    queryPacket *qp = (queryPacket *)(sendbuffer + sizeof(struct ethhdr) + 20*sizeof(char));
    struct sockaddr_ll dest;
    memset(&dest, 0, sizeof(dest));
    dest.sll_ifindex = ifr.ifr_ifindex;
    dest.sll_halen = ETH_ALEN;
    dest.sll_addr[0] = eth->h_dest[0];
    dest.sll_addr[1] = eth->h_dest[1];
    dest.sll_addr[2] = eth->h_dest[2];
    dest.sll_addr[3] = eth->h_dest[3];
    dest.sll_addr[4] = eth->h_dest[4];
    dest.sll_addr[5] = eth->h_dest[5];
    int queryid = 0;
    queryTable *qt = NULL;
    queryTable *qt1 = NULL;
    queryTable *qt2 = NULL;
    printf("Ready to send\n");
    char *token;
    /*qp->id = 0;
    qp->qcount = 1;
    qp->type = 0;
    strncpy(qp->queries[0].query,"google.com",11);
    qp->queries[0].size = 11;*/
    //sendto(sendsock,sendbuffer,1024,0,(struct sockaddr *)&dest,sizeof(dest));
    int FLAG = 0;
    int currID;
    time_t lastsent = time(NULL);
    time_t current = time(NULL);
    while(1){
        if(FLAG == 1 || FLAG == 2){
            qt1 = qt;
            qt2 = qt;
            while(qt1 != NULL){
                if(qt1->numTimeOuts > 3){
                    printf("Query ID %d timed out\n", qt1->id);
                    if(qt1 == qt){
                        qt = qt->next;
                        free(qt1);
                        qt1 = qt;
                    }
                    else{
                        qt2->next = qt1->next;
                        free(qt1);
                        qt1 = qt2->next;
                    }
                }
                else{
                    if(FLAG == 2 && qt1->id != currID){
                        qt2 = qt1;
                        qt1 = qt1->next;
                        continue;
                    }
                    qp->id = qt1->id;
                    qp->type = 0;
                    char temporary[400];
                    strncpy(temporary, qt1->queries, sizeof(temporary) - 1);
                    temporary[sizeof(temporary) - 1] = '\0';
                    //printf("%s\n", qt1->queries);
                    token = strtok(temporary, " ");
                    token = strtok(NULL, " ");
                    //printf("%s\n", temporary);
                    qp->qcount = atoi(token);
                    for(int i = 0; i < qp->qcount; i++){
                        token = strtok(NULL, " ");
                        strncpy(qp->queries[i].query, token, sizeof(qp->queries[i].query) - 1);
                        qp->queries[i].query[sizeof(qp->queries[i].query) - 1] = '\0';
                        qp->queries[i].size = strlen(qp->queries[i].query);
                    }
                    //printf("Qury ln: %ld\n", sizeof(qp->queries));
                    int sendlen = sendto(sendsock, sendbuffer, sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(queryPacket), 0, (struct sockaddr *)&dest, sizeof(dest));
                    if(sendlen < 0){
                        perror("Send failed");
                        close(sendsock);
                        exit(1);
                    }
                    //printf("Sent %d bytes for id %d\n", sendlen,qp->id);
                    qt2 = qt1;
                    qt1 = qt1->next;
                }
            }
            if(FLAG == 1){
                time(&lastsent);
            }
        }
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sendsock, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        struct timeval tv;
        tv.tv_sec = TIMEOUT;
        tv.tv_usec = 0;
        FLAG = 0;
        int retval = select(sendsock+1, &readfds, NULL, NULL, &tv);
        if(retval == -1)
        {
            perror("Select failed");
            continue;
        }
        else if(retval == 0){
            qt1 = qt;
            while(qt1 != NULL){
                qt1->numTimeOuts++;
                qt1 = qt1->next;
            }
            if(qt != NULL){
                FLAG = 1;
            }
            else{
                FLAG = 0;
            }
        }
        else{
            time(&current);
            if(qt != NULL && current - lastsent > TIMEOUT)
            {
                qt1 = qt;
                while(qt1 != NULL){
                    qt1->numTimeOuts++;
                    qt1 = qt1->next;
                }
                FLAG = 1;
            }
            if(FD_ISSET(STDIN_FILENO, &readfds)){
                char input[400];
                scanf("%[^\n]%*c", input);
                if(strncmp(input, "EXIT", 4) == 0){
                    break;
                }
                //printf("Recieved: %s\n", input);
                if(validate(input) == 0){
                    printf("Invalid query\n");
                    continue;
                }
                //printf("%s\n", input);
                queryTable *qtnew = (queryTable *)malloc(sizeof(queryTable));
                qtnew->id = (queryid++)%(1<<16);
                currID = qtnew->id;
                qtnew->numTimeOuts = 0;
                strncpy(qtnew->queries, input, sizeof(qtnew->queries) - 1);
                qtnew->queries[sizeof(qtnew->queries) - 1] = '\0';
                qtnew->next = qt;
                qt = qtnew;
                if(FLAG != 1){
                    FLAG = 2;
                }
            }
            if(FD_ISSET(sendsock,&readfds)){
                memset(recvbuffer, 0, 1024);
                int recvlen = recvfrom(sendsock, recvbuffer, 1024, 0, NULL, NULL);
                //printf("Recieved %d bytes\n", recvlen);
                if(recvlen < 0){
                    perror("Recieve failed");
                    close(sendsock);
                    exit(1);
                }
                //check if its the correct packet
                struct ethhdr *eth = (struct ethhdr *)(recvbuffer);
                //check if network layer is IP
                if(ntohs(eth->h_proto) != ETH_P_IP){
                    continue;
                }
                struct iphdr *iph = (struct iphdr *)(recvbuffer + sizeof(struct ethhdr));
                if(iph->protocol != 254){
                    continue;
                }
                //printf("Recieved %d bytes\n", recvlen);
                if(iph->saddr != inet_addr(argv[1])){
                    continue;
                }
                responsePacket *rp = (responsePacket *)(recvbuffer + sizeof(struct ethhdr) + 20*sizeof(char));
                if(rp->type != 1){
                    continue;
                }
                FLAG = 1;
                qt1 = qt;
                qt2 = qt;
                while(qt1 != NULL){
                    if(qt1->id == rp->id){
                        printf("\n\t\t\tQuery ID %d\n", rp->id);
                        //printf("\tTotal query strings: %d\n", rp->rcount);
                        printf("--------------------------------------------------------------\n");
                        printf("|S.NO|%-32s|%-22s|\n", "Query", "IP Address");
                        printf("--------------------------------------------------------------\n");
                        token = strtok(qt1->queries, " ");
                        token = strtok(NULL, " ");
                        int numQueries = atoi(token);
                        for(int i = 0; i < numQueries; i++){
                            printf("|%-4d|", i+1);
                            token = strtok(NULL, " ");
                            printf("%-32s", token);
                            if(rp->responses[i].flag == 0){
                                printf("|%-22s|\n","NO IP ADDRESS FOUND");
                            }
                            else{
                                rp->responses[i].ipad = ntohl(rp->responses[i].ipad);
                                char ip[16];
                                sprintf(ip, "%d.%d.%d.%d", (rp->responses[i].ipad & 0xFF000000) >> 24, (rp->responses[i].ipad & 0x00FF0000) >> 16, (rp->responses[i].ipad & 0x0000FF00) >> 8, (rp->responses[i].ipad & 0x000000FF));
                                printf("|%-22s|\n", ip);
                            }
                            
                        }
                        printf("--------------------------------------------------------------\n");
                        if(qt1 == qt){
                            qt = qt->next;
                            free(qt1);
                            qt1 = qt;
                        }
                        else{
                            qt2->next = qt1->next;
                            free(qt1);
                            qt1 = qt2->next;
                        }
                    }
                    else{
                        qt2 = qt1;
                        qt1 = qt1->next;
                    }
                }
            }
        }
    }
    free(sendbuffer);
    close(sendsock);
    return 0;
}

int validate(char str[])
{
    char temporary[400];
    strncpy(temporary, str, sizeof(temporary) - 1);
    char *token;
    int numQueries = 0;
    token = strtok(temporary, " ");
    if(strncmp(token, "getIP", 5) != 0){
        return 0;
    }
    token = strtok(NULL, " ");
    if(token == NULL){
        return 0;
    }
    for(int i = 0; i < strlen(token); i++){
        if(token[i] < '0' || token[i] > '9'){
            return 0;
        }
    }
    numQueries = atoi(token);
    if(numQueries > 8 || numQueries < 1){
        return 0;
    }
    for(int i = 0; i < numQueries; i++){
        token = strtok(NULL, " ");
        if(token == NULL){
            return 0;
        }
        if(strlen(token) < 3 || strlen(token) > 31){
            return 0;
        }
        for(int j = 0; j < strlen(token); j++){
            if((token[j] >= 'a' && token[j] <= 'z') || (token[j] >= 'A' && token[j] <= 'Z') || (token[j] >= '0' && token[j] <= '9')){
                continue;
            }
            else if(token[j] == '-'){
                if(j == 0 || j == strlen(token) - 1){
                    return 0;
                }
                if(token[j+1] == '-'){
                    return 0;
                }
            }
            else if(token[j] == '.'){
                continue;
            }
            else{
                return 0;
            }
        }
    }
    token = strtok(NULL, " ");
    if(token != NULL){
        return 0;
    }
    return 1;
}

