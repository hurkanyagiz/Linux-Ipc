/*
 * 03_unix_domain_server.c — Unix Domain Socket Server
 *
 * Unix domain sockets use a filesystem path instead of IP:port.
 * They bypass the network stack, making them faster than TCP for
 * local IPC. Used by Docker, PostgreSQL, and Nginx.
 *
 * Build: gcc -o 03_unix_domain_server 03_unix_domain_server.c
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#define SOCK_PATH "/tmp/ipc_demo_uds.sock"
#define BUFSIZE 256
int main(void){
    unlink(SOCK_PATH);
    int sfd=socket(AF_UNIX,SOCK_STREAM,0);
    struct sockaddr_un addr={.sun_family=AF_UNIX};
    strncpy(addr.sun_path,SOCK_PATH,sizeof(addr.sun_path)-1);
    bind(sfd,(struct sockaddr*)&addr,sizeof(addr));
    listen(sfd,1);
    printf("[UDS Server] Listening: %s\nRun 04_unix_domain_client\n\n",SOCK_PATH);
    int cfd=accept(sfd,NULL,NULL);
    printf("[UDS Server] Client connected.\n");
    char buf[BUFSIZE];ssize_t n;
    while((n=recv(cfd,buf,sizeof(buf)-1,0))>0){
        buf[n]='\0';printf("[UDS Server] Got: \"%s\"\n",buf);
        char rev[BUFSIZE];int len=strlen(buf);
        for(int i=0;i<len;i++)rev[i]=buf[len-1-i];rev[len]='\0';
        send(cfd,rev,len,0);
    }
    close(cfd);close(sfd);unlink(SOCK_PATH);
    printf("[UDS Server] Done.\n");return 0;
}
