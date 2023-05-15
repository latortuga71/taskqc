#ifndef TASKQC_H

#define TASKQC_H


// Headers Needed
//
//
//  Expose Functions Here

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

#define MAX_FRAME_SIZE 1024
#define MAX_BYTE_ARRAY_SIZE 1024

typedef struct {
    uint32_t length;
    void* data;
} taskqc_msg;


typedef struct {
    uint32_t capacity;
    uint32_t current;
    taskqc_msg** front;
    taskqc_msg** rear;
} task_queue;


typedef struct {
    int socket;
    int max_connections;
    struct sockaddr_in* taskqc_addr;
} taskqc_socket;

taskqc_msg taskqc_msg_init(uint32_t length, void* data){
    taskqc_msg msg =  {};
    msg.length = length;
    msg.data = calloc(length,sizeof(char));
    strncpy(msg.data,data,length);
    return msg;
}

void taskqc_msg_delete(taskqc_msg msg){
    free(msg.data);
}

taskqc_socket* taskqc_socket_init(int port, const char* ip){
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


void taskqc_socket_delete(taskqc_socket* socket){
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
    tq->front = (taskqc_msg**)malloc(sizeof(task_queue*) * capacity);
    tq->rear = tq->front;
    return tq;
}

void print_queue(task_queue* queue){
    for (int x = 0; x < queue->current; x++){
        printf("[%d] -> %d\n",x, queue->front[x]->length);
        if (queue->front[x] == NULL){
            return;
        }
    }
}

taskqc_msg* pop_queue_msg(task_queue* queue){
    // maybe in future if too many empty spots we can resize?
    if (queue->current == 0)
        return NULL;
    taskqc_msg* popped = queue->front[0];
    for (int x = 0; x < queue->current; x++){
        queue->front[x] = queue->front[x+1];
    }
    // the one at the end is empty now we can just move current down 1
    queue->current--;
    // set the rear again;
    queue->rear = &queue->front[queue->current];
    return popped;
}

void push_queue_msg(task_queue* queue,taskqc_msg* msg){
    // if queue is empty add the front and rear point to the same place
    if (queue->capacity - 1 == queue->current){
        // resize queue
        void* tmp = realloc(queue->front, sizeof(task_queue*) * (queue->capacity * 2));
        if (tmp == NULL){
            fprintf(stderr,"error::push_queue_msg::realloc::OOM");
            exit(-1);
        }
        queue->front = (taskqc_msg**)tmp;
        queue->capacity = queue->capacity * 2;
    }
    // add msg to queue
    queue->front[queue->current] = msg;
    queue->rear = &queue->front[queue->current];
    queue->current++;
}



int taskqc_socket_bind(taskqc_socket* socket, int max_conns){
    if (bind(socket->socket,(struct sockaddr*) socket->taskqc_addr, sizeof(struct sockaddr_in)) == -1){
        fprintf(stderr,"error::taskqc_bind::bind::%d",errno);
        return -1;
    }
    socket->max_connections = max_conns;
    if (listen(socket->socket,socket->max_connections) == -1){
        fprintf(stderr,"error::taskqc_bind::listen::%d",errno);
        return -1;
    }
    return 0;
};

int taskqc_recv_msg(taskqc_socket* socket, taskqc_msg* msg){
        struct sockaddr_storage client_addr;
        socklen_t client_addr_size = sizeof(client_addr);
        int new_fd = accept(socket->socket,(struct sockaddr*)&client_addr,&client_addr_size);
        if (new_fd == -1){
            fprintf(stderr,"error::taskqc_recv_msg::accept::%d",errno);
            return errno;
        }
        // read first byte to get length of data.
        uint32_t frame_length = 0;
        ssize_t nread = recv(new_fd,&frame_length,sizeof(frame_length),0);
        if (nread < 1){
            fprintf(stderr,"error::taskqc_recv_msg::recv::%d",errno);
            return errno;
        }
        if (frame_length > MAX_FRAME_SIZE){
            fprintf(stderr,"error::taskqc_recv_msg::recv::MAX_FRAME_SIZE_EXCEEDED");
            return -1;
        }
        if (frame_length < 1){
            fprintf(stderr,"error::taskqc_recv_msg::recv::MIN_FRAME_SIZE_NOT_MET");
            return -1;
        }
        char* buffer = (char*)malloc(sizeof(char) * frame_length);
        nread = recv(new_fd,buffer,frame_length,0);
        if (nread < 1){
            fprintf(stderr,"error::taskqc_recv_msg::recv::%d",errno);
            return errno;
        }
        msg->length = frame_length;
        msg->data = calloc(frame_length,sizeof(char));
        memcpy(msg->data,buffer,frame_length);
        free(buffer);
        return 0;
}

int taskqc_send_msg(taskqc_socket* socket, taskqc_msg* msg){
    socklen_t addr_size = sizeof(struct sockaddr_in);
    if (connect(socket->socket,(struct sockaddr*)socket->taskqc_addr,addr_size) == -1){
        fprintf(stderr,"error::taskqc_send_msg::connect::%d",errno);
        return errno;
    }
    // send the whole message as a frame
    char* frame = (char*)malloc(msg->length + sizeof(msg->length));
    memcpy(frame ,&msg->length,4);
    memcpy(frame + 4, msg->data,msg->length);
    // write frame
    ssize_t nwrote = send(socket->socket, frame, sizeof(msg->length) + msg->length, 0);
    if (nwrote == -1){
        fprintf(stderr,"error::taskqc_send_msg::send::%d",errno);
        return errno;
    }
    printf("SENT FRAME! %li\n",nwrote);
    free(frame);
    return 0;
}
int taskqc_send(taskqc_socket* socket, void* buffer, uint32_t buffer_sz){
    socklen_t addr_size = sizeof(struct sockaddr_in);
    if (connect(socket->socket,(struct sockaddr*)socket->taskqc_addr,addr_size) == -1){
        fprintf(stderr,"error::taskqc_send_msg::connect::%d",errno);
        return errno;
    }
    ssize_t nwrote = send(socket->socket,buffer,buffer_sz,0);
    if (nwrote == -1){
        fprintf(stderr,"error::taskqc_send_msg::send::%d",errno);
        return errno;
    }
    printf("SENT DATA! %li\n",nwrote);
    return 0;
}

int taskqc_recv(taskqc_socket* socket, void* buffer, uint32_t buffer_sz, uint32_t flags){
        if (buffer_sz > MAX_FRAME_SIZE){
            fprintf(stderr,"error::taskqc_recv::MAX_FRAME_SIZE_EXCEEDED");
            return -1;
        }
        if (buffer_sz < 1){
            fprintf(stderr,"error::taskqc_recv::MIN_FRAME_SIZE_NOT_MET");
            return -1;
        }
        struct sockaddr_storage client_addr;
        socklen_t client_addr_size = sizeof(client_addr);
        int new_fd = accept(socket->socket,(struct sockaddr*)&client_addr,&client_addr_size);
        if (new_fd == -1){
            fprintf(stderr,"error::taskqc_recv_msg::accept::%d",errno);
            return errno;
        }
        ssize_t nread = recv(new_fd,buffer,buffer_sz,0);
        if (nread < 1){
            fprintf(stderr,"error::taskqc_recv_msg::recv::%d",errno);
            return errno;
        }
        printf("%li\n",nread);
        printf("Got the string! %s\n",(char*)buffer);
        return 0;
}
