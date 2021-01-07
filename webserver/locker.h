#ifndef LOCKER_H
#define LOCKER_H
//将线程同步的三种机制封装成三各类，分别是信号量，互斥量，条件变量

#include <exception>
#include <pthread.h>
#include <semaphore.h>

//封装信号量的类
class sem{
public:
    sem(){
        if(sem_init(&m_sem,0,0)!=0){    //构造函数成功返回0，失败返回errno
            throw std::exception();
        }
    }
    sem(int num)
    {
        if (sem_init(&m_sem, 0, num) != 0)
        {
            throw std::exception();
        }
    }
    ~sem(){
        sem_destroy(&m_sem);
    }

    bool wait(){
        return sem_wait(&m_sem)==0;
    }
    //增加信号量
    bool post(){
        return  sem_post(&m_sem)==0;
    }
private:
    sem_t m_sem;
};

//封装互斥量的类

class locker{
public:
    locker(){
        if(pthread_mutex_init(&m_mutex,NULL)!=0){
            throw std::exception();
        }
    }
    ~locker(){
        pthread_mutex_destroy(&m_mutex);
    }

    bool lock(){
        return pthread_mutex_lock(&m_mutex)==0;
    }

    bool unlock(){
        return pthread_mutex_unlock(&m_mutex)==0;
    }
    pthread_mutex_t* get(){
        return &m_mutex;
    }
private:
    pthread_mutex_t m_mutex;
};

//封装条件变量的类

class cond{
public:
    cond(){
        if(pthread_mutex_init(&m_mutex,NULL)!=0){
            throw  std::exception();
        }
        if(pthread_cond_init(&m_cond,NULL)!=0){
            pthread_mutex_destroy(&m_mutex);
            throw  std::exception();
        }
    }
    ~cond(){
        pthread_mutex_destroy(&m_mutex);
        pthread_cond_destroy(&m_cond);
    }

    //条件变量一般需要配合锁来使用，内部有一次加锁和解锁，封装起来会使得更加的简洁
    bool wait(pthread_mutex_t* m_mutex){
        int ret=0;
        //pthread_mutex_lock(&m_mutex);
        ret=pthread_cond_wait(&m_cond,m_mutex);
        //pthread_mutex_unlock(&m_mutex);
        return ret==0;
    }
    bool timewait(pthread_mutex_t* m_mutex,struct timespec t){
        int ret=0;
        ret=pthread_cond_timedwait(&m_cond,m_mutex,&t);
        return ret==0;
    }

    bool signal(){
        return pthread_cond_signal(&m_cond)==0;
    }
    bool broadcast(){
        return pthread_cond_broadcast(&m_cond);
    }
private:
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};

#endif // LOCKER_H
