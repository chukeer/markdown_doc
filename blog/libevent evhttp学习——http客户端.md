## 基本环境
使用版本为libevent-2.1.5，目前为beta版，其中evhttp和旧版区别在于新增了如下接口

```cpp
// 设置回调函数，在包头读取完成后回调
void evhttp_request_set_header_cb (struct evhttp_request *, int(*cb)(struct evhttp_request *, void *))

// 设置回调函数，在body有数据返回后回调
void evhttp_request_set_chunked_cb (struct evhttp_request *, void(*cb)(struct evhttp_request *, void *))
```

这样的好处是可以在合适的时机回调我们注册的回调函数，比如下载1G的文件，在之前的版本只有下载完成后才会回调，现在每下载一部分数据就会回调一次，让上层应用更加灵活，尤其在http代理时，可以做到边下载边回复

2.1.5版本的完整接口文档详见[http://www.wangafu.net/~nickm/libevent-2.1/doxygen/html/http_8h.html](http://www.wangafu.net/~nickm/libevent-2.1/doxygen/html/http_8h.html)

## 请求流程
http客户端使用到的接口函数及请求流程如下

1. 初始化event\_base和evdns\_base

	```cpp
	struct event_base *event_base_new(void);
	struct evdns_base * evdns_base_new(struct event_base *event_base, int initialize_nameservers);
	```
2. 创建evhttp_request对象，并设置回调函数，这里的回调函数是和数据接收相关的

	```cpp
	struct evhttp_request *evhttp_request_new(void (*cb)(struct evhttp_request *, void *), void *arg);
	void evhttp_request_set_header_cb(struct evhttp_request *, int (*cb)(struct evhttp_request *, void *));
	void evhttp_request_set_chunked_cb(struct evhttp_request *, void (*cb)(struct evhttp_request *, void *));
	void evhttp_request_set_error_cb(struct evhttp_request *, void (*)(enum evhttp_request_error, void *));
	```
3. 创建evhttp_connection对象，并设置回调函数，这里的回调函数是和连接状态相关的

	```cpp
	struct evhttp_connection *evhttp_connection_base_new(struct event_base *base, 
	struct evdns_base *dnsbase, const char *address, unsigned short port);
	void evhttp_connection_set_closecb(struct evhttp_connection *evcon,
		void (*)(struct evhttp_connection *, void *), void *);
	```
4. 有选择的向evhttp\_request添加包头字段

	```cpp
	int evhttp_add_header(struct evkeyvalq *headers, const char *key, const char *value);
	```
5. 发送请求
		
	```cpp
	int evhttp_make_request(struct evhttp_connection *evcon,
	    struct evhttp_request *req,
	    enum evhttp_cmd_type type, const char *uri);
	```
6. 派发事件
	
	```cpp
	int event_base_dispatch(struct event_base *);
	```

## 完整代码

```cpp
#include "event2/http.h"
#include "event2/http_struct.h"
#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/dns.h"
#include "event2/thread.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/queue.h>
#include <event.h>

void RemoteReadCallback(struct evhttp_request* remote_rsp, void* arg)
{
    event_base_loopexit((struct event_base*)arg, NULL);
} 

int ReadHeaderDoneCallback(struct evhttp_request* remote_rsp, void* arg)
{
    fprintf(stderr, "< HTTP/1.1 %d %s\n", evhttp_request_get_response_code(remote_rsp), evhttp_request_get_response_code_line(remote_rsp));
    struct evkeyvalq* headers = evhttp_request_get_input_headers(remote_rsp);
    struct evkeyval* header;
    TAILQ_FOREACH(header, headers, next)
    {
        fprintf(stderr, "< %s: %s\n", header->key, header->value);
    }
    fprintf(stderr, "< \n");
    return 0;
}

void ReadChunkCallback(struct evhttp_request* remote_rsp, void* arg)
{
    char buf[4096];
    struct evbuffer* evbuf = evhttp_request_get_input_buffer(remote_rsp);
    int n = 0;
    while ((n = evbuffer_remove(evbuf, buf, 4096)) > 0)
    {
        fwrite(buf, n, 1, stdout);
    }
}

void RemoteRequestErrorCallback(enum evhttp_request_error error, void* arg)
{
    fprintf(stderr, "request failed\n");
    event_base_loopexit((struct event_base*)arg, NULL);
}

void RemoteConnectionCloseCallback(struct evhttp_connection* connection, void* arg)
{
    fprintf(stderr, "remote connection closed\n");
    event_base_loopexit((struct event_base*)arg, NULL);
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("usage:%s url", argv[1]);
        return 1;
    }
    char* url = argv[1];
    struct evhttp_uri* uri = evhttp_uri_parse(url);
    if (!uri)
    {
        fprintf(stderr, "parse url failed!\n");
        return 1;
    }

    struct event_base* base = event_base_new();
    if (!base)
    {
        fprintf(stderr, "create event base failed!\n");
        return 1;
    }

    struct evdns_base* dnsbase = evdns_base_new(base, 1);
    if (!dnsbase)
    {
        fprintf(stderr, "create dns base failed!\n");
    }
    assert(dnsbase);

    struct evhttp_request* request = evhttp_request_new(RemoteReadCallback, base);
    evhttp_request_set_header_cb(request, ReadHeaderDoneCallback);
    evhttp_request_set_chunked_cb(request, ReadChunkCallback);
    evhttp_request_set_error_cb(request, RemoteRequestErrorCallback);

    const char* host = evhttp_uri_get_host(uri);
    if (!host)
    {
        fprintf(stderr, "parse host failed!\n");
        return 1;
    }

    int port = evhttp_uri_get_port(uri);
    if (port < 0) port = 80;

    const char* request_url = url;
    const char* path = evhttp_uri_get_path(uri);
    if (path == NULL || strlen(path) == 0)
    {
        request_url = "/";
    }

    printf("url:%s host:%s port:%d path:%s request_url:%s\n", url, host, port, path, request_url);

    struct evhttp_connection* connection =  evhttp_connection_base_new(base, dnsbase, host, port);
    if (!connection)
    {
        fprintf(stderr, "create evhttp connection failed!\n");
        return 1;
    }

    evhttp_connection_set_closecb(connection, RemoteConnectionCloseCallback, base);

    evhttp_add_header(evhttp_request_get_output_headers(request), "Host", host);
    evhttp_make_request(connection, request, EVHTTP_REQ_GET, request_url);

    event_base_dispatch(base);

    return 0;
}
```

编译
	
	g++ http_client.cpp -I/opt/local/libevent-2.1.5/include -L/opt/local/libevent-2.1.5/lib -levent -g -o http_client

运行示例，这里只打印了包头字段
	
	 $ ./http_client http://www.qq.com >/dev/null
	< HTTP/1.1 200 OK
	< Server: squid/3.4.3
	< Content-Type: text/html; charset=GB2312
	< Cache-Control: max-age=60
	< Expires: Fri, 05 Aug 2016 08:48:31 GMT
	< Date: Fri, 05 Aug 2016 08:47:31 GMT
	< Transfer-Encoding: chunked
	< Connection: keep-alive
	< Connection: Transfer-Encoding
	< 

