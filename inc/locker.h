#ifndef LOCKER_H
#define LOCKER_H

#include<pthread.h>
#include<exception>
#include<semaphore.h>
// 线程同步机制封装类(线程同步用得到的东西)

// 互斥锁类
class locker{
public:
    // 构造函数
    locker(){
        // 返回值为0，说明调用成功
        if(pthread_mutex_init(&m_mutex, NULL)!=0){
            throw std::exception();
        }
    }

    // 析构函数
    ~locker(){
        pthread_mutex_destroy(&m_mutex);
    }

    // 调用加锁
    bool lock(){
        // 返回为0，说明调用成功，成功加锁了
        return pthread_mutex_lock(&m_mutex)==0;
    }

    // 调用解锁
    bool unlock(){
        // 返回为0，说明调用成功，成功解锁了
        return pthread_mutex_unlock(&m_mutex)==0;
    }

    // 获取成员变量
    pthread_mutex_t * get(){
        return &m_mutex;
    }
private:
    //成员包含了一个互斥锁
    pthread_mutex_t m_mutex;
};


// 条件变量类
class cond{
public:
    cond(){
        // 返回值为0，说明调用成功
        if(pthread_cond_init(&m_cond,NULL)!=0){
            throw std::exception();
        }
    }

    ~cond(){
        pthread_cond_destroy(&m_cond);
    }

    bool wait(pthread_mutex_t *mutex){
        return pthread_cond_wait(&m_cond, mutex) == 0;
    }

    bool timedwait(pthread_mutex_t *mutex, struct timespec t){
        return pthread_cond_timedwait(&m_cond, mutex, &t) == 0;
    }

    bool signal(){
        return pthread_cond_signal(&m_cond) == 0;
    }

    bool broadcast(){
        return pthread_cond_broadcast(&m_cond) == 0;
    }

private:
    pthread_cond_t m_cond;
};


// 信号量类

class sem{
public:
    sem(){
        if(sem_init(&m_sem, 0, 0) != 0){
            throw std::exception();
        }
    }
    // 带参数的构造函数,根据参数来初始化信号量的值
    sem(int num){
        if(sem_init(&m_sem, 0, num) != 0){
            throw std::exception();
        }
    }
    ~sem(){
        sem_destroy(&m_sem);
    }

    bool wait(){
        return sem_wait(&m_sem) == 0;
    }

    bool post(){
        return sem_post(&m_sem) == 0;
    }
private:
    sem_t m_sem;
};

#endif