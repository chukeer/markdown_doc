Title: c++实战——多人会话聊天室（二）
Date: 2016-09-06
Modified: 2016-09-06
Category: Language
Tags: c++
Slug: c++实战——多人会话聊天室（二）
Author: littlewhite

前面已经讲过一次多人会话聊天室的实现[C++实战——多人会话聊天室（一）](http://littlewhite.us/archives/109)，只不过上一篇是用最简单的方式，服务端每接收一个连接就起一个线程，而且是阻塞模式的，也就是说服务端每次调用accept函数时会一直等待有客户端连接上才会返回。今天介绍一种基于epoll模型的非阻塞方式的实现。

===
####阻塞与非阻塞
顾名思义，阻塞就是当你调用一个函数后它会一直等在那里，知道某个信号叫醒它，最典型的例子就是read之类的函数，当你调用时它会等待标准输入，直到你在屏幕上输完敲下回车，它才会继续执行。Linux默认IO都是阻塞模型的  
非阻塞就是当你调用函数之后它会立马返回，同样还是拿read举例，它不会阻塞在屏幕上等待你输入，而是立马返回，如果返回错误，那就代表没有数据可读。下面的例子可以大致说明一下差别

	#include <unistd.h>
	#include <stdio.h>
	#include <fcntl.h>
	#include <stdlib.h>

	int main(int argc, char *argv[])
	{
	    int res;
	    res = fcntl(0, F_GETFL);
	    if (-1 == res)
  	    {
  	    	perror("fcntl error!");
  	    	exit(1);
  	    }
	#ifdef NONBLOCK
    	res |= O_NONBLOCK;
    	if (fcntl(0, F_SETFL, res) == -1)
    	{
      	  perror("error");
      	  exit(1);
    	}
	#endif
    	char buf[100];
    	int n = 0;
    	n = read(0, buf, 100);
    	if (-1 == n)
    	{
      	  perror("read error");
      	  exit(1);
    	}
    	else
    	{
      	  printf("read %d characters\n", n);
    	}
    	return 0;
	}
	
代码的意思很好理解，我们从标准输入读取数据，并打印出读取了多少字节，但是我们做了个测试，当定义了宏NONBLOCK后，我们会将标准输入句柄改变成非阻塞的，宏可以通过编译时的-D参数指定，我们分别按如下指令编译，假设文件名为test.cpp

	g++ test.cpp -o test_block
	g++ test.cpp -D NONBLOCK -o test_nonblock
	
然后我们运行./test_block，程序会阻塞在屏幕上等待输入，输入hello world并回车，程序运行结束  
但是当我们运行./test_nonblock时，程序报错*read error: Resource temporarily unavailable*，这是因为此时的标准输入是非阻塞模式，当调用read后它会立马返回，而此时并没有数据可读取，就会返回错误，但是我们按如下方式就可运行成功
	
	echo "hello world" | ./test_nonblock
	
因为在read调用之前，管道里已经有了数据，所以它会去读取管道里的数据而不会出错。

to be continued...
