Title: Linux文件读写机制及优化方式
Date: 2016-09-19
Modified: 2016-09-19
Category: Skill
Tags: linux
Slug: Linux文件读写机制及优化方式
Author: littlewhite

本文只讨论Linux下文件的读写机制，不涉及不同读取方式如read,fread,cin等的对比，这些读取方式本质上都是调用系统api read，只是做了不同封装。以下所有测试均使用open, read, write这一套系统api

## 缓存
缓存是用来减少高速设备访问低速设备所需平均时间的组件，文件读写涉及到计算机内存和磁盘，内存操作速度远远大于磁盘，如果每次调用read,write都去直接操作磁盘，一方面速度会被限制，一方面也会降低磁盘使用寿命，因此不管是对磁盘的读操作还是写操作，操作系统都会将数据缓存起来

### Page Cache
页缓存（Page Cache）是位于内存和文件之间的缓冲区，它实际上也是一块内存区域，所有的文件IO（包括网络文件）都是直接和页缓存交互，操作系统通过一系列的数据结构，比如inode, address_space, struct page，实现将一个文件映射到页的级别，这些具体数据结构及之间的关系我们暂且不讨论，只需知道页缓存的存在以及它在文件IO中扮演着重要角色，很大一部分程度上，文件读写的优化就是对页缓存使用的优化

### Dirty Page
页缓存对应文件中的一块区域，如果页缓存和对应的文件区域内容不一致，则该页缓存叫做脏页（Dirty Page）。对页缓存进行修改或者新建页缓存，只要没有刷磁盘，都会产生脏页

### 查看页缓存大小
linux上有两种方式查看页缓存大小，一种是free命令

	 $ free
	             total       used       free     shared    buffers     cached
	Mem:      20470840    1973416   18497424        164     270208    1202864
	-/+ buffers/cache:     500344   19970496
	Swap:            0          0          0

cached那一列就是页缓存大小，单位Byte

另一种是直接查看/proc/meminfo，这里我们只关注两个字段
	
	Cached:          1202872 kB
	Dirty:                52 kB

Cached是页缓存大小，Dirty是脏页大小

### 脏页回写参数
Linux有一些参数可以改变操作系统对脏页的回写行为

	 $ sysctl -a 2>/dev/null | grep dirty
	vm.dirty_background_ratio = 10
	vm.dirty_background_bytes = 0
	vm.dirty_ratio = 20
	vm.dirty_bytes = 0
	vm.dirty_writeback_centisecs = 500
	vm.dirty_expire_centisecs = 3000

**vm.dirty_background_ratio**是内存可以填充脏页的百分比，当脏页总大小达到这个比例后，系统后台进程就会开始将脏页刷磁盘（vm.dirty_background_bytes类似，只不过是通过字节数来设置）

**vm.dirty_ratio**是绝对的脏数据限制，内存里的脏数据百分比不能超过这个值。如果脏数据超过这个数量，新的IO请求将会被阻挡，直到脏数据被写进磁盘

**vm.dirty_writeback_centisecs**指定多长时间做一次脏数据写回操作，单位为百分之一秒

**vm.dirty_expire_centisecs**指定脏数据能存活的时间，单位为百分之一秒，比如这里设置为30秒，在操作系统进行写回操作时，如果脏数据在内存中超过30秒时，就会被写回磁盘

这些参数可以通过`sudo sysctl -w vm.dirty_background_ratio=5`这样的命令来修改，需要root权限，也可以在root用户下执行`echo 5 > /proc/sys/vm/dirty_background_ratio`来修改

## 文件读写流程
在有了页缓存和脏页的概念后，我们再来看文件的读写流程
### 读文件
* 用户发起read操作
* 操作系统查找页缓存
	* 若未命中，则产生缺页异常，然后创建页缓存，并从磁盘读取相应页填充页缓存
	* 若命中，则直接从页缓存返回要读取的内容
* 用户read调用完成

### 写文件 
* 用户发起write操作
* 操作系统查找页缓存
	* 若未命中，则产生缺页异常，然后创建页缓存，将用户传入的内容写入页缓存
	* 若命中，则直接将用户传入的内容写入页缓存
* 用户write调用完成
* 页被修改后成为脏页，操作系统有两种机制将脏页写回磁盘
	* 用户手动调用fsync()
	* 由pdflush进程定时将脏页写回磁盘

页缓存和磁盘文件是有对应关系的，这种关系由操作系统维护，对页缓存的读写操作是在内核态完成，对用户来说是透明的

## 文件读写的优化思路
不同的优化方案适应于不同的使用场景，比如文件大小，读写频次等，这里我们不考虑修改系统参数的方案，修改系统参数总是有得有失，需要选择一个平衡点，这和业务相关度太高，比如是否要求数据的强一致性，是否容忍数据丢失等等。优化的思路有以下两个考虑点

1. 最大化利用页缓存
2. 减少系统api调用次数

第一点很容易理解，尽量让每次IO操作都命中页缓存，这比操作磁盘会快很多，第二点提到的系统api主要是read和write，由于系统调用会从用户态进入内核态，并且有些还伴随这内存数据的拷贝，因此在有些场景下减少系统调用也会提高性能

### readahead
readahead是一种非阻塞的系统调用，它会触发操作系统将文件内容预读到页缓存中，并且立马返回，函数原型如下

	ssize_t readahead(int fd, off64_t offset, size_t count);

在通常情况下，调用readahead后立马调用read并不会提高读取速度，我们通常在批量读取或在读取之前一段时间调用readahead，假设如下场景，我们需要连续读取1000个1M的文件，有如下两个方案，伪代码如下

**直接调用read函数**
	
	char* buf = (char*)malloc(10*1024*1024);
	for (int i = 0; i < 1000; ++i)
	{
		int fd = open_file();
		int size = stat_file_size();
		read(fd, buf, size);
		// do something with buf
		close(fd);
	}

**先批量调用readahead再调用read**

	int* fds = (int*)malloc(sizeof(int)*1000);
	int* fd_size = (int*)malloc(sizeof(int)*1000);
	for (int i = 0; i < 1000; ++i)
	{
		int fd = open_file();
		int size = stat_file_size();
		readahead(fd, 0, size);
		fds[i] = fd;
		fd_size[i] = size;
	}
	char* buf = (char*)malloc(10*1024*1024);
	for (int i = 0; i < 1000; ++i)
	{
		read(fds[i], buf, fd_size[i]);
		// do something with buf
		close(fds[i]);
	}
	
感兴趣的可以写代码实际测试一下，需要注意的是在测试前必须先回写脏页和清空页缓存，执行如下命令

	sync && sudo sysctl -w vm.drop_caches=3

可通过查看/proc/meminfo中的Cached及Dirty项确认是否生效

通过测试发现，第二种方法比第一种读取速度大约提高10%-20%，这种场景下是批量执行readahead后立马执行read，优化空间有限，如果有一种场景可以在read之前一段时间调用readahead，那将大大提高read本身的读取速度

这种方案实际上是利用了操作系统的页缓存，即提前触发操作系统将文件读取到页缓存，并且操作系统对缺页处理、缓存命中、缓存淘汰都由一套完善的机制，虽然用户也可以针对自己的数据做缓存管理，但和直接使用页缓存比并没有多大差别，而且会增加维护代价

### mmap
mmap是一种内存映射文件的方法，即将一个文件或者其它对象映射到进程的地址空间，实现文件磁盘地址和进程虚拟地址空间中一段虚拟地址的一一对映关系，函数原型如下

	void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);

实现这样的映射关系后，进程就可以采用指针的方式读写操作这一段内存，而系统会自动回写脏页面到对应的文件磁盘上，即完成了对文件的操作而不必再调用read,write等系统调用函数。如下图所示

![](http://littlewhite.us/pic/mmap.jpg)

mmap除了可以减少read,write等系统调用以外，还可以减少内存的拷贝次数，比如在read调用时，一个完整的流程是操作系统读磁盘文件到页缓存，再从页缓存将数据拷贝到read传递的buffer里，而如果使用mmap之后，操作系统只需要将磁盘读到页缓存，然后用户就可以直接通过指针的方式操作mmap映射的内存，减少了从内核态到用户态的数据拷贝

mmap适合于对同一块区域频繁读写的情况，比如一个64M的文件存储了一些索引信息，我们需要频繁修改并持久化到磁盘，这样可以将文件通过mmap映射到用户虚拟内存，然后通过指针的方式修改内存区域，由操作系统自动将修改的部分刷回磁盘，也可以自己调用msync手动刷磁盘
