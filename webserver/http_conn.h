#ifndef HTTP_CONN_H
#define HTTP_CONN_H
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>
#include <stdio.h>
#include <string>
#include <iostream>
#include "locker.h"
#include "sql_connection_pool.h"
#include <unistd.h>

class http_conn
{
public:
    //设置读取文件的名称m_real_file大小
    static const int FILENAME_LEN=200;
    //设置读缓冲区m_read_buf大小
    static const int READ_BUFFER_SIZE=2048;
    //设置写缓冲区m_write_buf大小
    static const int WRITE_BUFFER_SIZE=1024;
    //报文的请求方法，本项目只用到GET和POST
    enum METHOD
    {
        GET=0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
     //主状态机的状态
    enum CHEAK_STATE
    {
        CHECK_STATE_REQUESTLINE=0,  //当前正在分析请求行
        CHECK_STATE_HEADER,     //当前正在分析头部字段
        CHECK_STATE_CONTENT,

    };
    //报文解析的结果
    enum HTTP_CODE
    {
        NO_REQUEST, //请求不完整，需要继续读取客户数据
        GET_REQUEST,    //获得了一个完整的客户请求
        BAD_REQUEST,    //客户请求有语法错误
        NO_RESOURCE,
        FORBIDDEN_REQUEST,  //表示客户堆资源没有足够的访问权
        FILE_REQUEST,
        INTERNAL_ERROR, //表示服务器内部错误
        CLOSED_CONNECTION   //表示客户端已经关闭连接了
    };
    //从状态机的状态
    enum LINE_STATUS
    {
        LINE_OK=0,  //读取到一个完整的行
        LINE_BAD,   //行出错
        LINE_OPEN   //行数据不完整
    };

public:
    http_conn(){};
    ~http_conn(){};
public:
    //初始化新接受的连接
    void init(int sockfd,const sockaddr_in &addr,char*,int,int,std::string user,std::string passwd,std::string sqlname);
    //关闭连接
    void close_conn(bool real_close=true);
    //处理客户请求
    void process();
    //非阻塞读操作
    bool read_once();
    //非阻塞写操作
    bool write();
    sockaddr_in *get_address(){
        return &m_address;
    }
    void initmysql_result(sql_connection_pool *connPool);
    int timer_flag;
    int improv;

private:
    //初始化连接
    void init();
    //解析http请求
    HTTP_CODE process_read();
    //填充http应答
    bool process_write(HTTP_CODE ret);
    //下面一组数据被process_read调用以分析http的请求
    HTTP_CODE parse_request_line(char* text);
    HTTP_CODE parse_headers(char* text);
    HTTP_CODE parse_content(char* text);
    HTTP_CODE do_request();
    char* get_line(){return m_read_buf+m_start_line;}
    LINE_STATUS parse_line();

    //下面一组数据被process_write调用以填充http应答
    void unmap();
    bool add_response(const char* format,...);
    bool add_content(const char* content);
    bool add_status_line(int status,const char* title);
    bool add_headers(int content_length);
    bool add_content_type();    ////
    bool add_content_length(int content_length);    ////
    bool add_linger();
    bool add_blank_line();
public:
    //所有socket上的事件都被注册到同一个epoll内核事件表中，所以将epoll文件描述符设置为静态的
    static int m_epollfd;
    //统计用户数量
    static int m_user_count;
    MYSQL *mysql;
    int m_state;    //读为0，写为1

private:
    int m_sockfd;
    sockaddr_in m_address;

    //读缓冲区
    char m_read_buf[READ_BUFFER_SIZE];
    //标识读缓冲区中已经读入的客户数据的最后一个字节的下一个位置
    int m_read_idx;
    //当前正在分析的字符在读缓冲区中的位置
    int m_checked_idx;
    //当前正在解析的行的起始位置
    int m_start_line;
    //写缓冲区
    char m_write_buf[WRITE_BUFFER_SIZE];
    //写缓冲区中待发送的字节数
    int m_write_idx;

    //主状态机当前所处的状态
    CHEAK_STATE m_check_state;
    METHOD m_method;    //请求方法

    //以下为解析请求报文中对应的6个变量
    char m_real_file[FILENAME_LEN]; //存储读取文件的名称
    char *m_url;    //客户请求的目标文件的文件名
    char *m_version;    //HTTP协议版本号，仅支持HTTP1.1
    char *m_host;   //主机名
    int m_content_length;   //http请求的消息体长度
    bool m_linger;  //http请求是否需要保持连接

    char *m_file_address;   //读取服务器上的文件地址
    struct stat m_file_stat;    //目标文件的状态，通过它我们可以判断文件是否存在、是否为目录、是否可读，并获取文件大小等信息
    struct iovec m_iv[2];   //io向量机制iovec
    int m_iv_count; //表示写内存快的数量
    int cgi;    //是否启用POST
    char *m_string;     //存储请求头数据
    int bytes_to_send;  //剩余发送字节数
    int bytes_have_send;    //已发送字节数

    char *doc_root;
    std::map<std::string,std::string> m_users;
    int m_TRIGMode;
    int m_close_log;

    char sql_user[100];
    char sql_passwd[100];
    char sql_name[100];
};

#endif // HTTP_CONN_H
