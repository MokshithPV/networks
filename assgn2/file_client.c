#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include<netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<fcntl.h> 

int main()
{
    char req;
    req = 'y';
    while(req == 'y' || req == 'Y'){
    char fnin[96];
    printf("Enter Name of the file:\n");
    fflush(stdin);
    fflush(stdout);
    scanf(" %[^\n]%*c",fnin);
    //getchar();
    fflush(stdout);
    fflush(stdin);
    int fd;
    while((fd = open(fnin,O_RDONLY))<0){
        printf("Enter Name of the file:");
        fflush(stdin);
        scanf("%[^\n]%*c",fnin);
    }
    printf("file %s found\n",fnin);
    printf("Enter k:");
    int ENCR;
    scanf("%d",&ENCR);
    ENCR = ENCR%26;
    struct addrinfo hints,*servinfo,*temp;
    int sockfd;
    memset(&hints, 0, sizeof (hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int rv;
    if((rv = getaddrinfo(NULL,"20000",&hints,&servinfo)) != 0){
        fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(rv));
        exit(EXIT_FAILURE);
    }
    for(temp = servinfo; temp != NULL; temp = temp->ai_next){
        if((sockfd = socket(temp->ai_family,temp->ai_socktype,temp->ai_protocol)) == -1){
            perror("socket");
            continue;
        }
        if (connect(sockfd, temp->ai_addr, temp->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
        continue;
        }
        break;
    }
    if(temp == NULL){
        fprintf(stderr,"failed to bind socket\n");
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(servinfo);
    char buf1[100];
    char ipad[INET_ADDRSTRLEN];
    inet_ntop(AF_INET,(struct sockaddr_in *)temp->ai_addr,ipad,INET_ADDRSTRLEN);
    printf("Established connection with server at ip %s port %d\n",ipad,((struct sockaddr_in *)temp->ai_addr)->sin_port);
    int n;
    if(0<=ENCR && ENCR <= 9) sprintf(buf1,"0%d",ENCR);
    else sprintf(buf1,"%d",ENCR);
    send(sockfd,buf1,strlen(buf1)+1,0);
    printf("sent k to server\n");
    int sendb = 30;
    int se[sendb+1];
    for(int i=0;i<=sendb;i++){
        se[i] = 0;
    }
    char buf[110];
    while((n = read(fd,buf,sendb)) > 0)
    {
        buf[n] = '\0';
        sprintf(buf,"%s\r\n",buf);
        send(sockfd,buf,strlen(buf),0);
        se[n]++;
    }
    char BUF[100] = "";
    send(sockfd,BUF,1,0);
    printf("Sent complete file:\n");
    for(int i=0;i<=sendb;i++){
        if(se[i]>0){
            printf("\t%d chunks of %d bytes\n",se[i],i);
        }
    }
    close(fd);
    char filn[100];
    sprintf(filn,"%s.enc",fnin);
    fd = open(filn,O_WRONLY | O_CREAT, 0644);
    int re[101];
    for(int i=0;i<=100;i++){
        re[i] = 0;
    }
    while((n = recv(sockfd,BUF,100,0))>0){
        re[n]++;
        write(fd,BUF,n);
    }
    close(fd);
    printf("file %s recieved:\n",filn);
    for(int i=0;i<=100;i++){
        if(re[i]>0){
            printf("\t%d chunks of %d bytes\n",re[i],i);
        }
    }
    close(sockfd);
    printf("Do you want to continue? (y/n):");
    fflush(stdin);
    fflush(stdout);
    scanf(" %c",&req);
    }
    return 0;
}
