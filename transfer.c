#include "thread_pool.h"

#define FILENAME "big_Bible.txt"

int sendn(int sockfd, const void* buff, int len){
    int left = len;
    const char * pbuf = (const char*)buff;
    int ret = 0;
    while(left > 0){
        ret = send(sockfd, pbuf, left, 0);
        if(ret == -1){
            perror("send");
            return -1;
        }
        pbuf += ret;
        left -= ret;
    }
    return len - left;
}

int transferFile(int peerfd){
    //读取本地文件
    int fd = open(FILENAME, O_RDWR);
    ERROR_CHECK(fd, -1, "open");

    train_t t;
    memset(&t, 0, sizeof(t));
    //先发送文件名
    t.len = strlen(FILENAME);
    strcpy(t.buff, FILENAME);
    send(peerfd, &t, 4 + t.len, 0);

    //其次发送文件长度
    struct stat st;
    printf("filelength:%ld\n", st.st_size); //off_t
    printf("sizeof(st.st_size):%ld\n", sizeof(st.st_size));
    send(peerfd, &st.st_size, sizeof(st.st_size), 0);

    //最后发送文件内容
    while(1){
        memset(&t, 0, sizeof(t));
        int ret = read(fd, t.buff, sizeof(t.buff));
        if(ret != 1000){
            printf("read ret: %d\n", ret);
        }
        if(ret == 0){
            //文件已经读取完毕
            break;
        }
        t.len = ret;
        ret= sendn(peerfd, &t, 4 + t.len);
        if(ret == -1){
            break;
        }
        if(ret != 1004){
            printf("send ret: %d]n", ret);
        }
    }
    printf("send file over.\n");

    close(fd); //关闭文件
    return 0;
}
