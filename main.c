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
    int result = taskqc_socket_bind(socket,10);
    assert(result == 0);
    printf("Listening!\n");
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    char s[INET6_ADDRSTRLEN];
    while(1){
        int new_fd = accept(socket->socket,(struct sockaddr*)&client_addr,&client_addr_size);
        if (new_fd == -1){
            perror("accept");
            continue;
        }
        inet_ntop(client_addr.ss_family,get_in_addr((struct sockaddr *)&client_addr),s, sizeof(s));
        printf("Got connections from %s\n",s);
        close(new_fd);
    }
    return 0;
}