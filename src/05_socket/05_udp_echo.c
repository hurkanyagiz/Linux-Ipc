/*
 * 05_udp_echo.c — Connectionless UDP Echo (server + client)
 * Usage: ./05_udp_echo server   OR   ./05_udp_echo client
 * Build: gcc -o 05_udp_echo 05_udp_echo.c
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define PORT 9090
#define BUFSIZE 512
static void run_server(void){
    int fd=socket(AF_INET,SOCK_DGRAM,0);
    struct sockaddr_in a={.sin_family=AF_INET,.sin_addr.s_addr=INADDR_ANY,.sin_port=htons(PORT)};
    bind(fd,(struct sockaddr*)&a,sizeof(a));
    printf("[UDP Server] Port %d (Ctrl+C to stop)\n\n",PORT);
    char buf[BUFSIZE];struct sockaddr_in cl;socklen_t cl_len=sizeof(cl);
    while(1){
        ssize_t n=recvfrom(fd,buf,sizeof(buf)-1,0,(struct sockaddr*)&cl,&cl_len);
        if(n<=0)continue;buf[n]='\0';
        char ip[INET_ADDRSTRLEN];inet_ntop(AF_INET,&cl.sin_addr,ip,sizeof(ip));
        printf("[UDP Server] From %s: \"%s\"\n",ip,buf);
        sendto(fd,buf,n,0,(struct sockaddr*)&cl,cl_len);
    }
}
static void run_client(void){
    int fd=socket(AF_INET,SOCK_DGRAM,0);
    struct sockaddr_in srv={.sin_family=AF_INET,.sin_port=htons(PORT)};
    inet_pton(AF_INET,"127.0.0.1",&srv.sin_addr);
    const char*msgs[]={"Hello UDP","No handshake needed","Fast but unreliable",NULL};
    char buf[BUFSIZE];
    for(int i=0;msgs[i];i++){
        sendto(fd,msgs[i],strlen(msgs[i]),0,(struct sockaddr*)&srv,sizeof(srv));
        printf("[UDP Client] Sent: \"%s\"\n",msgs[i]);
        ssize_t n=recvfrom(fd,buf,sizeof(buf)-1,0,NULL,NULL);
        if(n>0){buf[n]='\0';printf("[UDP Client] Echo: \"%s\"\n",buf);}
        usleep(500000);
    }
    close(fd);
}
int main(int argc,char*argv[]){
    if(argc!=2||(strcmp(argv[1],"server")&&strcmp(argv[1],"client"))){
        fprintf(stderr,"Usage: %s server|client\n",argv[0]);return 1;}
    strcmp(argv[1],"server")==0?run_server():run_client();return 0;
}
