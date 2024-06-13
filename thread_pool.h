#ifndef _WD_FUNC_H_
#define _WD_FUNC_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <error.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <assert.h>
#include <fcntl.h>  //open()的头文件
#include <sys/ioctl.h>
#include <pthread.h>
#include <sys/uio.h>


#define SIZE(a) (sizeof(a)/sizeof(a[0]))  //求数组长度的宏

typedef void (*sighandler_t)(int);  //函数指针

#define ARGS_CHECK(argc, num){           \
    if(argc != num){                     \
        fprintf(stderr, "ARGS ERROR!\n");\
        return -1;                       \
    }                                    \
}                                        \

#define ERROR_CHECK(ret, num, msg){ \
    if(ret == num){                 \
        perror(msg);                \
        return -1;                  \
    }                               \
}                                   \

#define THREAD_ERROR_CHECK(ret, func){                  \
    if(ret != 0){                                       \
        fprintf(stderr, "%s:%s\n", func, strerror(ret));\
    }                                                   \
}                                                       \

#define EVENT_ARR_SIZE 100

//小火车协议
typedef struct train{
    int len; 
    char buff[1000];
} train_t;

//任务结点 (任务队列中的任务结点)
typedef struct task_s{
    int peerfd;
    struct task_s* pNext;
} task_t;

//任务队列
typedef struct taskqueue_s{
    task_t* pFront;
    task_t* pRear;
    int queueSize;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int exitFlag;             
} task_queue_t;

//线程池
typedef struct threadpool_s{
    pthread_t* pthreads;  //动态数组,保存线程ID
    int threadNum;        //线程数量
    task_queue_t queue;   //任务队列
} threadpool_t;

//任务队列相关API
void queueInit(task_queue_t* que);
void queueDestroy(task_queue_t* que);
int taskSize(task_queue_t* que);
int queueIsEmpty(task_queue_t* que);
void taskEnqueue(task_queue_t* que, int peerfd);
int taskDequeue(task_queue_t* que);
void broadcastAll(task_queue_t* que);

//线程池相关API
void* threadFunc(void* arg);
void threadpoolInit(threadpool_t *, int num);
void threadpoolDestroy(threadpool_t *);
void threadpoolStart(threadpool_t *);
void threadpoolStop(threadpool_t *);

//TCP通信相关API
int tcpInit(const char* ip, const char* port);

//IO多路复用 epoll相关API
int epollAddFd(int epfd, int fd);
int epollDelFd(int epfd, int fd);

//文件传输相关API
int transferFile(int peerfd);

#endif 
