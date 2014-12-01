#BitQiu压力测试方法及结果说明
##StoreServer测试
###测试方法
该测试主要针对StoreServer进行，测试方法如下

* 客户端数量：1
* 生成1G本地文件，循环执行如下操作
	* 设置sharding_size为1M（一次写一个完整page，没有对page进行分片），客户端多线程写入page，线程数为10，该文件写入4次
	* 分别设置sharding_size为2、4、8、16、32M，重复上一步操作
	* 把写入的数据依次读回来（一共是24个文件共24G），每个文件分别读4次，每读一个文件，比较MD5值和源文件MD5值，若不同则返回失败
	* 把写入的数据全部删除
	
这里读写page的并发线程数设置为10，是因为在进行单独的读写测试时发现客户端在这个参数下读写速度能达到较大值，同时每个消息响应时间不会太长。客户端数量设置为1，是因为服务器响应速度有限，在单独进行读写测试时发现，设置两个相同配置的客户端，每个消息的响应速度为只有一个客户端情况下的一半，所以一个客户端就够了

###观察指标  
[http://192.168.3.6/btq_test/pressure_test/storeserver.html](http://192.168.3.6/btq_test/pressure_test/storeserver.html)

在客户端读写过程中，观察服务器系统指标，包括CPU、内存、磁盘IO，其中<font color="red">**网络IO在测试环境中数据采用不准，请忽略该项指标**</font>
###指标说明
####内存
最大内存消耗接近1G，是因为读写文件时sharding_size最大设置为32M，且没有分片，测试完成后服务器内存消耗恢复到平常水平
####CPU
由于是多核CPU，因此测试时CPU消耗会超过100%。在单独进行写测试时发现，随着sharding_size的增大，StoreServer的CPU消耗也会变大，猜测是因为StoreServer要进行数据解密以及反序列化计算占用CPU时间较长导致
####DISK_RIO
读磁盘IO，由于测试时间跨度较长，所以平均值都很小，可以参考最大值。这里读请求主要集中在SN4和SN5两台机器，原因有：（1）这两台机器cluster_id和客户端配置相同（2）读文件查询page信息时，对每个page所在的节点列表在网络距离一样的情况下并没有做随机打乱，导致每次都集中在某几台机器上，该问题已修复
####DISK_WIO
写磁盘IO，同理，由于时间跨度较大，参考最大值即可，可以看到每台节点的最大磁盘写IO比较接近，没有异常情况
###结论
在大量读写情况下，StoreServer表现稳定，StoreServer的性能和服务程序线程配置以及机器配置有关，这里主要考察在长时间大量读写时是否有逻辑处理错误以及更严重的崩溃情况，目前看来一切还算正常

##RootServer测试
###测试方法
该测试主要针对RootServer进行，方法如下  

* 客户端数量：1
* 使用10个线程，每个线程循环执行如下操作
	* 创建一个目录
	* 写入一个1M以内的小文件
	* 进行1000次查询相关操作，每次随机进行如下一项
		* Set文件属性和Get文件属性，set值和get值必须一致
		* QueryFileBaseInfo
		* QueryFileDetailInfo
		* QueryDirEnumBaseInfo
		* QueryDirEnumDetailInfo
	* 删除创建的目录

除了写文件以为，其它操作都只需要和RootServer通信，让写入文件尽量小，是为了保证和RootServer的通信尽量不中断太长时间
###观察指标
[http://192.168.3.6/btq_test/pressure_test/rootserver.html](http://192.168.3.6/btq_test/pressure_test/rootserver.html)

重点关注RootServer内存和CPU，<font color="red">**NET相关信息请忽略**</font>

###指标说明
####内存
由于客户端主要是发送查询消息，因此RootServer内存使用并不大
####CPU
CPU占用相对平常较高，但基本上维持在90%左右，最大值出现在结尾处，只持续了几秒钟，根据采样数据制作的变化图如下，采样间隔为1秒
![RootSever CPU占用折线图](http://192.168.3.6/btq_test/pic/RootServer_cpu.png)
###结论
RootServer在面对大量查询量时表现比较稳定，但CPU利用率不高，原因可能是消息网络传输以及和Redis通信使得RootServer是IO密集型服务，而非CPU密集型。测试过程中除了最后RootServer因日志打印过多占满磁盘导致无法处理消息，其它时候均能正常处理
