#include <stdio.h>
#include <assert.h>
#include "taskqc.h"

pthread_t worker_thread_pool[1024];
int worker_thread_index = 0;

extern char** environ;

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void* worker_process(void* arg){
    taskqc_msg msg = *((taskqc_msg*)arg);
    printf("Worker Executing %s Task\n",(char*)msg.data);
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
        char* argv[] = {msg.data,NULL};
        execvp(msg.data,argv);
        exit(0);
    } else {
        close(link[1]);
        int nbytes = read(link[0],buffer,sizeof(buffer));
        printf(" * Command Result\n %s\n",buffer);
        wait(NULL);
    }
    sleep(5);
    //taskqc_msg* out = taskqc_msg_init(strlen(buffer),buffer);
    //push_queue_msg(global_results_queue,out);
    worker_thread_index--;
    printf("Task Done\n");
    pthread_exit(NULL);
}

int main(int argc,char** argv){
    fprintf(stderr,"TODO DEBUG/ERROR LOGS\n");
    fprintf(stderr,"TODO LOCK THREAD COUNT\n");
    fprintf(stderr,"TODO WORKER ARGS (name,host,broker,max processes etc,database\n");
    taskqc_socket* socket = taskqc_socket_init(9998,"127.0.0.1");
    taskqc_connect(socket);
    taskqc_msg msg;
    while(1){
        if (worker_thread_index >= 2){
            printf("Too many worker processes...sleeping\n");
            sleep(1);
            continue;
        } else {
            printf("Ready Waiting for task...\n");
            printf("%d Threads running\n",worker_thread_index);
            taskqc_recv_msg(socket->socket,&msg);
            pthread_create(&worker_thread_pool[worker_thread_index++],NULL,&worker_process,&msg);
            taskqc_send(socket,"OK!",3);
        }
    }

    taskqc_socket_delete(socket);
    return 0;
}
