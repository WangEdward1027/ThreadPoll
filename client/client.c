#include <func.h>

//带进度条的客户端

//接受确定个字节的数据
int recvn(int sockfd, void* buff, int len){
    int left = len;
    char* pbuf = (char*)buff;
    int ret = 0;
    while(left > 0){
        ret = recv(sockfd, pbuf, left, 0);
        if(ret < 0){
            perror("recv");
            break;
        }else if(ret == 0){
            break;
        }
        pbuf += ret;
        left -= ret;
    }
    return len - left;
}

int main(){
    //创建客户端套接字
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_CHECK(clientfd, -1, "socket");

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0 , sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(8080);
    serveraddr.sin_addr.s_addr = inet_addr("192.168.248.136");

    int ret = connect(clientfd, (const struct sockaddr*)&serveraddr, sizeof(serveraddr));
    ERROR_CHECK(ret, -1, "connect");

    //先接收文件名
    char filename[100] = {0};
    int len = 0;
    ret = recvn(clientfd, (char*)&len, sizeof(len));  //先接长度
    printf("ret:%d, filename's len: %d\n", ret, len);
    ret= recvn(clientfd, filename, len);  //再接内容
    printf("ret:%d, filename: %s\n", ret, filename);

    int wfd = open(filename, O_CREAT | O_RDWR, 0644);
    ERROR_CHECK(wfd, -1, "open");

    //再获取的是文件长度
    off_t length = 0;
    recvn(clientfd, (char*)&length, sizeof(length));
    printf("file length: %ld\n", length);
    
    //进度条
    off_t splice = length / 100;
    off_t currSize = 0;
    off_t lastSize = 0;

    //最后接收文件内容
    char buff[1000] = {0};
    while(1){
        ret = recvn(clientfd, (char*)&len, sizeof(len));  //先接长度
        if(ret == 0){
            break;
        }
        //可以确定接收len个字节的长度
        ret = recvn(clientfd, buff, len);  //再接文件内容
        if(ret != 1000){
            printf("ret: %d\n", ret);
        }
        //最后写入本地
        write(wfd, buff, ret);
        //更新当前接收的数据
        currSize += ret;
        if(currSize - lastSize > splice){
            printf("has complete %5.2lf%%\n",(double)currSize / length * 100);
            /* printf("has complete %5.2lf%%\r",(double)currSize / length * 100); */
            /* fflush(stdout); */
            lastSize = currSize;
        }
    }
    printf("has complete 100.00%%\n");
    close(wfd);
    close(clientfd);
}
