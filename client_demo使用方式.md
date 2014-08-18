###client demo使用方式

#####环境目录
/home/zhangmenghan/svn_code/BitQiu_Core/Source/build

####停止服务
sh run.sh stop  
停止之前先查看服务是否启动  `ps aux | gerp Server`  
如果有UID为500的rootServer和storeServer进程则表示服务已启动

####启动服务
sh run.sh start 2  
2代表启动2个storeServer，如果要启动多个storeServer，需要手动配置一些client的配置，测试时暂时只使用2个storeServer

####启动client
workroot： /home/zhangmenghan/svn_code/BitQiu_Core/Source/build/client  
可用脚本运行或直接二进制运行，如：

* sh run.sh mkdir "/" (会在屏幕打印日志信息，并在log目录下生成日志文件)
* ./client mkdir "/" （没有日志信息）


支持命令（以不带日志的方式运行）：

* 创建目录  
	./client mkdir "/"  
	注意：  
	1、路径必须用引号包括，否则shell可能会解释出错，这是Linux限制的  
	2、该命令意思为在bitqiu的/目录下创建一个目录，如果创建成功，屏幕会打印创建的目录路径，比如/<1163>
* 上传文件  
	./client put ./local_file "/<1163>"  
	注意：  
	1、该命令意思为将本地的local_file上传到bitqiu的/<1163>的目录下，暂时不支持指定文件名  
	2、如果上传成功，系统会返回文件路径，比如/<1163>/<1164>
* 下载文件  
	./client get "/<1163>/<1164>" ./local_file2  
	注意：  
	1、该命令意思为将bitqiu系统的文件/<1163>/<1164>下载到本地的local_file2中  
	2、比较local_file和local_file2的md5，如果一致证明系统运行正常

