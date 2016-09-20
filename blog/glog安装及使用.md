Title: glog安装及使用
Date: 2016-09-20
Modified: 2016-09-20
Category: Skill
Tags: glog
Slug: glog安装及使用
Author: littlewhite

[TOC]

glog是Google的开源日志系统，使用简单，配置灵活，适合大小型程序的开发。本文介绍Linux平台的安装和使用

## 安装
推荐从源码安装，下载地址[https://code.google.com/p/google-glog/downloads/list](https://code.google.com/p/google-glog/downloads/list)

下载解压后进入目录，和所有Linux程序的安装步骤一样

    ./configuer
    make
    make install
    
如果没有权限请在命令前加上sudo，如果想安装在指定目录，使用./configuer --prefix=your_dir

## 使用
glog的使用简单到令人发指，现有测试程序test.cpp如下

    #include <glog/logging.h>
    int main(int argc, char* argv[])
    {
       google::InitGoogleLogging(argv[0]);
       LOG(INFO) << "info: hello world!";
       LOG(WARNING) << "warning: hello world!";
       LOG(ERROR) << "error: hello world!";
       VLOG(0) << "vlog0: hello world!";
       VLOG(1) << "vlog1: hello world!";
       VLOG(2) << "vlog2: hello world!";
       VLOG(3) << "vlog3: hello world!";
       DLOG(INFO) << "DLOG: hello world!";
       return 0;
    }
    
假设你的glog库的路径为/usr/local/lib/libglog.a，头文件路径为/usr/local/include/glog/logging.h，那么编译命令如下

    g++ test.cpp -o test -L/usr/local/lib -lglog -I/usr/local/include/glog
    
看看是不是将日志打印到屏幕上了，但是你会发现有些日志没有打印出来。接下来我来一一解释

### 日志级别
使用日志必须了解日志级别的概念，说白了就是将日志信息按照严重程度进行分类，glog提供四种日志级别：INFO, WARNING, ERROR, FATAL。它们对应的日志级别整数分别为0、1、2、3， 每个级别的日志对应一个日志文件，其中高级别的日志也会出现在低级别的日志文件中，也就是说FATAL日志会出现在INFO、WARNING、ERROR对应的日志文件中。**FATAL日志会终止程序**，没事别乱用

### 日志文件

glog的日志文件默认是保存在/tmp目录下的，当然你可以指定日志路路径和日志名称

指定日志文件名字

    google::InitGoogleLogging(argv[0])
    
你也可以指定其它字符串，比如本例指定的名字为test，那么日志文件就是test.INFO, test.WARNING这样的格式

### 指定参数
glog可以采用命令行的模式配置参数，这也是它灵活易用的体现，有两种指定参数的方法，一种依赖于gflag如下：

    ./your_application --logtostderr=1
    
或者通过环境变量指定：

    GLOG_logtostderr=1 ./your_application
    
所有的环境变量均以GLOG_开头，我们推荐使用第二种，一来不必依赖于gflag，二来当参数很多时，可以写成脚本的形式，看起来更直观，GLOG支持的flag如下（只列出常用的，如果想看全部的，可以在源码的logging.cc文件下看到）：

环境变量 | 说明
---: | :---
GLOG_logtostderr | bool，默认为FALSE，将日志打印到标准错误，而不是日志文件
GLOG_alsologtostderr | bool，默认为FALSE，将日志打印到日志文件，同时也打印到标准错误
GLOG_stderrthreshold | int，默认为2（ERROR），大于等于这个级别的日志才打印到标准错误，当指定这个参数时，GLOG_alsologtostderr参数将会失效
GLOG_minloglevel | int，默认为0（INFO）， 小于这个日志级别的将不会打印
GLOG\_log\_dir | string类型，指定日志输出目录，目录必须存在
GLOG\_max\_log\_size | int，指定日志文件最大size，超过会被切割，单位为MB
GLOG\_stop\_logging\_if\_full\_disk | bool，默认为FALSE，当磁盘满了之后不再打印日志
GLOG_v | int，默认为0，指定GLOG_v=n时，对vlog(m)，当m<=n时才会打印日志

知道了这些参数之后，我们可以在脚本中指定这些变量，还是以test程序为例，test.sh如下：

    #!/bin/sh
    export GLOG_log_dir=log
    export GLOG_minloglevel=1
    export GLOG_stderrthreshold=1
    export GLOG_v=3
    export GLOG_max_log_size=1
    ./test

执行脚本sh test.sh即可。这样看上去就非常清晰，修改起来也方便

### 打印日志

#### 普通模式

    // 打印日志为INFO级别
    LOG(INFO) << "info: hello world!";
    
    //满足num_cookies > 10时，打印日志
    LOG_IF(INFO, num_cookies > 10) << "Got lots of cookies";
    
    // 当日志语句被执行的第1次、11次、21次...时打印日志，其中google::COUNTER代表的是被执行的次数
    LOG_EVERY_N(INFO, 10) << "Got the " << google::COUNTER << "th cookie";
    
    // 以上两者的组合
    LOG_IF_EVERY_N(INFO, (size > 1024), 10) << "Got the " << google::COUNTER << "th big cookie";

    // 前20次执行的时候打印日志
    LOG_FIRST_N(INFO, 20) << "Got the " << google::COUNTER << "th cookie";


#### debug模式
debug模式的语句如下

    DLOG(INFO) << "Found cookies";
    DLOG_IF(INFO, num_cookies > 10) << "Got lots of cookies";
    DLOG_EVERY_N(INFO, 10) << "Got the " << google::COUNTER << "th cookie";

当编译的时候指定-D NDEBUG时，debug日志不会被输出，比如test.cpp的编译命令改为

    g++ test.cpp -o test -L/usr/local/lib -lglog -I/usr/local/include/glog -D NDEBUG

那么就不会有debug日志输出

#### check模式
    CHECK( fd != NULL ) << " fd is NULL, can not be used ! ";
    
当check的条件不成立时，程序打印完日志之后直接退出，其它命令包括CHECK_EQ, CHECK_NE, CHECK_LE, CHECK_LT, CHECK_GE, CHECK_GT。以CHECK_EQ为例，适用方式如下

    CHECK_NE(1, 2) << ": The world must be ending!";
    
#### 自定义日志级别
    VLOG(1) << "vlog1: hello world!";
    VLOG(2) << "vlog2: hello world!";
    
这个是独立于默认日志级别的，可以配合GLOG_v参数适用

## 总结
GLOG使用就是如此方便，你只需要在代码里指定日志文件名，然后就可以放心的在代码里添加日志而不需要管那些初始化和销毁的操作，其它都可以以命令行的方式来配置，简单灵活，而且基本功能也比较齐全。另外，如果想了解GLOG详细适用，可以参考官方文档[http://google-glog.googlecode.com/svn/trunk/doc/glog.html](http://google-glog.googlecode.com/svn/trunk/doc/glog.html)




 
 





