#include "sql_connection_pool.h"
#include <mysql/mysql.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <iostream>
using namespace::std;

sql_connection_pool::sql_connection_pool()
{
    m_curConn=0;
    m_freeConn=0;
}
sql_connection_pool *sql_connection_pool::GetInstance()
{
    static sql_connection_pool connPool;
    return &connPool;
}

//构造初始化
void sql_connection_pool::init(string url, string User, string PassWord, string databaseName, int port, int maxConn, int close_log)
{
    m_url=url;
    m_port=port;
    m_user=User;
    m_password=PassWord;
    m_dataname=databaseName;
    m_close_log=close_log;

    for(int i=0;i<maxConn;i++)
    {
        MYSQL *con=NULL;
        con=mysql_init(con);

        if(con==NULL)
        {
            //LOG_ERROR("MySQL Error");
            cout<<"MySQL Error"<<endl;
            exit(1);
        }
        con=mysql_real_connect(con,url.c_str(),User.c_str(),PassWord.c_str(),databaseName.c_str(),port,NULL,0);
        if(con==NULL)
        {
            //LOG_ERROR("MySQL Error");
            cout<<"MySQL Error"<<endl;
            exit(1);
        }
        //更新连接池和空闲连接数量
        connList.push_back(con);
        ++m_freeConn;
    }
    //将信号量初始化为最大连接次数
    reserve=sem(m_freeConn);
    m_maxConn=m_freeConn;
}

MYSQL *sql_connection_pool::GetConnection()
{
    MYSQL *con=NULL;
    if(0==connList.size())
        return NULL;

    //取出连接，信号量原子减1，为0则等待
    reserve.wait();
    lock.lock();
    con=connList.front();
    connList.pop_front();
    --m_freeConn;
    ++m_curConn;

    lock.unlock();
    return con;
}

bool sql_connection_pool::ReleaseConnection(MYSQL *conn)
{
    if(NULL==conn)
        return false;
    lock.lock();
    connList.push_back(conn);
    ++m_freeConn;
    --m_curConn;
    lock.unlock();
    lock.unlock();
    //释放连接原子加1
    reserve.post();
    return true;
}
void sql_connection_pool::DestroyPool()
{
    lock.lock();
    if(connList.size()>0)
    {
        list<MYSQL*>::iterator it;
        for(it=connList.begin();it!=connList.end();++it)
        {
            MYSQL*con=*it;
            mysql_close(con);
        }
        m_curConn=0;
        m_freeConn=0;
        connList.clear();
    }
    lock.unlock();
}

int sql_connection_pool::GetFreeConn()
{
    return this->m_freeConn;
}

sql_connection_pool::~sql_connection_pool()
{
    DestroyPool();
}

connectionRAII::connectionRAII(MYSQL **SQL,sql_connection_pool *connPool)
{
    *SQL=connPool->GetConnection();
    conRAII=*SQL;
    poolRAII=connPool;
}
connectionRAII::~connectionRAII()
{
    poolRAII->ReleaseConnection(conRAII);
}
