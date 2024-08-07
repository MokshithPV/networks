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

int main()
{
    int sockfd,newsocfd;
    int clilen;
    struct sockaddr_in cli_addr,serv_addr;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Cannot create socket\n");
		exit(0);
	}
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(20000);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        printf("Unable to bind local address\n");
        exit(0);
    }
    printf("server listening at port %d\n",serv_addr.sin_port);
    listen(sockfd,5);
    char buf[100];
    char buf1[3];
    char filn[96];
    char filn1[100];
    char ipad[INET_ADDRSTRLEN];
    int n;
    while(1)
    {
        clilen = sizeof(cli_addr);
        newsocfd = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
        int re[101];
        for(int i=0;i<=100;i++){
            re[i] = 0;
        }
        //printf("%d\n",newsocfd);
        if(fork()==0)
        {
            inet_ntop(AF_INET,&cli_addr.sin_addr,ipad,INET_ADDRSTRLEN);
            printf("Established connection with client at ip %s port %d\n",ipad,cli_addr.sin_port);
            close(sockfd);
            int ENCR;
            sprintf(filn,"%s.%u.txt",ipad,cli_addr.sin_port);
            int fd = open(filn,O_WRONLY | O_CREAT, 0644);
            if(fd<0){
                printf("error\n");
                exit(0);
            }
            n = recv(newsocfd,buf1,3,0);
            ENCR = atoi(buf1);
            printf("recieved k from client\n");
            while(1){
                n = recv(newsocfd,buf,100,0);
                re[n]++;
                printf("recieved %d bytes\n",n);
                if(n<100 || buf[n-1]=='\0') {
                    n = write(fd,buf,n-1);
                    break;
                }
                n = write(fd,buf,n);
            }
            printf("Recieved complete file:\n");
            for(int i=0;i<=100;i++){
                if(re[i]>0){
                    printf("\t%d chunks of %d bytes\n",re[i],i);
                }
            }
            close(fd);
            sprintf(filn1,"%s.enc",filn);
            int fd1 = open(filn1,O_WRONLY | O_CREAT, 0644);
            fd = open(filn,O_RDONLY);
            char temp[1];
            while(read(fd,temp,1)>0){
                if((temp[0] - 'a' ) > -1 && ('z' - temp[0])>-1){
                    temp[0] = 'a'+(temp[0] - 'a' + ENCR)%26;
                }
                else if((temp[0] - 'A' ) > -1 && ('Z' - temp[0])>-1){
                    temp[0] = 'A'+(temp[0] - 'A' + ENCR)%26;
                }
                write(fd1,temp,1);
            }
            close(fd);
            close(fd1);
            printf("file %s encrypted\n",filn);
            fd1 = open(filn1,O_RDONLY);
            int sendingb = 100;
            int se[sendingb+1];
            for(int i=0;i<=sendingb;i++){
                se[i] = 0;
            }
            while((n = read(fd1,buf,sendingb))>0){
                send(newsocfd,buf,n,0);
                se[n]++;
            }
            printf("Sent complete encrypted file:\n");
            for(int i=0;i<=sendingb;i++){
                if(se[i]>0){
                    printf("\t%d chunks of %d bytes\n",se[i],i);
                }
            }
            close(fd1);
            close(newsocfd);
		    exit(0);
        }
        close(newsocfd);
    }
    return 0;
}

