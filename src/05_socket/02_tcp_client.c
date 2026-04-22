/*
 * 02_tcp_client.c — Interactive TCP Client
 * Build: gcc -o 02_tcp_client 02_tcp_client.c
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define PORT 8080
#define BUFSIZE 1024
int main(void){
    int sock=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in a={.sin_family=AF_INET,.sin_port=htons(PORT)};
    inet_pton(AF_INET,"127.0.0.1",&a.sin_addr);
    printf("[Client] Connecting to 127.0.0.1:%d...\n",PORT);
    if(connect(sock,(struct sockaddr*)&a,sizeof(a))==-1){perror("connect");return 1;}
    char buf[BUFSIZE];ssize_t n=recv(sock,buf,sizeof(buf)-1,0);
    if(n>0){buf[n]='\0';printf("[Server] %s",buf);}
    printf("[Client] Type messages ('quit' to exit):\n");
    while(1){
        printf("> ");if(!fgets(buf,sizeof(buf),stdin))break;
        buf[strcspn(buf,"\n")]='\0';if(!strlen(buf))continue;
        send(sock,buf,strlen(buf),0);if(strcmp(buf,"quit")==0)break;
        n=recv(sock,buf,sizeof(buf)-1,0);if(n<=0)break;
        buf[n]='\0';printf("[Server] %s",buf);
    }
    close(sock);return 0;
}
