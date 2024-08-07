#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include<string.h>
#include<fcntl.h> 

struct node{
    char name[10];
    int port;
};

int main(int argc, char *argv[])
{
    int sockfd,newsocfd;
    int clilen;
    struct sockaddr_in cli_addr,serv_addr;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Cannot create socket\n");
		exit(0);
	}
    serv_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &serv_addr.sin_addr);
    serv_addr.sin_port = htons(atoi(argv[1]));
    if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        printf("Unable to bind local address\n");
        exit(0);
    }
    struct node users[3];
    sprintf(users[0].name,"user_1");
    sprintf(users[1].name,"user_2");
    sprintf(users[2].name,"user_3");
    users[0].port = 50000;
    users[1].port = 50001;
    users[2].port = 50002;
    for(int i=0;i<3;i++){
        if(atoi(argv[1])==users[i].port){
            printf("user %d listening at port %d\n",i+1,serv_addr.sin_port);
        }
    }
    listen(sockfd,5);
    int newsockfd1=-1,newsockfd2=-1;
    int clientsockfd=-1,liveserv = -1;
    fd_set rfds,wfds;
    char buf[300];
    while(1){
        FD_ZERO(&rfds);
        FD_SET(sockfd,&rfds);
        if(newsockfd1>0){
            FD_SET(newsockfd1,&rfds);
        }
        if(newsockfd2>0){

            FD_SET(newsockfd2,&rfds);
        }
        if(clientsockfd>0){
            FD_SET(clientsockfd,&rfds);
        }
        FD_SET(0,&rfds);
        int max = 0;
        if(max<sockfd) max = sockfd;
        if(max<newsockfd2) max = newsockfd2;
        if(max<newsockfd1) max = newsockfd1;
        if(max<clientsockfd) max = clientsockfd;
        int ret = select(max + 1,&rfds,NULL,NULL,NULL);
        if(ret == -1){
            perror("Select error:");
        }
        if(FD_ISSET(0,&rfds)){
            scanf(" %[^\n]%*c",buf);
            char c = buf[5];
            char *z = buf + 7;
            char buf2[300];
            sprintf(buf2,"user_%d: %s",atoi(argv[1])-50000 + 1,z);
            if((c-'0')==liveserv){
                send(clientsockfd,buf2,strlen(buf),0);
            }
            else{
                //printf("Closing connection with %d\n",liveserv);
                close(clientsockfd);
                clientsockfd = socket(AF_INET, SOCK_STREAM, 0);
                serv_addr.sin_port = htons(users[(c-'0')-1].port);
                if ((connect(clientsockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr))) < 0) {
		            perror("Unable to connect to server\n");
		            exit(0);
	            }
                liveserv = (c - '0');
                //printf("Connected to user %d\n",liveserv);
                send(clientsockfd,buf2,strlen(buf2),0);
            }
        }
        if(clientsockfd>0){
            if(FD_ISSET(clientsockfd,&rfds)){
                close(clientsockfd);
                clientsockfd = -1;
                liveserv = -1;
            }
        }
        if(newsockfd1>0){
            if(FD_ISSET(newsockfd1,&rfds)){
                int n = recv(newsockfd1,buf,299,0);
                if(n == 0){
                    //printf("Connection 1 closed\n");
                    close(newsockfd1);
                    newsockfd1 = -1;
                }
                else{
                buf[n] = '\0';
                printf("%s\n",buf);
                }
            }
        }
        if(newsockfd2>0){
            if(FD_ISSET(newsockfd2,&rfds)){
                int n = recv(newsockfd2,buf,299,0);
                if(n == 0){
                    //printf("Connection 2 closed\n");
                    close(newsockfd2);
                    newsockfd1 = -1;
                }
                else{
                buf[n] = '\0';
                printf("%s\n",buf);
                }
            }
        }
        if(FD_ISSET(sockfd,&rfds)){
            if(newsockfd1<0){
                clilen = sizeof(cli_addr);
                newsockfd1 = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
                //printf("Connection 1\n");
            }
            else if(newsockfd2<0){
                clilen = sizeof(cli_addr);
                newsockfd2 = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
                //printf("Connection 2\n");
            }
        }
    }
}