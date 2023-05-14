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
    taskqc_socket* socket = taskqc_socket_init(9999,"127.0.0.1");
    //char data[] = "BUFFERTESTER";
    //printf("Length %li\n",strlen(data));
    //taskqc_send(socket, data, strlen(data));
    taskqc_msg msg = taskqc_msg_init(strlen("AAAABBBB"),"AAAABBBB");
    taskqc_send_msg(socket,&msg);
    free(msg.data);
    taskqc_socket_delete(socket);
    return 0;
}
