#ifndef TASKQC_H

#define TASKQC_H


// Headers Needed
//
//
//  Expose Functions Here

#include <netinet/in.h>
#endif

// Implementations
// Internal structs
// etc.


#ifdef TASKQC_IMPLEMENTATION

#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

typedef struct {
    uint32_t length;
    char *    data;
} task_msg;


typedef struct {
    uint32_t capacity;
    uint32_t current;
    task_msg* queue;
} task_queue;


typedef struct {
    int socket;
    int max_connections;
    struct sockaddr_in* taskqc_addr;
} taskqc_socket;

taskqc_socket* new_taskqc_socket(int port, const char* ip){
    int taskqc_sock = socket(PF_INET,SOCK_STREAM,0);
    struct sockaddr_in* taskqc_addr = (struct sockaddr_in*)calloc(1,sizeof(struct sockaddr_in));
    taskqc_addr->sin_family = AF_INET;
    taskqc_addr->sin_port = htons(port);
    taskqc_addr->sin_addr.s_addr = inet_addr(ip);
    memset(taskqc_addr->sin_zero,0,sizeof(taskqc_addr->sin_zero));
    taskqc_socket* sock = (taskqc_socket*)calloc(1,sizeof(taskqc_socket));
    sock->socket = taskqc_sock;
    sock->taskqc_addr = taskqc_addr;
    return sock;
}


void delete_taskqc_socket(taskqc_socket* socket){
    close(socket->socket);
    free(socket->taskqc_addr);
    free(socket);
    return;
}

task_queue* init_queue(uint32_t capacity);

task_queue* init_queue(uint32_t capacity){
    task_queue* tq = (task_queue*)malloc(sizeof(task_queue));
    tq->capacity = capacity;
    tq->current = 0;
    tq->queue = NULL;
    return tq;
}


int taskqc_socket_bind(taskqc_socket* socket, int max_conns){
    if (bind(socket->socket,(struct sockaddr*) socket->taskqc_addr, sizeof(struct sockaddr_in)) == -1){
        fprintf(stderr,"Error::taskqc_bind::bind::%d",errno);
        return -1;
    }
    socket->max_connections = max_conns;
    if (listen(socket->socket,socket->max_connections) == -1){
        fprintf(stderr,"Error::taskqc_bind::listen::%d",errno);
        return -1;
    }
    return 0;
}
