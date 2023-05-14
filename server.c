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
    int result = taskqc_socket_bind(socket,10);
    assert(result == 0);
    printf("Listening!\n");
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    char s[INET6_ADDRSTRLEN];
    while(1){
        //char buffer[25];
        //taskqc_recv(socket,buffer,25,0);
        taskqc_msg msg;
        taskqc_recv_msg(socket,&msg);
        printf("Length %d Data: %s\n",msg.length,(char*)msg.data);
    }
    return 0;
}
