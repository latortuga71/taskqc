#include <stdio.h>
#include <assert.h>
#include "taskqc.h"


void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc,char** argv){
    taskqc_socket* socket = taskqc_socket_init(9998,"127.0.0.1");
    taskqc_connect(socket);
    taskqc_msg msg;
    while(1){
        taskqc_recv_msg(socket->socket,&msg);
        printf("%d -> %s\n",msg.length,(char*)msg.data);
        printf("Ok Doing Work!...\n");
        sleep(5);
        printf("Work Done!\n");
    }

    taskqc_socket_delete(socket);
    return 0;
}
