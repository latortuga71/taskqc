#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include "taskqc.h"

// global for now
//


task_queue* global_task_queue = NULL;
task_queue* global_results_queue = NULL;
pthread_t worker_thread_pool[1024];
int worker_thread_index = 0;
pthread_t client_thread_pool[1024];
int client_thread_index = 0;
taskqc_socket_unix* worker_socket = NULL;

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void* worker_process(void* arg){
    int link[2];
    pid_t pid;
    char buffer[4096];
    if (pipe(link) == -1){
        fprintf(stderr,"error::worker_process::pipe::%d\n",errno);
        exit(-1);
    }
    if ((pid = fork()) == -1){
        fprintf(stderr,"error::worker_process::fork::%d\n",errno);
        exit(-1);
    }
    // child process
    if (pid == 0){
        dup2(link[1],STDOUT_FILENO);
        close(link[0]);
        close(link[1]);
        execve("/usr/bin/who",NULL,NULL);
        exit(0);
    } else {
        close(link[1]);
        int nbytes = read(link[0],buffer,sizeof(buffer));
        printf(" * Command Result\n %s\n",buffer);
        wait(NULL);
    }
    taskqc_msg* out = taskqc_msg_init(strlen(buffer),buffer);
    push_queue_msg(global_results_queue,out);
    worker_thread_index--;
    pthread_exit(NULL);
}

void* worker_handler(){
    while(1){
        fprintf(stderr,"debug::worker_handler::checking_tasks_queue\n");
        /*
         * IN THE FUTURE MAYBE USE A MESSAGE QUEUE AND USE SELECT ? ON THE FILE DESCRIPTOR? SO WE DONT HAVE TO SLEEP
         *
         * 
         */
        if (queue_has_item(global_task_queue)){
            taskqc_msg* msg = pop_queue_msg(global_task_queue);
            fprintf(stderr,"debug::worker_handler::gotmsg Length: %d Data: %s\n",msg->length,(char*)msg->data);
            printf("For now we execve into whatever binary is passed in the data.\n");
            int worker_tid = pthread_create(&worker_thread_pool[worker_thread_index], NULL, &worker_process, NULL);
            pthread_detach(worker_thread_pool[worker_thread_index++]);
            free(msg->data);
            free(msg);
        } 
        sleep(1);
    }
}

void* results_handler(){
    while(1){
        fprintf(stderr,"debug::results_handler::checking_results_queue\n");
        /*
         * IN THE FUTURE MAYBE USE A MESSAGE QUEUE AND USE SELECT ? ON THE FILE DESCRIPTOR? SO WE DONT HAVE TO SLEEP WE CAN JUST BLOCK 
         *
         * 
         */
        if (queue_has_item(global_results_queue)){
            taskqc_msg* msg = pop_queue_msg(global_results_queue);
            fprintf(stderr,"debug::results_handler::gotmsg Length: %d Data: %s\n",msg->length,(char*)msg->data);
            printf("Got a result!\n");
            free(msg->data);
            free(msg);
        } else {
            sleep(1);
        }
    }
}

void* client_handler(void* arg){
        int fd = *((int*)arg);
        // cast to int
        printf("New Connection\n");
        taskqc_msg* msg1 = calloc(1,sizeof(taskqc_msg));
        taskqc_recv_msg(fd,msg1);
        printf("after recv\n");
        push_queue_msg(global_task_queue, msg1);
        printf("after push");
        printf("Length %d Data: \n%s",msg1->length,(char*)msg1->data);
        print_queue(global_task_queue);
        printf("Connection End\n");
        close(fd);
        client_thread_index--;
        pthread_exit(NULL);
}

int main(int argc,char** argv){
    fprintf(stderr,"TODO TASK TYPE THAT CAN BE SERIALIZED");
    fprintf(stderr,"TODO RESULTS TYPE THAT CAN BE SERIALIZED");
    fprintf(stderr,"HAVE WORKERS USE TASK TYPE AND CHOOSE BINARY");
    fprintf(stderr,"TODO INIT PROPER THREAD POOL FOR CLIENTS\n");
    fprintf(stderr,"TODO INIT PROPER THREAD POOL FOR WORKERS\n");
    fprintf(stderr,"TODO CHECK MAX THREADS AND DONT EXCEED\n");
    fprintf(stderr,"debug::main::init_worker_thread::\n");
    global_task_queue = init_queue(10);
    global_results_queue = init_queue(10);
    int error;
    int worker_thread_tid = pthread_create(&client_thread_pool[client_thread_index], NULL, &worker_handler, NULL);
    if (worker_thread_tid != 0){
        fprintf(stderr,"error::main::pthread_create::%d",errno);
        return -1;
    }
    pthread_detach(client_thread_pool[client_thread_index++]);
    int results_thread_tid = pthread_create(&client_thread_pool[client_thread_index], NULL, &results_handler, NULL);
    if (results_thread_tid != 0){
        fprintf(stderr,"error::main::pthread_create::%d",errno);
        return -1;
    }
    pthread_detach(client_thread_pool[client_thread_index++]);
    taskqc_socket* socket = taskqc_socket_init(9999,"127.0.0.1");
    int result = taskqc_socket_bind(socket,10);
    assert(result == 0);
    printf("Listening!\n");
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    char s[INET6_ADDRSTRLEN];
    while(1){
        struct sockaddr_storage client_addr;
        socklen_t client_addr_size = sizeof(client_addr);
        int new_fd = accept(socket->socket,(struct sockaddr*)&client_addr,&client_addr_size);
        if (new_fd == -1){
            fprintf(stderr,"error::taskqc_recv_msg::accept::%d",errno);
            return errno;
        }
        pthread_create(&client_thread_pool[client_thread_index++],NULL,&client_handler,&new_fd);
    }
    return 0;
}
