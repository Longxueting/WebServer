#ifndef SQL_CONNECTION_POOL_H
#define SQL_CONNECTION_POOL_H
#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include "locker.h"
using namespace::std;

class sql_connection_pool
{
public:
    MYSQL *GetConnection(); //获取数据库连接
    bool ReleaseConnection(MYSQL *conn);    //释放连接
    int GetFreeConn();  //获取空闲的连接
    void DestroyPool(); //销毁所有连接

    //单例模式
    static sql_connection_pool *GetInstance();
    void init(string url,string User,string PassWord,string databaseName,int port,int maxConn,int close_log);
private:
    sql_connection_pool();
    ~sql_connection_pool();

    int m_maxConn;  //最大连接数
    int m_curConn;  //当前使用的连接数
    int m_freeConn; //当前空闲的连接数
    locker lock;
    list<MYSQL*> connList;
    sem reserve;

public:
    string m_url;   //主机地址
    string m_port;  //数据端口号
    string m_user;  //登录数据库用户名
    string m_password;  //登录数据库密码
    string m_dataname;  //使用的数据库名
    int m_close_log;    //日志开关
};

class connectionRAII{
public:
    //双指针对MYSQL *con修改
    connectionRAII(MYSQL **con,sql_connection_pool *connPool);
    ~connectionRAII();
private:
    MYSQL *conRAII;
    sql_connection_pool *poolRAII;
};

#endif // SQL_CONNECTION_POOL_H
