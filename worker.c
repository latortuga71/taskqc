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
    int stdoutlink[2];
    int stderrlink[2];
    pid_t pid;
    char stdoutbuffer[4096];
    char stderrbuffer[4096];
    if (pipe(stdoutlink) == -1){
        fprintf(stderr,"error::worker_process::pipe::%d\n",errno);
        exit(-1);
    }
    if (pipe(stderrlink) == -1){
        fprintf(stderr,"error::worker_process::pipe::%d\n",errno);
        exit(-1);
    }
    if ((pid = fork()) == -1){
        fprintf(stderr,"error::worker_process::fork::%d\n",errno);
        exit(-1);
    }
    // child process
    if (pid == 0){
        dup2(stdoutlink[1],STDOUT_FILENO);
        dup2(stderrlink[1],STDERR_FILENO);
        close(stdoutlink[0]);
        close(stdoutlink[1]);
        close(stderrlink[0]);
        close(stderrlink[1]);
        char* argv[] = {msg.data,NULL};
        execvp(msg.data,argv);
        exit(0);
    } else {
        close(stdoutlink[1]);
        close(stderrlink[1]);
        int stdoutbytes = read(stdoutlink[0],stdoutbuffer,sizeof(stdoutbuffer));
        int stderrbytes = read(stderrlink[0],stderrbuffer,sizeof(stderrbuffer));
        printf(" * Command STDOUT\n %s\n",stdoutbuffer);
        printf(" * Command STDERR\n %s\n",stderrbuffer);
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
