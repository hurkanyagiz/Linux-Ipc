/*
 * 04_unix_domain_client.c — Unix Domain Socket Client
 * Build: gcc -o 04_unix_domain_client 04_unix_domain_client.c
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
    int sock=socket(AF_UNIX,SOCK_STREAM,0);
    struct sockaddr_un addr={.sun_family=AF_UNIX};
    strncpy(addr.sun_path,SOCK_PATH,sizeof(addr.sun_path)-1);
    if(connect(sock,(struct sockaddr*)&addr,sizeof(addr))==-1){perror("connect");return 1;}
    printf("[UDS Client] Connected to %s\n",SOCK_PATH);
    const char*msgs[]={"Hello","Linux IPC","Unix Socket",NULL};
    char buf[BUFSIZE];
    for(int i=0;msgs[i];i++){
        send(sock,msgs[i],strlen(msgs[i]),0);
        printf("[UDS Client] Sent: \"%s\"\n",msgs[i]);
        ssize_t n=recv(sock,buf,sizeof(buf)-1,0);
        if(n>0){buf[n]='\0';printf("[UDS Client] Reversed: \"%s\"\n",buf);}
        usleep(300000);
    }
    close(sock);return 0;
}
