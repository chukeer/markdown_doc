#BitQiu测试问题汇总

###1、StoreNode和MasterRoot出core

####问题描述  
client读写文件时，StoreNode或MasteRoot出core，当sharding_size为默认1M时，出core概率较小，跑几天才会出现一次，当sharding_size较大，比如为16M时，出core概率增大，可在几分钟之内重现
####core信息  
	(gdb) bt
	#0  0x0000000000000000 in ?? ()
	#1  0x00007fbf17e41acb in 	stnts::bitqiu::network::ConnectionPool::Find (this=0xd53a90, net_address=...) at ../../../network/client/connection_pool.cpp:197
	#2  0x00007fbf17e41696 in stnts::bitqiu::network::ConnectionPool::CreateConnectionInternal (this=0xd53a90, net_address=..., cached=true)
    at ../../../network/client/connection_pool.cpp:136
	#3  0x00007fbf17e4127c in stnts::bitqiu::network::ConnectionPool::CreateConnection (this=0xd53a90, net_address=..., cached=true)
    at ../../../network/client/connection_pool.cpp:46
	#4  0x00007fbf17e4b505 in stnts::bitqiu::network::PackageTransporter::Create (this=0x7fbec8004890, net_address=..., cached=true)
    at ../../../network/client/package_transporter.cpp:19
	#5  0x00000000004b3d8e in stnts::bitqiu::message::MessageTransporter::Create (this=0x7fbf1856e830, net_address=..., from_pool=true)
    at ../../message/framework/btq_message_transporter.cpp:20
	#6  0x00000000004b44c9 in stnts::bitqiu::message::MessageTransporter::MessageTransport (net_address=..., from_pool=true, req_base=..., rsp_base=..., conn_pool=...)
    at ../../message/framework/btq_message_transporter.cpp:75
	#7  0x00000000004b7d01 in stnts::bitqiu::message::EncryptAndTransfer (msg_param=..., req_base=..., rsp_base=...) at ../../message/btq_message_send/message_send.cpp:32
	#8  0x00000000004b803c in stnts::bitqiu::message::SendMessage (msg_param=..., req=..., rsp=...) at ../../message/btq_message_send/message_send.cpp:51
	#9  0x00000000004b82b8 in stnts::bitqiu::message::SendMsgToRoot (conn_pool=..., token_manager=..., dest_net_address=..., req_base=..., rsp_base=...)
    at ../../message/btq_message_send/message_send.cpp:78
	#10 0x000000000047134f in stnts::bitqiu::root::ReportNodeStateInfo (param=..., state_info=...) at ../../nodes/rootserver/message_export_root.cpp:101
	#11 0x0000000000488f76 in stnts::bitqiu::root::SyncNodeStateHandler::doIt (this=0xd50770) at ../../nodes/rootserver/sync_node_state_handler.cpp:60
	#12 0x000000000049d012 in ThreadPool::threadProc (para=0xd50770) at ../../common/thread_pool/thread_pool.cpp:68
	#13 0x00007fbf16c449d1 in start_thread () from /lib64/libpthread.so.0
	#14 0x00007fbf150bcb5d in clone () from /lib64/libc.so.6

####问题原因
枚举出的过期连接没有从active_connections中移除（从busy_connections中移除），导致下次还能从active_connections中获取到已经销毁的Connection连接对象，并且继续使用导致crash

###2、节点管理判断节点存储是否满足
####问题描述
选举节点时判断存储空间是否满足时，比较方式不对，为之前移动节点选举时遗留的，代码拷贝过来时忘了修改
####问题解决
将1464行
  
	return load_after_write < MAX_DISK_LOAD load < global_load + 10? true : false;
改为

	return load_after_write < MAX_DISK_LOAD ? true : false;

###3、CreateFile超时
####问题描述
假设创建一个sharding_size为1M，大小为1G的文件，MasterRoot需要发送1024个CreatePage消息，目前采用串行发送，若总时间超过连接池的最大发送和接收时间，则发送CreateFile的客户端会返回失败
####目前采取解决方案
将连接池最大发送和接收时间从30秒提升到3分钟，该值根据经验设置
####优化思路
MasterRoot在选举出page后，将发送给同一个StoreNode的CreatePage消息组成CreatePageList消息，这样串行消息数最多为StoreNode的数量，在此基础上还可以设置多线程发送，可以大大提高CreateFile的处理时间

###4、RootServer重启时写文件会失败
####问题描述
RootServer重启后由于storeNode可能未上报心跳，这时写文件时可能无法选举

###5、RootServer重启时StoreNode上报心跳失败
####问题描述
RootServer重启时，storeNode由于缓存连接池里的连接，上报会导致失败，目前无法自动恢复

###6、StoreNode重启后客户端写文件失败
客户端多次写失败后又可写成功，可能是RootServer连接池缓存，客户端写多次后将失败的连接冲走

###7、读文件时，返回NodeId和client不在一个集群或room
QueryPageStoreNode处理时，没有过滤掉和client不在一个集群或room的节点，在去掉Bridge后，client是无法和这样的节点通信的
####解决方案
修改GetPageStoreNodeList，当option带opt_balance_r或opt_balance_w参数时，过滤掉和client不在一个集群或room的节点
###8、sharding_size设为64M时写文件失败
原因：  
反序列化失败
###9、delete file handler函数没有设置状态码
	