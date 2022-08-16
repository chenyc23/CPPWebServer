#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<pthread.h>
#include<list>
#include<cstdio>
#include<exception>
#include"locker.h"

// 线程池的类
// 定义成模板类，是为了以后改了任务类型还能继续用
template<typename T>
class threadpool{
public:
    threadpool(int thread_number = 8, int max_requests =10000);
    ~threadpool();
    // 往线程池内添加任务
    bool append(T* request);

private:
    static void* worker(void *arg);
    void run();

private:
    // 线程的数量
    int m_thread_number;

    // 线程池数组，用来装各种线程，大小为m_thread_number
    pthread_t *m_threads;

    // 请求队列中最多允许的，等待处理的请求数量
    int m_max_requests;

    // 请求队列
    std::list< T*> m_workqueue;

    // 互斥锁
    locker m_queuelocker;

    // 信号量，用来判断是否有任务需要进行处理
    sem m_queuestat;

    // 是否结束线程，根据这个判断是不是需要结束线程
    bool m_stop;
};

template<typename T>
threadpool<T>::threadpool(int thread_number, int max_requests) :
    m_thread_number(thread_number),
    m_max_requests(max_requests),
    m_stop(false),
    m_threads(NULL){
    
    if((thread_number <= 0) || (max_requests <= 0)){
        throw std::exception();
    }

    // 根据thread_number创建对应个数的线程数组
    m_threads = new pthread_t[m_thread_number];
    if(!m_threads){
        throw std::exception();
    }

    // 设置创建线程并进行线程分离
    for(int i=0; i<m_thread_number; i++){
        printf("create the %dth thread\n",i);
        
        // 需要注意的是，第三个参数worker函数在c++中必须为静态成员函数(没有this指针)
        // c语言中就必须为全局函数
        if(pthread_create(m_threads + i, NULL, worker, this)!=0){
            delete[] m_threads;
            throw std::exception();
        }

        // create要的是地址，因为是传出参数，detach不需要地址，因为不需要传出
        if(pthread_detach(m_threads[i])){
            delete[] m_threads;
            throw std::exception();
        }
    }

}


// 类的析构函数
template<typename T>
threadpool<T>::~threadpool(){
    delete[] m_threads;
    m_stop = true;
}

// 往线程池中增加任务
template<typename T>
bool threadpool<T>::append(T* request){
    
    m_queuelocker.lock();
    // 请求队列已经满了,就不允许添加了
    if(m_workqueue.size() >= m_max_requests){
        m_queuelocker.unlock();
        return false;
    }

    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    // 信号量表示目前还有多少个任务
    // 队列新增了一个任务，就要让信号量+1
    m_queuestat.post();
    return true;
}

template<typename T>
void* threadpool<T>::worker(void* arg){
    // 把this指针传进来了，这样就可以通过this指针来访问类内成员了
    threadpool* pool = (threadpool*) arg;
    // 线程从请求队列中取数据进行工作
    pool->run();
    return pool;
}

template<typename T>
void threadpool<T>::run(){
    // 一直循环，直到m_stop为false
    while(!m_stop){
        // 判断有没有任务，没有任务是会阻塞在这里的
        m_queuestat.wait();
        // 执行任务是要上锁的
        m_queuelocker.lock();

        if(m_workqueue.empty()){
            m_queuelocker.unlock();
            continue;
        }
        
        T* request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();

        if(!request){
            continue;
        }
        
        // 注意，任务类里面需要写如何执行任务
        //
        // 
        request->process();
    }
}

#endif