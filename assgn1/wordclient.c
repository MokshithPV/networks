#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <errno.h>
#include <netdb.h>

#define MAXLINE 1024
#define PORT 8181

int main(int argc, char *argv[])
{
    if(argc != 2){
        printf("Please specify file name\n");
        exit(EXIT_FAILURE);
    }
    struct addrinfo hints,*servinfo,*temp;
    int sockfd;
    memset(&hints, 0, sizeof (hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    int rv;
    char *BUFF;
    BUFF = (char *)malloc(sizeof(char)*MAXLINE);
    sprintf(BUFF,"%d",PORT);
    if((rv = getaddrinfo(NULL,BUFF,&hints,&servinfo)) != 0){
        fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(rv));
        exit(EXIT_FAILURE);
    }
    for(temp = servinfo; temp != NULL; temp = temp->ai_next){
        if((sockfd = socket(temp->ai_family,temp->ai_socktype,temp->ai_protocol)) == -1){
            perror("socket");
            continue;
        }
        break;
    }
    if(temp == NULL){
        fprintf(stderr,"failed to bind socket\n");
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(servinfo);
    int numbytes = 0;
    //printf("%ld",strlen(argv[1]));
    while(numbytes != strlen(argv[1])){
        numbytes = sendto(sockfd,argv[1],strlen(argv[1]),0,temp->ai_addr,temp->ai_addrlen);
    }
    printf("Requested file %s from server by sending %d bytes\n",argv[1],numbytes);
    char *BUFF2;
    BUFF2 = (char *)malloc(sizeof(char)*MAXLINE);
    struct sockaddr_in theiraddr;
    int n;
    socklen_t len;
    len = sizeof(theiraddr);
    n = recvfrom(sockfd,(char *)BUFF2,MAXLINE,0,(struct sockaddr *)&theiraddr,&len);
    BUFF2[n-1] = '\0';
    char *che;
    che = (char *)malloc(sizeof(char)*MAXLINE);
    sprintf(che,"NOTFOUND %s",argv[1]);
    if(!strcmp(BUFF2,che)){
        printf("File %s not found\n",argv[1]);
        exit(0);
    }
    else if(!strcmp(BUFF2,"HELLO")){
        //printf("recieving data from server and writing into %s")
        FILE *fptr = fopen("recieved.txt","w");
        int nwor = 1;
        printf("Recieving contents of file %s from server\n",argv[1]);
        while(1){
            sprintf(che,"WORD%d",nwor);
            sendto(sockfd,(const char *)che,strlen(che),0,(const struct sockaddr *)&theiraddr,len);
            n = recvfrom(sockfd,(char *)BUFF2,MAXLINE,0,(struct sockaddr *)&theiraddr,&len);
            BUFF2[n] = '\0';
            if(!strcmp(BUFF2,"END\n")){
                printf("Recieved contents of the file %s and stored it in recieved.txt\n",argv[1]);
                break;
            }
            fprintf(fptr,"%s",BUFF2);
            printf("%d. %s",nwor,BUFF2);
            nwor++;
        }
        close(sockfd);
    }
    else{
        printf("%s\n",BUFF2);
        printf("Unknown response from server\n");
        exit(EXIT_FAILURE);
    }
}
