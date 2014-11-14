#BitQiu测试梳理
参照刘宽平给的测试报告，梳理了下BitQiu的测试方案，主要罗列了我想到的测试点，具体的方法实现可灵活考虑。这里发出来大家审核一下，如有遗漏的测试要点或测试用例可以提出来：）
<hr>

主要从两个维度来进行测试

* 基本功能测试：这里除了要看正常情况下功能测试，还要看一些边界条件或非正常情况下功能是否出错，如果出错，错误是否符合预期
* 性能测试：跑一些单独的测试用例，比如写一些大文件，统计读写时间，磁盘网络IO等等

可基于gtest跑测试样例，但目前gtest里很多测试用例是相互依赖，比如创建目录、读写文件、查询文件、设置属性等等，也有一些是独立的，比如负载均衡、副本复制、查询节点状态

所以可以考虑将gtest用例分为两类，一类是有互相依赖的，这类测试用例可以一起测试，另外一类是独立的，这类测试用例可以每个单独测试，而且每个测试用例运行前可能需要一些设置，比如进行负载均衡测试前，先上线一个负载很低的节点，再进行负载均衡测试，这样才能体现效果。分类之后，通过gtest样例和外围脚本，可满足目前功能测试和性能测试两种需求

###样例分类
####互相依赖的样例

	BtqClientTest.
		CreatDirFileTest
		ReadFileTest
		QueryFileBaseInfoTest
		QueryFileDetailInfoTest
		QueryDirBaseInfoTest
		QueryDirDetailInfoTest
		QueryDirEnumBaseInfoTest
		QueryDirEnumDetailInfoTest
		FileAttrsTest
		SyncFileTest
  
这部分测试用例在本地内存建立了对照的文件系统，通过本地系统和BitQiu系统对比验证测试用例是否正确，由于每次运行都需要从本地的文件系统获取上下文关系，比如写了一批文件，测试查询文件基本信息时，需要先从本地系统知道文件path_id，所以这批用例可以一起运行，主要目的是检查基本功能是否正确，运行方式如下

	./run_gtest.sh --gtest_filter=BtqClientTest.\*
	
这部分功能相对单一，可用一个报告记录所有结果

####单独运行的样例
  
	BtqClientTestSingle.
		LoadBalanceTest
		CopyTest
		WriteReadTest
		AddRootServerTest
		AddNodeTest
		QueryNodeStateInfo
	
这部分样例都可以单独运行，用例之间没有互相依赖，包括加入节点、查询状态、触发负载均衡、触发副本复制、读写文件，由于读写文件需要有上下文关系，因此做成一个样例，之所以还单独做一个读写文件的用例，主要是为了测试大数据读写的情况，可以通过外围脚本调用更加灵活的使用

比如BtqClientTestSingle.WriteReadTest这个用例，在代码里指定了要写的本地文件是./tmp/test_data.up，可以通过外围脚本先创建文件，然后执行用例

	./run_gtest.sh --gtest_filter=BtqClientTestSingle.WriteReadTest --gtest_repeat=2

该命令会重复执行2次用例，由于上传文件路径在代码里写死了，因此这里是重复上传同一个文件，如果想上传不同文件，可以在外围脚本创建，然后重复调用执行用例，这样提供了更灵活的测试方式

###重点考虑的测试用例
除了前面提到的基本功能用例以外，还需要考虑一些异常情况的测试或者组合测试，基本功能用现有的gtest程序完全可以胜任，下面列举一些其它复杂情况的测试用例，需要用外围脚本配合

1. 副本复制
	* 测试方法：修改redis元数据提高副本数，执行BtqClientTestSingle.CopyTest用例用例，通过日志解析出执行时间、成功失败次数，读取redis元数据对比是否符合预期，并和storeNode磁盘数据比较是否一致
	* 测试观察点：  
	<table border="1">
	<tr align="center">
	<th>storeNode数量</th>
	<th>复制page数</th>
	<th>复制数据size</th>
	<th>复制完成时间</th>
	<th>元数据是否符合预期且和磁盘数据一致</th>
	<th>Master CPU</th>
	<th>StoreNode磁盘IO</th>
	<th>StoreNode网络IO</th>
	</tr>
	</table>                                  
	
2. 负载均衡
	* 测试方法：（1）启动若干StoreNode，执行BtqClientTestSingle.WriteReadTest用例写入一些数据；（2）启动一个负载低的StoreNode，执行BtqClientTestSingle.LoadBalanceTest用例
	* 测试观察点：
	<table border="1">
	<tr align="center">
	<th>storeNode数量</th>
	<th>移动page数</th>
	<th>移动数据size</th>
	<th>移动完成时间</th>
	<th>成功和失败的消息数</th>
	<th>元数据是否符合预期且和磁盘数据一致</th>
	<th>Master CPU</th>
	<th>StoreNode磁盘IO</th>
	<th>StoreNode网络IO</th>
	</tr>
	</table> 
3. 节点退出
	* 测试方法：（1）kill若干StoreNode；（2）执行相关受影响的用例，看是否符合预期，这里包含一系列用例，比如QueryNodeStateInfo的测试、副本复制的测试等等
	* 测试观察点，以QueryNodeStateInfo为例
	<table border="1">
	<tr align="center">
	<th>退出节点数</th>
	<th>退出前查询的StoreNode在线数</th>
	<th>退出后查询的StoreNode在线数</th>
	</tr>
	</table>
4. 读写功能测试
	* 测试方法：（1）启动若干客户端，每个客户端若干线程，将本地文件写入BitQiu再读回本地，比较读写数据是否一致
	* 测试观察点：
	<table border="1">
	<tr align="center">
	<th>storeNode数量</th>
	<th>写入数据量</th>
	<th>sharding_size</th>
	<th>replicas</th>
	<th>写数据耗时</th>
	<th>写数据速度</th>
	<th>读数据耗时</th>
	<th>读数据速度</th>
	<th>写之前StoreNode磁盘利用率分布</th>
	<th>写之后StoreNode磁盘利用率分布</th>
	</tr>
	</table>
5. 服务器性能测试
	* 测试方法：（1）修改服务器相关参数，比如线程数、心跳上报间隔，在不同参数下编译不同程序；（2）修改客户端线程数，调整客户端并发数量，同时往服务器写数据
	* 测试观察点
	<table border="1">
	<tr align="center">
	<th>Master服务线程数</th>
	<th>storeNode数量</th>
	<th>StoreNode服务线程数</th>
	<th>客户端并发数量</th>
	<th>客户端线程数</th>
	<th>平均读速度</th>
	<th>平均写速度</th>
	<th>消息来回平均耗时</th>
	<th>Master CPU</th>
	<th>StoreNode CPU</th>
	<th>StoreNode磁盘IO</th>
	<th>StoreNode网络IO</th>
	</tr>
	</table>
	
	
所有用例尽量通过日志方式记录结果，然后用脚本解析日志自动生成测试报告，可按时间分类保存为HTML或邮件发送。每个测试用例需开发单独的控制脚本，尽量减少手工操作，前期工作量可能会大一些，但是自动化脚本可以提高重复性工作效率
