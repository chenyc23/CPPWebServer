#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <signal.h>
#include "http_conn.h"
#include "locker.h"
#include "threadpool.h"

#define MAX_FD 65535  //最大的文件描述符个数
#define MAX_EVENT_NUMBER 10000 // epoll事件最大监听数量

// 添加信号捕捉的函数,调用一次，就可以注册一种信号的处理
void addsig(int sig, void(handler)(int)){
    struct sigaction sa;

    sa.sa_flags = 0;
    sa.sa_handler = handler;
    // 设置临时阻塞信号集
    sigfillset(&sa.sa_mask);

    sigaction(sig, &sa, NULL);
}

// 添加文件描述符到epoll中,设置epoll监听事件
extern void addfd(int epollfd, int fd, bool one_shot);
// 从epoll中删除文件描述符
extern void removefd(int epollfd, int fd);
// 修改epoll中的文件描述符
extern void modfd(int epollfd, int fd, int ev);


int main(int argc, char* argv[]){

    if(argc <= 1){
        printf("按照如下格式运行: %s port_number\n", basename(argv[0]));
        exit(-1);
    }

    // 获取端口号
    int port = atoi(argv[1]);

    // 对SIGPIE信号进行处理
    addsig(SIGPIPE, SIG_IGN);

    // 创建线程池，初始化线程池
    // 主线程把连接能获得的信息先全获得，然后在封装成任务类，最后把任务类传给子线程
    threadpool<http_conn> *pool = NULL;
    try{
        pool = new threadpool<http_conn>;
    } catch(...){
        exit(-1);
    }

    // 创建一个数组用于保存所有的客户端信息
    http_conn *users = new http_conn[MAX_FD];

    // 创建一个监听的socket
    int listenfd = socket(AF_INET,SOCK_STREAM, 0 );
    if(listenfd == -1){
        perror("socket");
        exit(-1);
    }

    // 设置端口复用
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    // 绑定
    int ret = bind(listenfd,(struct sockaddr *)&address, sizeof(address));
    if(ret == -1){
        perror("bind");
        exit(-1);
    }

    // 监听
    ret = listen(listenfd, 5);
    if(ret == -1) {
        perror("listen");
        exit(-1);
    }

    // 调用epoll_create()创建一个epoll实例
    int epfd = epoll_create(100);
    struct epoll_event epevs[MAX_EVENT_NUMBER];

    // 将监听的文件描述符相关的检测信息添加到epoll实例中
    addfd(epfd, listenfd, false);
    http_conn::m_epollfd = epfd;

    while(true){
        int num = epoll_wait(epfd, epevs, MAX_EVENT_NUMBER, -1);
        if((num < 0) && (errno != EINTR)){
            printf("epoll failure\n");
            break;
        }

        // 循环遍历事件数组
        for(int i = 0; i < num; i++){
            int sockfd = epevs[i].data.fd;
            if(sockfd == listenfd){
                //有客户端连接进来了
                struct sockaddr_in client_address;
                socklen_t client_addrlen = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr *)&client_address, &client_addrlen);
                if(connfd == -1){
                    perror("accept");
                    exit(-1);
                }

                if(http_conn::m_user_count >= MAX_FD){
                    // 目前连接数满了
                    // 给客户端写一个信息：服务器内部正忙。
                    close(connfd);
                    continue;
                }
                // 将新客户的数据初始化，放到数组中
                users[connfd].init(connfd, client_address);
            } else if(epevs[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                //对方异常断开了或者错误的事件发生了
                users[sockfd].close_conn();


            } else if(epevs[i].events & EPOLLIN){
                // 检测到读事件已经就绪
                if(users[sockfd].read()){
                    //如果一次性全部读完
                    pool->append(users + sockfd);
                }else{
                    users[sockfd].close_conn();
                }

                
            } else if(epevs[i].events & EPOLLOUT){
                if(users[sockfd].write()){
                    //一次性全部写完数据
                    users[sockfd].close_conn();
                }
            }
        }
    }    

    close(epfd);
    close(listenfd);
    delete [] users;
    delete pool;

    return 0;
}