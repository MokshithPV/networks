#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h> 

int main()
{
    int fd = open("test.txt",O_RDONLY);
    char buf[100];
    int n;
    while((n = read(fd,buf,100)) > 0)
    {
        printf("length of buf is %d\n",n);
        printf("%s",buf);
    }
    n = read(fd,buf,100);
    printf("%s\n%d\n",buf,n);
    char buff[100] = "\n";
    printf("%d",strlen(buff));
    return 0;
}
