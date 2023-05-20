#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include "taskqc.h"


#define CLIENT_LISTENER 0
#define WORKER_LISTENER 1
#define MAX_MSG_SIZE 1024

// client listener configs
char* global_client_address = NULL;
pthread_t client_thread_pool[1024];
int global_client_port = 0;
int client_thread_index = 0;
// worker listener configs
char* global_worker_address = NULL;
pthread_t worker_thread_pool[1024];
int global_worker_port = 0;
task_queue* global_task_queue = NULL;
int worker_thread_index = 0;

void* worker_client_handler(void* arg){
    taskqc_socket socket = *((taskqc_socket*)arg);
    char buffer[1024];
    while(1){
        sleep(1);
        taskqc_msg* task = pop_queue_msg(global_task_queue);
        if (task == NULL){
            fprintf(stderr,"debug::broker_worker_handler::no tasks to pop\n");
            continue;
        }
        fprintf(stderr,"debug::broker_worker_handler::popped task %s off queue\n",(char*)task->data);
        print_queue(global_task_queue);
        if (taskqc_send_msg(&socket,task) != 0){
            fprintf(stderr,"debug::broker_worker_handler::failed to send task to %d socket",socket.socket);
            push_queue_msg(global_task_queue, task);
            printf("push\n");
            sleep(60);
        } else {
            recv(socket.socket,buffer,sizeof(buffer),0);
            printf("Got this from the client! -> %s\n",buffer);
            printf("Worker ready for more\n");
            free(task->data);
            free(task);
        }
    }
    close(socket.socket);
    worker_thread_index--;
    pthread_exit(NULL);
}

void* broker_client_handler(void* arg){
    int fd = *((int*)arg);
    taskqc_msg* msg1 = calloc(1,sizeof(taskqc_msg));
    taskqc_recv_msg(fd,msg1);
    push_queue_msg(global_task_queue, msg1);
    fprintf(stderr,"debug::broker_client_handler::pushed task %s on queue\n",(char*)msg1->data);
    print_queue(global_task_queue);
    char ok_buffer[] = "OK\0";
    int nwrote = send(fd, ok_buffer, sizeof(ok_buffer),0);
    if (nwrote == -1){
        fprintf(stderr,"error::broker_client_handler::failed to send client OK\n");
    } else {
        fprintf(stderr,"debug::broker_client_handler::sent %d bytes client OK!\n",nwrote);
    }
    close(fd);
    client_thread_index--;
    pthread_exit(NULL);
}


void* broker_client_listener(){
    taskqc_socket* client_socket = taskqc_socket_init(9999,"127.0.0.1");
    int result = taskqc_socket_bind(client_socket,10);
    assert(result == 0);
    fprintf(stderr,"debug::broker_client_listener::listening on port ?::\n");
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    char s[INET6_ADDRSTRLEN];
    while(1){
        struct sockaddr_storage client_addr;
        socklen_t client_addr_size = sizeof(client_addr);
        int new_fd = accept(client_socket->socket,(struct sockaddr*)&client_addr,&client_addr_size);
        if (new_fd == -1){
            fprintf(stderr,"error::broker_client_listener::taskqc_recv_msg::accept::%d",errno);
            continue;
        }
        inet_ntop(client_addr.ss_family,get_client_address((struct sockaddr*)&client_addr),s,sizeof(s));
        fprintf(stderr,"debug::broker::broker_client_listener::client connection from %s\n",s);
        pthread_create(&client_thread_pool[client_thread_index++],NULL,&broker_client_handler,&new_fd);
    }
}

void* worker_client_listener(){
    taskqc_socket* client_socket = taskqc_socket_init(9998,"127.0.0.1");
    int result = taskqc_socket_bind(client_socket,10);
    assert(result == 0);
    fprintf(stderr,"debug::worker_client_listener::listening on port ?::\n");
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    char s[INET6_ADDRSTRLEN];
    while(1){
        struct sockaddr_storage client_addr;
        socklen_t client_addr_size = sizeof(client_addr);
        int new_fd = accept(client_socket->socket,(struct sockaddr*)&client_addr,&client_addr_size);
        if (new_fd == -1){
            fprintf(stderr,"error::worker_client_listener::taskqc_recv_msg::accept::%d",errno);
            continue;
        }
        inet_ntop(client_addr.ss_family,get_client_address((struct sockaddr*)&client_addr),s,sizeof(s));
        fprintf(stderr,"debug::broker::worker_client_listener::worker connection from %s\n",s);
        taskqc_socket worker_socket = {
            .socket = new_fd,
            .taskqc_addr = NULL,
            .max_connections = 0,
        };
        pthread_create(&worker_thread_pool[worker_thread_index++],NULL,&worker_client_handler,&worker_socket);
    }
    printf("listener end\n");
}

int main(int argc,char** argv){
    /*
     * Args for max queue size
     * Args for max client threads at once.
     * Args for listening port
     * Args for listening ip
     */
    // this stops broken pipes from crashing whole server.
    signal(SIGPIPE,SIG_IGN);
    fprintf(stderr,"debug::main::starting broker...\n");
    global_task_queue = init_queue(10);
    pthread_t server_threads[2];
    pthread_create(&server_threads[CLIENT_LISTENER],NULL,&broker_client_listener,NULL);
    pthread_create(&server_threads[WORKER_LISTENER],NULL,&worker_client_listener,NULL);
    pthread_join(server_threads[CLIENT_LISTENER],NULL);
    pthread_join(server_threads[WORKER_LISTENER],NULL);
    printf("Server End\n");
    return 0;
}
