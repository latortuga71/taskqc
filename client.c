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
    taskqc_socket* socket = new_taskqc_socket(9999,"127.0.0.1");
    char data[5] = "HELLO";
    taskqc_send_msg(socket, data,strlen(data));
    return 0;
}
