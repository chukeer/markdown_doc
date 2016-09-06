Title: c++实战——多人会话聊天室
Date: 2016-09-06
Modified: 2016-09-06
Category: Language
Tags: c++
Slug: c++实战——多人会话聊天室
Author: littlewhite

[TOC]

>无他，但手熟尔  
 ——《卖油翁》

任何一门编程语言，要想熟练，唯有多练。即便是读了一千本小说，若不自己写文章，也成不了作家，编程技术更是需要日复一日反复练习，我将自己学习C++过程中的练习经历与大家分享，如果能给其他初学者以帮助，那是最让人感到欣慰的，当然，本人也是初学者，不足之处难免，望高手多多指教   

###功能概述
===
多人会话聊天室的练习起源于学习多线程和socket编程，服务端和客户端的大致功能如下：  

* 服务端进程代表一个聊天室，响应多个客户端的请求，客户端以用户名登陆之后，可以发表消息，服务端将消息推送给所有登陆的用户，类似于QQ的讨论组一样的功能
* 客户端向服务端发起连接，通过特定指令登陆，登陆之后可发表消息。客户端支持的指令包扩
	* login name, 以name为用户名登陆
	* look, 查看当前登陆的所有用户
	* logout, 退出当前用户
	* quit, 退出客户端  

本文所有代码在Ubuntu和Mac OS X上编译运行通过，如果是windows用户不保证能编得过。  
  
**项目地址**
[https://github.com/handy1989/chatserver/tree/version1.0.1](https://github.com/handy1989/chatserver/tree/version1.0.1)

###实践分析
===
由于在实践过程中我只保留了最终可运行的版本，所以我这里只给出最终版本的源代码，一些中间状态的代码只会分析一下逻辑。毕竟任何程序都不是一开始就完成所有功能，我也是先从简单功能开始实现，然后一点一点添加。

####小试牛刀
先看一下最简单的服务器客户端程序逻辑，这种源代码Google一下到处都是  
**服务端**  
1. 调用socket函数建立一个连接，返回一个文件描述符sockfd   
2. 调用bind函数将sockfd和服务器地址绑定  
3. 调用listen函数，使sockfd可以接受其它连接  
4. 调用accept函数，接受客户端的连接，返回这个连接的文件描述符connfd  
5. 调用send函数向connfd发送消息  
6. 调用recv函数从connfd接受消息  

**客户端**  
客户端的逻辑就简单多了  
1. 调用socket得到sockfd  
2. 调用connect和sockfd建立连接  
3. 调用send向sockfd发送消息  
4. 调用recv从sockfd接受消息  

以上函数具体用法在Linux下都可以通过man手册查到（比如man socket），虽然英文的阅读效率低一点，但绝对是最权威的  

这里需要说明一下文件描述符的概念，在Linux下，所有设备、网络套接字、目录、文件，都以file的概念来对待，打开一个对象就会返回一个文件描述符，通过文件描述符就可以实际的去操作对象，比如read, write, close等。其中最典型的文件描述符就是0、1、2，分别代表标准输入、标准输出、标准错误 。  

从上面可以看出服务器端和客户端的区别，服务端先创建一个文件描述符sockfd，这个是负责接受客户端的连接请求的，当客户端请求成功后服务端会得到这个连接的一个专属文件描述符connfd，如果有多个客户端，那么这多个客户端的connfd都是不同的，服务端和客户端的消息读写都是通过connfd进行，而客户端和服务端的消息读写都是通过sockfd进行。

####客户端实现
客户端逻辑很简单，先建立连接，然后收发消息。但要注意一点，客户端的收发消息并不是同步的，也就是说并不是发一条就收一条，由于是多人会话，即便你不发消息，也有可能收到别人的消息，所以这里需要将收消息和发消息分离，这里我们用多线程来实现，一个线程专门负责接收消息，并将消息打印到屏幕上，一个线程专门读取标准输入，将消息发送出去。 

客户端代码见[https://github.com/handy1989/chatserver/blob/version1.0.1/client.cpp](https://github.com/handy1989/chatserver/blob/version1.0.1/client.cpp)

####服务端实现
根据文章开始给出的服务端功能，我们画出服务端的处理流程图  
![chatserver](http://littlewhite.us/pic/chatserver.jpg)   
我们在服务端对每个连接建立一个线程，由线程来单独管理和客户端的通信，线程里的处理逻辑就和最简单的服务器客户端模型一样，先接收客户端消息，再给客户端返回信息，不过由于是多人会话，每个客户端发表一条消息，服务端需要给其它所有用户推送消息，这就需要服务端记录登陆进来的所有用户，为了简化，我没有设置密码，并且每次服务端重启后，所有用户信息清零  
服务端的数据结构见文件[https://github.com/handy1989/chatserver/blob/version1.0.1/chatserver.h](https://github.com/handy1989/chatserver/blob/version1.0.1/chatserver.h)，下面分别说明几个关键变量  

	std::map<int, std::string> m_users;
	std::set<std::string> s_users;
	
m_users用来存储和客户端连接的文件描述符与用户名的对应关系，s_users存储的是所有登陆的用户，也就是m_users的value的集合，为很么还要设置一个s_users呢，因为每次用户登陆的时候需要查找用户是否已注册，而m_users是以文件描述符为key的，查value是否存在不太好操作，于是就将value单独存储起来，便于查找  

	int connfd_arr[MAX_THREAD_NUM];
	
connfd_arr存储的时当前连接的文件描述符，设置了一个最大连接数，当有用户连接时，如果连接数超过了最大值，服务端将不会建立线程去通信，否则，服务端会从数组里找一个未被占用的分配给该连接，当线程退出时，数组对应的值会置为-1

	typedef void (ChatServer::*p_func)(char *arg, bool &is_logged, int connfd, ChatServer *p_session, std::string &user_name);
	std::map<std::string, p_func> m_func;
	
由于服务端需要根据用户输入的消息来调用相应处理函数，比如login name对应的登录函数，look对应查看用户的函数，所以服务端需要根据字符串去调用一个函数，最简单的实现就是写若干个if语句一一比较，但我们用了一种更优雅的方式，首先我们将所有处理函数定义成一样的类型，也就是参数和返回值都一样，然后定义一个map型变量，key为命令的关键字，如“login”"logout"等等，value就是对应的处理函数的地址，这样我们接收到客户端的消息后，解析出是哪种命令，然后直接查找map得到函数地址，就可以调用对应函数了

服务端和客户端完整代码见[https://github.com/handy1989/chatserver/tree/version1.0.1](https://github.com/handy1989/chatserver/tree/version1.0.1)，客户端运行之后的效果如下  
![](http://littlewhite.us/pic/client.tiff)  
这里只是演示了一个用户登录的情况，感兴趣的可以多个客户端同时连接看看效果  
###小结
===
本文实现了多人会话的基本功能，服务端通过线程与客户端建立连接，并且自己管理线程，为了简单，线程同步等因素都没有考虑进去。这样做只是为了能尽快对网络通信有个感性的认识，咱又不是想把它做成产品，能运行起来就是最终目的。但是明显的缺陷也摆在这里，比如多线程的管理，accept的阻塞等等，下次将会分享一个基于epoll模型的多人会话聊天室，有了epoll的管理，服务端的代码将会变得清晰而又简洁

