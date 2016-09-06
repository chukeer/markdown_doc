Title: libevent evhttp学习——http服务端
Date: 2016-09-06
Modified: 2016-09-06
Category: Language
Tags: c++
Slug: libevent evhttp学习——http服务端
Author: littlewhite

# libevent evhttp学习——http服务端
http服务端相对客户端要简单很多，我们仍旧使用libevent-2.1.5版本，服务端接口和2.0版本没有区别

### 基本流程
http服务端使用到的借口函数及流程如下

1. 创建event_base和evhttp
    
    ```cpp
    struct event_base *event_base_new(void);
    struct evhttp *evhttp_new(struct event_base *base);
    ```
    
2. 绑定地址和端口

    ```cpp
    int evhttp_bind_socket(struct evhttp *http, const char *address, ev_uint16_t port);
    ```
    
3. 设置处理函数

    ```cpp
    void evhttp_set_gencb(struct evhttp *http,
        void (*cb)(struct evhttp_request *, void *), void *arg);
    ```

4. 派发事件循环

    ```cpp
    int event_base_dispatch(struct event_base *);
    ```
    
### 完整代码
服务器接收到请求后打印URL，并返回一段文本信息

```cpp
#include "event2/http.h"
#include "event2/event.h"
#include "event2/buffer.h"

#include <stdlib.h>
#include <stdio.h>

void HttpGenericCallback(struct evhttp_request* request, void* arg)
{
    const struct evhttp_uri* evhttp_uri = evhttp_request_get_evhttp_uri(request);
    char url[8192];
    evhttp_uri_join(const_cast<struct evhttp_uri*>(evhttp_uri), url, 8192);

    printf("accept request url:%s\n", url);

    struct evbuffer* evbuf = evbuffer_new();
    if (!evbuf)
    {
        printf("create evbuffer failed!\n");
        return ;
    }

    evbuffer_add_printf(evbuf, "Server response. Your request url is %s", url);
    evhttp_send_reply(request, HTTP_OK, "OK", evbuf);
    evbuffer_free(evbuf);
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("usage:%s port\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    if (port == 0)
    {
        printf("port error:%s\n", argv[1]);
        return 1;
    }

    struct event_base* base = event_base_new();
    if (!base)
    {
        printf("create event_base failed!\n");
        return 1;
    }

    struct evhttp* http = evhttp_new(base);
    if (!http)
    {
        printf("create evhttp failed!\n");
        return 1;
    }

    if (evhttp_bind_socket(http, "0.0.0.0", port) != 0)
    {
        printf("bind socket failed! port:%d\n", port);
        return 1;
    }

    evhttp_set_gencb(http, HttpGenericCallback, NULL);

    event_base_dispatch(base);
    return 0;
}

```

编译

    g++ http-server.cpp -I/opt/third_party/libevent/include -L/opt/third_party/libevent/lib -levent -o http-server
