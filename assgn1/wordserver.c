#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#define MAXLINE 1024
#define PORT 8181

int main()
{
    int sockfd;
    struct sockaddr_in servaddr,theiraddr;
    sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    memset(&servaddr,0,sizeof(servaddr));
    memset(&theiraddr,0,sizeof(theiraddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);
    if(bind(sockfd,(const struct sockaddr *)&servaddr,sizeof(servaddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("\nServer Running....\n");
    int n;
    socklen_t len;
    FILE *fptr;
    char buff[MAXLINE];
    while(1){
        len = sizeof(theiraddr);
        n = recvfrom(sockfd,(char *)buff,MAXLINE,0,(struct sockaddr *)&theiraddr,&len);
        buff[n] = '\0';
        printf("client requested content from file %s\nSearching for the file...\n",buff);
        fptr = fopen(buff,"r");
        if(fptr == NULL){
            char *che;
            che = (char *)malloc(sizeof(char)*MAXLINE);
            sprintf(che,"NOTFOUND %s",buff);
            sendto(sockfd,(const char *)che,strlen(che),0,(const struct sockaddr *)&theiraddr,len);
        }
        else{
            printf("Found file %s\n",buff);
            break;
        }
    }
    char *che;
    che = (char *)malloc(sizeof(char)*MAXLINE);
    char *Buff;
    Buff = (char *)malloc(sizeof(char)*MAXLINE);
    while(fgets(che,MAXLINE,fptr) != NULL){
        //printf("%ld",strlen(che));
        int nspace = 0;
        for(int i=0;che[i]!='\n';i++){
            if(che[i] == ' '){nspace++;
                che[i] = '\n';
            }
        }
            int j = 0;
            //printf("%d\n",nspace);
            for(int i=0;i<nspace+1;i++){
                char *tempch;
                tempch = (char *)malloc(sizeof(char)*MAXLINE);
                int k = 0;
                while(che[j] != '\n'){
                    tempch[k] = che[j];
                    j++;
                    k++;
                }
                tempch[k] = '\n';
                j++;
                sendto(sockfd,(const char *)tempch,strlen(tempch),0,(const struct sockaddr *)&theiraddr,len);
                if(strcmp(tempch,"END") && strcmp(tempch,"END\n")){
                    recvfrom(sockfd,(char *)Buff,MAXLINE,0,(struct sockaddr *)&theiraddr,&len);
                }
                else{
                    printf("Finished sending contents of the file %s\n",buff);
                }
            }
    }
    close(sockfd);
}
