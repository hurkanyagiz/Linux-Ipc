/*
 * 01_tcp_server.c — Multi-Client TCP Server
 * Accepts multiple clients using fork(). Echoes messages in uppercase.
 * Build: gcc -o 01_tcp_server 01_tcp_server.c
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define PORT 8080
#define BUFSIZE 1024
static volatile sig_atomic_t running = 1;
static void on_sig(int s){(void)s;running=0;}
static void reap(int s){(void)s;while(waitpid(-1,NULL,WNOHANG)>0);}
static void handle_client(int fd,const char*ip){
    char buf[BUFSIZE];
    send(fd,"Welcome! Type messages (quit to exit).\n",39,0);
    ssize_t n;
    while((n=recv(fd,buf,sizeof(buf)-1,0))>0){
        buf[n]='\0';buf[strcspn(buf,"\r\n")]='\0';
        if(strcmp(buf,"quit")==0)break;
        printf("[Server] %s: \"%s\"\n",ip,buf);
        char resp[BUFSIZE];
        int len=snprintf(resp,sizeof(resp),"ECHO: ");
        for(int i=0;buf[i];i++)resp[len++]=toupper((unsigned char)buf[i]);
        resp[len++]='\n';resp[len]='\0';
        send(fd,resp,len,0);
    }
    close(fd);
}
int main(void){
    struct sigaction sa={.sa_handler=on_sig};sigemptyset(&sa.sa_mask);
    sigaction(SIGINT,&sa,NULL);
    sa.sa_handler=reap;sa.sa_flags=SA_RESTART;sigaction(SIGCHLD,&sa,NULL);
    signal(SIGPIPE,SIG_IGN);
    int sfd=socket(AF_INET,SOCK_STREAM,0);
    int opt=1;setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    struct sockaddr_in a={.sin_family=AF_INET,.sin_addr.s_addr=INADDR_ANY,.sin_port=htons(PORT)};
    if(bind(sfd,(struct sockaddr*)&a,sizeof(a))==-1){perror("bind");return 1;}
    listen(sfd,5);
    printf("=== TCP Server on port %d ===\nCtrl+C to stop\n\n",PORT);
    while(running){
        struct sockaddr_in cl;socklen_t cl_len=sizeof(cl);
        int cfd=accept(sfd,(struct sockaddr*)&cl,&cl_len);
        if(cfd<0)continue;
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET,&cl.sin_addr,ip,sizeof(ip));
        printf("[Server] Client connected: %s\n",ip);
        if(fork()==0){close(sfd);handle_client(cfd,ip);exit(0);}
        close(cfd);
    }
    close(sfd);return 0;
}
