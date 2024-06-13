#include "thread_pool.h"
#include <pthread.h>

void queueInit(task_queue_t* que){
    if(que){
        que->pFront = que->pRear = NULL;
        que->queueSize = 0;
        que->exitFlag = 0;
        int ret = pthread_mutex_init(&que->mutex, NULL);
        THREAD_ERROR_CHECK(ret, "pthread_mutex_init");
        ret = pthread_cond_init(&que->cond, NULL);
        THREAD_ERROR_CHECK(ret, "pthread_cond_init");
    }
}

//当调用该函数时,队列中的任务结点已经全部出队
void queueDestroy(task_queue_t* que){
    if(que){
        int ret = pthread_mutex_destroy(&que->mutex);
        THREAD_ERROR_CHECK(ret, "pthread_mutex_destroy");
        ret = pthread_cond_destroy(&que->cond);
        THREAD_ERROR_CHECK(ret , "pthread_cond_destroy");
    }
}

int taskSize(task_queue_t* que){
    return que->queueSize;
}

int queueIsEmpty(task_queue_t* que){
    return que->queueSize == 0;
}

//由生产者(主线程)调用
void taskEnqueue(task_queue_t* que, int peerfd){
    //断言, 当que为NULL时,程序会直接崩溃
    assert(que != NULL);
    //创建一个新的结点
    task_t* pNode = (task_t*)calloc(1, sizeof(task_t));
    pNode->peerfd = peerfd;
    pNode->pNext = NULL;
    //要往链表中存放新的结点,要先加锁,再进行头插法
    pthread_mutex_lock(&que->mutex);
    if(queueIsEmpty(que)){
       que->pFront = que->pRear = pNode;
    }else{
        que->pRear->pNext = pNode;
        que->pRear = pNode;
    }
    que->queueSize++;
    
    //通知消费者线程取结点
    pthread_cond_signal(&que->cond);
    pthread_mutex_unlock(&que->mutex);
}

//由消费者(子进程)调用
int taskDequeue(task_queue_t* que){
    assert(que != NULL);  //断言, 当 que == NULL 时,程序会直接崩溃
    //加锁
    pthread_mutex_lock(&que->mutex);
    //队列中没有元素时,要进入等待状态
    //使用while是为了防止出现虚假唤醒
    while(!que->exitFlag && queueIsEmpty(que)){
        pthread_cond_wait(&que->cond, &que->mutex);
    }

    int ret = -1;
    if(!que->exitFlag){
        //元素出队操作
        ret = que->pFront->peerfd;
        task_t* pDelete = que->pFront->pNext;
        //头插法
        if(taskSize(que) > 1){
            que->pFront = que->pFront->pNext;
        }else{  //仅有一个元素,出队后队列为空
            que->pFront = que->pRear = NULL;
        }
        //释放待删除的结点
        free(pDelete);
        que->queueSize--;
    }
    pthread_mutex_unlock(&que->mutex);
    return ret;
}

void broadcastAll(task_queue_t* que){
    //退出标识位设置为1
    que->exitFlag = 1;
    pthread_cond_broadcast(&que->cond);
}
