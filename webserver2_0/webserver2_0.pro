TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        config.cpp \
        http_conn.cpp \
        log.cpp \
        lst_timer.cpp \
        main.cpp \
        sql_connection_pool.cpp \
        webserver.cpp

HEADERS += \
    block_queue.h \
    config.h \
    http_conn.h \
    locker.h \
    log.h \
    lst_timer.h \
    sql_connection_pool.h \
    threadpool.h \
    webserver.h

LIBS += -lpthread -lmysqlclient
