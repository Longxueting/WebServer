#ifndef THREADPOOL_H
#define THREADPOOL_H

//这是一个利用半同步/半反应堆实现的线程池，反应堆为proactor模式
#include <list>
#include <cstdio>
#include <exception>
#include "locker.h"
#include "sql_connection_pool.h"
#include <pthread.h>
#include <iostream>

template <typename T>
class threadpool{
public:
    //thread_number是线程池中线程的数量
    //max_requests是请求队列中最多允许的、等待处理的请求的数量
    //connPool是数据库连接池指针
    threadpool(int actor_model,sql_connection_pool *connPool,int thread_number=8,int max_requests=10000);
    ~threadpool();
    //请求队列中插入任务请求
    bool append(T* request,int state);
    bool append_p(T* request);

private:
    //工作线程的运行函数，它不断的从工作队列中取出任务并执行
    static void* worker(void* arg);
    void run();
private:
    int m_thread_number;        //线程池中的线程数
    int m_max_requests;         //请求队列中允许的最大请求数
    pthread_t *m_threads;       //描述线程池的数组，其大小为m_thread_number
    std::list<T *> m_workqueue; //请求队列
    locker m_queuelocker;       //保护请求队列的互斥锁
    sem m_queuestat;            //是否有任务需要处理
    sql_connection_pool *m_connPool;  //数据库
    int m_actor_model;          //模型切换
};

template <typename T>
threadpool<T>::threadpool( int actor_model, sql_connection_pool *connPool, int thread_number, int max_requests) : m_actor_model(actor_model),m_thread_number(thread_number), m_max_requests(max_requests), m_threads(NULL),m_connPool(connPool)
{
    if(thread_number<=0 || max_requests<=0){
        throw std::exception();
    }
    //线程id初始化
    m_threads=new pthread_t[m_thread_number];
    if(!m_threads){
        throw std::exception();
    }

    //创建thread_number个线程，并将它们都设置为脱离线程
    for(int i=0;i<thread_number;++i){
        //std::cout<<"create the "<<i<<"th thread"<<std::endl;
        //循环创建线程，并将工作线程按要求进行运行
        if(pthread_create(m_threads+i,NULL,worker,this)!=0){
            delete [] m_threads;
            throw std::exception();
        }
        //将线程进行分离后，不用单独堆工作线程进行回收
        if(pthread_detach(m_threads[i])){
            delete [] m_threads;
            throw std::exception();
        }
    }
}

template <typename T>
threadpool<T>::~threadpool<T>(){
    delete [] m_threads;
};

template <typename T>
bool threadpool<T>::append(T* request,int state){
    m_queuelocker.lock();
    if(m_workqueue.size()>=m_max_requests){
        m_queuelocker.unlock();
        return false;
    }
    request->m_state=state;
    //添加任务
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post(); //发送信号，信号量提醒有任务要处理
    return true;
}

template <typename T>
bool threadpool<T>::append_p(T* request){
    m_queuelocker.lock();
    if(m_workqueue.size()>=m_max_requests){
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}

template <typename T>
void* threadpool<T>::worker(void* arg){
    //将参数强制转换为线程池类，调用成员方法
    threadpool* pool=(threadpool*)arg;
    pool->run();
    return pool;
}

template  <typename T>
void threadpool<T>::run(){
    while (1) {
        //信号量等待
        m_queuestat.wait();
        //被唤醒后先加互斥锁
        m_queuelocker.lock();
        if(m_workqueue.empty()){
            m_queuelocker.unlock();
            continue;
        }
        //从请求队列中取出第一个任务
        //将任务从请求队列删除
        T* request=m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if(!request){
            continue;
        }
        if(m_actor_model==1){
            if(request->m_state==0){
                if(request->read_once()){
                    request->improv=1;
                    connectionRAII mysqlcon(&request->mysql, m_connPool);
                    request->process();
                }
                else{
                    request->improv=1;
                    request->timer_flag=1;
                }
            }
            else{
                if(request->write()){
                    request->improv=1;
                }
                else{
                    request->improv=1;
                    request->timer_flag=1;
                }
            }
        }
        else{
            connectionRAII mysqlcon(&request->mysql,m_connPool);
            //process(模板类中的方法,这里是http类)进行处理
            request->process();
        }
    }
}
#endif // THREADPOOL_H
