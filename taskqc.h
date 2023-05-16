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
#include <pthread.h>
#include <sys/un.h>

#define MAX_FRAME_SIZE 1024
#define MAX_BYTE_ARRAY_SIZE 1024
#define TASKQC_WORKER_SOCKET "/tmp/taskqc_worker.socket"

typedef struct {
    uint32_t length;
    void* data;
} taskqc_msg;


typedef struct {
    pthread_mutex_t lock;
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

typedef struct {
    int socket;
    int max_connections;
    struct sockaddr_un* taskqc_addr;
} taskqc_socket_unix;

taskqc_msg* taskqc_msg_init(uint32_t length, void* data){
    taskqc_msg* msg =  (taskqc_msg*)calloc(1,sizeof(taskqc_msg));
    msg->length = length;
    msg->data = calloc(length,sizeof(char));
    strncpy(msg->data,data,length);
    return msg;
}

void taskqc_msg_delete(taskqc_msg* msg){
    free(msg->data);
    free(msg);
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

taskqc_socket_unix* taskqc_unix_socket_init(){
    int taskqc_sock = socket(AF_UNIX,SOCK_STREAM,0);
    struct sockaddr_un* taskqc_addr = (struct sockaddr_un*)calloc(1,sizeof(struct sockaddr_un));
    taskqc_addr->sun_family = AF_UNIX;
    strncpy(taskqc_addr->sun_path,TASKQC_WORKER_SOCKET,strlen(TASKQC_WORKER_SOCKET));
    taskqc_socket_unix* sock = (taskqc_socket_unix*)calloc(1,sizeof(taskqc_socket_unix));
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

void taskqc_unix_socket_delete(taskqc_socket_unix* socket){
    close(socket->socket);
    free(socket->taskqc_addr);
    free(socket);
    return;
}

task_queue* init_queue(uint32_t capacity);


task_queue* init_queue(uint32_t capacity){
    task_queue* tq = (task_queue*)malloc(sizeof(task_queue));
    if (pthread_mutex_init(&tq->lock,NULL) != 0){
        fprintf(stderr,"error::init_queue::pthread_mutex_init::failed");
        exit(-1);
    }
    tq->capacity = capacity;
    tq->current = 0;
    tq->front = (taskqc_msg**)malloc(sizeof(task_queue*) * capacity);
    tq->rear = tq->front;
    return tq;
}

void delete_queue_msgs(task_queue* q){
    pthread_mutex_lock(&q->lock);
    for (int x = 0; x < q->current; x++){
        taskqc_msg_delete(q->front[x]);
        free(q->front[x]);
    }
    pthread_mutex_unlock(&q->lock);
}

void delete_queue(task_queue* q){
    pthread_mutex_lock(&q->lock);
    free(q->front);
    pthread_mutex_unlock(&q->lock);
    pthread_mutex_destroy(&q->lock);
    free(q);
}

int queue_has_item(task_queue* queue){
    pthread_mutex_lock(&queue->lock);
    if (queue->current > 0){
        pthread_mutex_unlock(&queue->lock);
        return 1;
    }
    pthread_mutex_unlock(&queue->lock);
    return 0;
}

void print_queue(task_queue* queue){
    pthread_mutex_lock(&queue->lock);
    for (int x = 0; x < queue->current; x++){
        printf("[%d] -> %d\n",x, queue->front[x]->length);
        if (queue->front[x] == NULL){
            pthread_mutex_unlock(&queue->lock);
            return;
        }
    }
    pthread_mutex_unlock(&queue->lock);
    return;
}

taskqc_msg* pop_queue_msg(task_queue* queue){
    // lock the mutex
    pthread_mutex_lock(&queue->lock);
    if (queue->current == 0){
        pthread_mutex_unlock(&queue->lock);
        return NULL;
    }
    taskqc_msg* popped = queue->front[0];
    for (int x = 0; x < queue->current; x++){
        queue->front[x] = queue->front[x+1];
    }
    // the one at the end is empty now we can just move current down 1
    queue->current--;
    // set the rear again;
    queue->rear = &queue->front[queue->current];
    pthread_mutex_unlock(&queue->lock);
    return popped;
}

void push_queue_msg(task_queue* queue,taskqc_msg* msg){
    // if queue is empty add the front and rear point to the same place
    pthread_mutex_lock(&queue->lock);
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
    pthread_mutex_unlock(&queue->lock);
}


int taskqc_socket_bind_unix(taskqc_socket_unix* socket, int max_conns){
    unlink(TASKQC_WORKER_SOCKET);
    if (bind(socket->socket,(struct sockaddr*) socket->taskqc_addr, sizeof(struct sockaddr_un)) == -1){
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

int taskqc_recv_msg(int fd, taskqc_msg* msg){
        // read first byte to get length of data.
        uint32_t frame_length = 0;
        ssize_t nread = recv(fd,&frame_length,sizeof(frame_length),0);
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
        nread = recv(fd,buffer,frame_length,0);
        if (nread < 1){
            fprintf(stderr,"error::taskqc_recv_msg::recv::%d",errno);
            return errno;
        }
        /*printf("READ %li\n",nread);
        char dump[1024];
        while(nread != 0){
            printf("draining buffer\n");
            nread = recv(new_fd,dump, 1024, 0);
            printf("%li\n",nread);
        }
        printf("done dump\n");
        */
        msg->length = frame_length;
        msg->data = calloc(frame_length,sizeof(char));
        memcpy(msg->data,buffer,frame_length);
        free(buffer);
        return 0;
}

int taskqc_recv_msg_unix(taskqc_socket_unix* socket, taskqc_msg* msg){
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

int taskqc_send_msg_unix(taskqc_socket_unix* socket, taskqc_msg* msg){
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

int taskqc_connect_unix(taskqc_socket_unix* socket){
    socklen_t addr_size = sizeof(struct sockaddr_un);
    if (connect(socket->socket,(struct sockaddr*)socket->taskqc_addr,addr_size) == -1){
        fprintf(stderr,"error::taskqc_connect_unix::connect::%d",errno);
        return errno;
    }
    return 0;
}
int taskqc_connect(taskqc_socket* socket){
    socklen_t addr_size = sizeof(struct sockaddr_in);
    if (connect(socket->socket,(struct sockaddr*)socket->taskqc_addr,addr_size) == -1){
        fprintf(stderr,"error::taskqc_connect::connect::%d",errno);
        return errno;
    }
    return 0;
}

int taskqc_send_msg(taskqc_socket* socket, taskqc_msg* msg){
    // call connect at some point before sending message
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
