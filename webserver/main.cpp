#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include "config.h"
using namespace std;

int main(int argc,char* argv[])
{
    //需要修改的数据库信息，登录名，密码，数据库名字
    string user="root";
    string passwd="";
    string databasename="webdb";

    //命令解析
    config configs;
    configs.parse_arg(argc,argv);

    webserver server;
    server.init(configs.PORT,user,passwd,databasename,configs.LOGWrite,configs.OPT_LINGER,configs.TRIGMode,configs.sql_num,configs.thread_num,configs.close_log,configs.actor_model);

    //日志
    //server.log_write();
    //数据库
    server.sql_pool();
    //线程池
    server.thread_pool();
    //触发模式
    server.trig_mode();
    //监听
    server.eventListen();
    //运行
    server.eventLoop();

    return 0;
}
