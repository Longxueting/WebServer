#ifndef CONFIG_H
#define CONFIG_H
#include "webserver.h"
using namespace std;
class config
{
public:
    config();
    ~config(){};

    void parse_arg(int argc,char*argv[]);
    int PORT;   //端口号
    int LOGWrite;   //日志写入方式
    int TRIGMode;   //触发组合模式
    int LISTENTrigmode; //listenfd触发模式
    int CONNTrigmode;   //connfd的触发方式
    int OPT_LINGER; //优雅关闭连接
    int sql_num;    //数据库连接池的数量
    int thread_num; //线程池内的线程数量
    int close_log;  //是否关闭日志
    int actor_model;    //并发模型的选择
};

#endif // CONFIG_H
