#include "thread_pool.h"
#include <sys/epoll.h>

int exitPipe[2];
void sighandler(int num){
    printf("sig %d is coming.\n",num);
    //写管道,通知子进程退出
    int one = 1;
    write(exitPipe[1], &one, sizeof(one));
}

int main(int argc, char* argv[])
{
    //ip port threadNum
    ARGS_CHECK(argc, 4);

    //exitPipe用在父子进程间通信, 在fork之前创建
    pipe(exitPipe);

    pid_t pid = fork();
    if(pid  == -1){
        error(1, errno, "fork");
    }else if(pid > 0){  //父进程  
        signal(SIGUSR1, sighandler);  //只在父进程中注册10号信号
        wait(NULL);
        exit(0);
    }
    //else pid == 0 子进程的流程
    //初始化线程池
    threadpool_t threadpool;
    threadpoolInit(&threadpool, atoi(argv[3]));

    //启动线程池
    threadpoolStart(&threadpool);

    //创建TCP监听套接字
    int listenfd = tcpInit(argv[1], argv[2]);
    printf("server start listening.\n");
    
    //创建epoll实例
    int epfd = epoll_create1(0);
    ERROR_CHECK(epfd, -1 ,"epoll_create1");

    //添加监听
    epollAddFd(epfd, listenfd);
    epollAddFd(epfd, exitPipe[0]);

    struct epoll_event* pEventArr = (struct epoll_event*)calloc(EVENT_ARR_SIZE, sizeof(struct epoll_event));
    
    while(1){
        int nready = epoll_wait(epfd, pEventArr, EVENT_ARR_SIZE, -1);
        for(int i = 0; i < nready; ++i){
            int fd = pEventArr[i].data.fd;
            if(fd == listenfd){
                int peerfd = accept(listenfd, NULL, NULL);
                //将peerfd添加到任务队列中
                taskEnqueue(&threadpool.queue, peerfd);
            }else if(fd == exitPipe[0]){
                printf("read exitPipe[0]\n");
                int howmany = 0; //读取管道中的数据
                read(exitPipe[0], &howmany, sizeof(howmany));
                //执行线程池退出的流程
                threadpoolStop(&threadpool);
                threadpoolDestroy(&threadpool);
                close(listenfd);
                close(epfd);
                exit(0);  //子进程的退出
            }
        }
    }
    return 0;
}
