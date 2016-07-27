# HTTPS代理介绍

### HTTPS请求

HTTPS也被成为HTTP over TSL/SSL，是一种网络安全传输协议，我们知道HTTP协议是明文的，可以很容易的写一个HTTP客户端，而HTTPS是将HTTP报文加密之后发送，一个HTTPS会话步骤大致如下

1. 客户端向服务端索要并验证公钥
2. 双方协商生成“对话密钥”
3. 双方采用“对话密钥”进行加密通信

其中前两步称为SSL握手（handshake），第三步就是发送加密后的HTTP报文

一个HTTPS客户端需要实现以上三步，其中SSL握手是一个非常复杂的步骤，现在支持SSL的客户端基本上都采用OpenSSL库来实现，SSL握手步骤参见[http://www.ruanyifeng.com/blog/2014/02/ssl_tls.html](http://www.ruanyifeng.com/blog/2014/02/ssl_tls.html)，这里就不展开了，因为HTTPS代理压根用不到这些

### HTTPS代理请求

HTTPS代理是采用隧道的方式建立客户端和服务端之间的联系，并将所有数据直接在隧道里转发。隧道是客户端通过HTTP的CONNECT方法建立起来，隧道允许用户通过HTTP连接发送非HTTP流量，这里发送的就是加密后的HTTP数据。一个HTTPS代理请求的流程如下

![HTTPS代理请求流程](http://littlewhite.us/pic/stnts/HTTPS-proxy-request.png)

简单描述如下

1. 客户端以CONNECT方式向代理发起请求，CONNECT数据包和普通HTTP数据包类似，第一行包含服务端主机和端口，后面是HTTP包头。注意第一行只有服务端HOST，而没有GET请求里的PATH，因为真正的HTTP请求是在第5步加密发送的
2. 代理读取CONNECT数据包后，向远程服务端443端口建立TCP连接
3. 连接建立成功
4. 代理向客户端返回连接建立成功的报文，也是明文发送
5. 此时客户端会发送真正的HTTPS请求，包括和服务端的SSL握手，加密数据的传输等，此时代理只需将收到的数据直接盲转发即可

总结起来，代理要做的就以下三件事

1. 解析客户端CONNECT请求
2. 和远程服务建立连接并给客户端返回连接建立成功报文
3. 读取客户端和服务端发送的数据，并直接转发给对方

因此代理并不需要向客户端一样实现一套SSL请求，与之相对应的是HTTP/HTTPS网关：由网关（而不是客户端）初始化与远端HTTPS服务器的SSL会话，然后代表客户端执行HTTPS事务。响应会由代理接收并解密，然后通过（不安全的）HTTP传送给客户端

<div style="text-align: center">
<img src="http://littlewhite.us/pic/stnts/HTTP-HTTPS-gateway.jpg"/>
</div>

但这种方式有几个缺点：

* 客户端到网关之间的连接是普通的非安全HTTP
* 尽管网关是已认证主体，但客户端无法对远端服务器执行SSL客户端认证（基于X509证书的认证）
* 网关要支持完整的SSL实现

所以实现一套HTTPS代理方案相对是容易的，它的SSL会话是建立在产生请求的客户端和目的（安全的）Web服务器之间的，中间的代理服务器只是将加密数据经过隧道传输，并不会在安全事务中扮演其他的角色