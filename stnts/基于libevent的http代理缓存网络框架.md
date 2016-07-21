# 基于libevent的http代理缓存网络框架

### 基本结构
[libevent](http://libevent.org/)是一个被广泛应用的基于事件驱动的高性能跨平台网络库，缓存网络框架采用多线程，基本设计模型为one loop per thread，即每个线程起一个独立的事件循环，每个请求在单个事件循环中被处理，不会涉及同一个请求在不同线程之间流转的问题，并将缓存资源做成全局共享结构，多线程通过加锁去访问缓存，基本结构如下

![http缓存代理](http://littlewhite.us/pic/stnts/http-proxy-cache-frame.png)

event\_base线程集合里的每个线程都是对等的，它们监听同一个fd，并在自己的事件循环内处理请求，这样，每个event\_base线程可当作单线程程序来实现，并不涉及线程间通信

### 线程处理
每个处理线程采用evhttp接口处理http请求，该接口底层基于bufferevent实现，在最新的2.1.5-beta版中提供了更丰富的回调接口，以下是单个线程的处理流程

![http缓存处理](http://littlewhite.us/pic/stnts/http-cache-process.png)

### 关键技术点
为了实现高性能缓存服务器，在一些细节上需要优化处理，下面列出两个关键点

#### 边下边回

顾名思义，缓存服务向远程服务请求资源时，每读取一部分数据就应该向客户端返回一部分数据，而不是等资源读取完成才返回，libevent-2.1.5版本的evhttp提供了很方便的回调接口，分别是

	// 创建远程服务请求时，注册读取完成后的回调函数
	evhttp_request* evhttp_request_new (void(*cb)(struct evhttp_request *, void *), void *arg)
	
	// 注册读取包头完成后的回调函数
	void evhttp_request_set_header_cb (struct evhttp_request *, int(*cb)(struct evhttp_request *, void *))

	// 注册读取部分body后的回调函数
	void evhttp_request_set_chunked_cb (struct evhttp_request *, void(*cb)(struct evhttp_request *, void *))

有了回调接口，还需要调用回复函数，回复函数如下

	// 回复起始行及包头
	void evhttp_send_reply_start (struct evhttp_request *req, int code, const char *reason)

	// 回复部分body数据，这里不要被chunk给迷惑了，该函数并不一定以chunk编码恢复
	// 只有当回包HTTP版本为1.1且没有Content-Length字段时才会以chunk编码回复
	void evhttp_send_reply_chunk (struct evhttp_request *req, struct evbuffer *databuf)

	// 回复结束
	void evhttp_send_reply_end (struct evhttp_request *req)

由此可见，libevent原生接口即可很好的支持边下边回的需求

#### 避免重复请求
如果多个客户端同时请求相同资源，并且该资源并没有缓存，我们应该只请求一份原始资源，这样可减少带宽消耗，也能更快的回复给客户端

由于我们采用多线程模型，所以这些请求可能被不通的线程处理，而且我们的每个线程都是一个事件循环并采用边下边回的模式，因此这些请求也可能被同一个线程在不同事件循环中处理，这里我们可以通过全局缓存资源解决这个问题

设计缓存管理结构如下

	class CacheMgr
	{
	public:
	    StoreEntry* GetStoreEntry(const std::string& url);
	private:
	    SafeLock lock_;
	};

该模块提供通过URL获取StoreEntry(缓存资源结构）的接口，如果根据URL查找到缓存，则返回对应的StoreEntry结构，如果没有找到，则创建一个StoreEntry资源并返回，同时会将该StoreEntry加入缓存结构中，该接口需要加互斥锁

StoreEntry结构设计如下

	class StoreEntry
	{
	public:
	    void Lock(const ELockType type);
	    void Unlock();

	    MemObj* mem_obj_;
	    StoreStatus status_;
	private:
	    RWLock lock_;
	};

其中MemObj为内存缓存结构，我们暂且不用关心，这里的重点为status\_成员变量和Lock()，Unlock()函数，status_有如下几种状态
	
	enum StoreStatus
	{
	    STORE_INIT = 0,
	    STORE_PENDING,
	    STORE_FINISHED,
	    STORE_ERROR
	};

假设多个线程同时拥有了StoreEntry指针，并且该StoreEntry对象为新建对象，这时通过加锁竞争修改status\_字段，如果status\_为STORE\_INIT，则修改为STORE\_PENDING，此时该线程角色为producer，由该线程去请求远程资源，其它线程发现status\_不为STORE\_INIT，则变成consumer角色，通过定时器注册回调函数读取StoreEntry，producer和consumer对StoreEntry的读写操作都需要加锁，producer下载资源完成后会将status\_标记为STORE\_FINISHED，若下载失败则标记为STORE_ERROR

### evhttp缺点
evhttp的底层请求和回包数据均以evbuffer结构存储，当我们收到回包后，可以将evbuffer结构管理起来，但在给客户端回复时，需要拷贝一份出来填充到客户端连接对应的evbuffer结构里，在单线程模型下，可以采用evbuffer\_add\_buffer\_reference接口，但多线程下使用会导致程序崩溃，每次回复都进行数据拷贝，这或多或少会影响一些性能

### 性能测试
通过http_load工具测试性能，使用方法

	http_load -paralle 10 -fetches 10000 -proxy 192.168.3.57:3333 url_file

	参数说明
	paralle: 请求线程数
	fetches: 请求总数
	proxy: 代理
	url_file: 要请求的url列表文件

我们以nginx作为对比，代理缓存和nginx部署在同一台机器上，代理缓存启用8线程，url_file包含10个url列表（均为nginx服务器资源），对应的资源大小是一样的，在另一台机器上通过http_load执行测试，结果如下

1. 文件大小10B，请求数10W
	
	nginx测试结果，每秒请求为5621
		
		 $ http_load -paralle 10 -fetches 100000  ~/tmp/1                        
		100000 fetches, 10 max parallel, 1.2e+06 bytes, in 17.7891 seconds
		12 mean bytes/connection
		5621.43 fetches/sec, 67457.2 bytes/sec
		msecs/connect: 0.496168 mean, 5.972 max, 0.105 min
		msecs/first-response: 1.23227 mean, 6.748 max, 0.199 min
		HTTP response codes:
		  code 200 -- 100000  

	libevent代理缓存测试结果，每秒请求为4100
	                                                                                                           
		  $ http_load -paralle 10 -fetches 100000 -proxy 192.168.3.57:3333  ~/tmp/1
		100000 fetches, 10 max parallel, 1.2e+06 bytes, in 24.3881 seconds
		12 mean bytes/connection
		4100.35 fetches/sec, 49204.2 bytes/sec
		msecs/connect: 0.546726 mean, 9.7 max, 0.103 min
		msecs/first-response: 1.77949 mean, 10.499 max, 0.404 min
		HTTP response codes:
		  code 200 -- 100000

2. 文件大小为1M，请求数1W

	nginx测试结果，每秒请求为468

		 $ http_load -paralle 10 -fetches 10000  ~/tmp/2
		10000 fetches, 10 max parallel, 1.04858e+10 bytes, in 21.3557 seconds
		1.04858e+06 mean bytes/connection
		468.258 fetches/sec, 4.91004e+08 bytes/sec
		msecs/connect: 0.836883 mean, 4.836 max, 0.129 min
		msecs/first-response: 1.51106 mean, 16.167 max, 0.254 min
		HTTP response codes:
		  code 200 -- 10000

	libevent代理缓存测试结果，每秒请求为351

		 $ http_load -paralle 10 -fetches 10000 -proxy 192.168.3.57:3333  ~/tmp/2 
		10000 fetches, 10 max parallel, 1.04858e+10 bytes, in 28.4885 seconds
		1.04858e+06 mean bytes/connection
		351.019 fetches/sec, 3.6807e+08 bytes/sec
		msecs/connect: 0.860886 mean, 6.685 max, 0.165 min
		msecs/first-response: 4.37781 mean, 200.881 max, 1.174 min
		HTTP response codes:
		  code 200 -- 10000

3. 文件大小为10M，请求1000

	nginx测试结果，每秒请求为70

		 $ http_load -paralle 10 -fetches 1000  ~/tmp/3                          
		1000 fetches, 10 max parallel, 1.04858e+10 bytes, in 14.2722 seconds
		1.04858e+07 mean bytes/connection
		70.0662 fetches/sec, 7.34697e+08 bytes/sec
		msecs/connect: 1.50244 mean, 161.754 max, 0.283 min
		msecs/first-response: 1.8909 mean, 57.902 max, 0.418 min
		HTTP response codes:
		  code 200 -- 1000

	libevent代理缓存测试结果，每秒请求为70

		 $ http_load -paralle 10 -fetches 1000 -proxy 192.168.3.57:3333  ~/tmp/3
		1000 fetches, 10 max parallel, 1.04858e+10 bytes, in 14.1516 seconds
		1.04858e+07 mean bytes/connection
		70.6634 fetches/sec, 7.4096e+08 bytes/sec
		msecs/connect: 1.07996 mean, 28.823 max, 0.181 min
		msecs/first-response: 26.1919 mean, 262.084 max, 10.496 min
		HTTP response codes:
		  code 200 -- 1000

为什么要和nginx进行比较且把代理缓存和nginx部署在一台机器上？因为当代理有了缓存之后，不需要向远程服务器（这里的远程服务器就是nginx）请求资源，它本身就可看作http服务器，nginx作为http服务器的标杆，因此具有一定的参照性

从测试数据可以看出，在请求小文件时，libevent代理缓存响应请求的效率大约为nginx的70%-80%，当请求文件较大时效率相当，因为代理缓存是直接从内存读取数据返回，而nginx对大文件可能没有做缓存优化

以上只是libevent代理缓存demo测试结果，还有很多细节尚可完善
	