#bridge节点逻辑 && 消息发送处理流程

###bridge节点具备功能

* 向master发送心跳包
* 接收其它节点的数据包，并转发给目的节点

###bridge节点实现
####心跳发送
和store_node一样，通过调用ReportNodeStateInfo发送心跳消息，调用关系如下  

	ReportNodeStateInfo -> SendMsgToRoot -> MessageTransport -> Transport  

其中Transport之前的API都是以消息类为参数，到了Transport这一层则以NetPackage结构作为参数，这一层和消息无关

心跳发送是bridge作为客户端的形态体现之一，它依赖上面提到的调用关系中的每一层

####数据转发
数据转发与消息类型无关，需要处理的是NetPackage类型的数据，此时bridge是以服务器的形态存在，这种形态可以参考store_node和master_root的服务器模型，以store_node为例，服务端的调用关系为

1. 收到数据，类型为NetPackage，在BtqMessageServer中通过一个PackageHandler类进行处理
2. 在PackageHandler中对输入数据进行解包，创建Req消息并填充数据
3. 根据消息类型，调用对应的消息的Handler函数，得到Rsp包
4. 对Rsp包进行解析，并转成NetPackage结构的回包

由于bridge节点对数据的处理与消息类型无关，因此服务端处理流程大大简化，只需两步

1. 收到数据，类型为NetPackage，在BtqMessageServer中通过一个PackageHandler类进行处理
2. 调用Transport将NetPackage数据转发给目的节点，拿到回包数据返回给发送节点

这里调用的Tranport其实是作为客户端在调用，和发送心跳消息流程里的Transport是一样的

###包含bridge节点的消息发送流程
以WritePage为例（假设需要Bridge节点）

1. 调用WritePage接口，其中SendMsgParam中的bridge_address填写bridge节点的地址，use_bridge为true
2. SendMsgToNode -> MessageTransport -> Transport

这和没有bridge节点的区别主要有以下几点：

1. 包装消息发送参数的结构体原来为

		struct SendMsgParam
		{
    		boost::shared_ptr<AccessTokenManager> token_manager;
    		NodeId node_id;
    		NetAddressInfo net_address;
		};
		
	改为
		
		struct SendMsgParam
		{
    		SendMsgParam()
    		{
        		use_bridge = false;
    		}
    		boost::shared_ptr<AccessTokenManager> token_manager;
    		NodeId node_id;
    		NetAddressInfo net_address;
    		NetAddressInfo bridge_address;
    		bool use_bridge;
		};
	增加了两个变量，如果需要Bridge节点，则填上地址并置use_bridge为true即可，发送接口保持不变
2. 在WritePage接口中，需要判断是否用到Bridge，如果用到，则创建BridgeHeader并设置到REQ消息中，并将发送地址设为Bridge地址

除此之外，发送节点的发送方式和目的节点的处理方式均不变

###消息发送处理流程
![](/Users/zhangmenghan/Desktop/消息发送及处理流程2.jpg)