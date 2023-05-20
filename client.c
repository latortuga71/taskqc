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
    if (argc < 2){
        fprintf(stderr,"Need Args\n");
        return 0;
    }
    taskqc_socket* socket = taskqc_socket_init(9999,"127.0.0.1");
    taskqc_connect(socket);
    taskqc_msg* msg1 = taskqc_msg_init(strlen(argv[1]),argv[1]);
    taskqc_send_msg(socket,msg1);
    char* buffer = malloc(sizeof(char) * 1024);
    int nread = recv(socket->socket,buffer,1024,0);
    buffer[nread] = '\0';
    printf("Broker Said -> %s\n",buffer);
    taskqc_msg_delete(msg1);
    taskqc_socket_delete(socket);
    //char data[] = "BUFFERTESTER";
    //printf("Length %li\n",strlen(data));
    //taskqc_send(socket, data, strlen(data));
    /*task_queue* q = init_queue(1);
    taskqc_msg msg1 = taskqc_msg_init(strlen("AAAABBBB"),"AAAABBBB");
    taskqc_msg msg2 = taskqc_msg_init(strlen("BBBB"),"BBBB");
    taskqc_msg msg3 = taskqc_msg_init(strlen("AAAA"),"AAAA");
    taskqc_msg msg4 = taskqc_msg_init(strlen("C"),"C");
    push_queue_msg(q,&msg1);
    push_queue_msg(q,&msg2);
    push_queue_msg(q,&msg3);
    print_queue(q);
    taskqc_msg* a = pop_queue_msg(q);
    pop_queue_msg(q);
    push_queue_msg(q,&msg4);
    printf("=====\n");
    print_queue(q);
    printf("%d\n",a->length);
    */
    return 0;
}
