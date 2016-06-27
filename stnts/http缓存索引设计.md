### 应用场景
单机的http缓存的索引管理

http缓存分三级

1. 内存
2. SSD
3. 磁盘

索引主要以内存存储为主，当超出期望内存时，会将索引持久化到磁盘

### 数据量估计
以视频分片请求为主，分片大小按100K（作为参考，优酷分片请求约为700K）算，磁盘缓存大小按3T算，缓存的文件数量为`3T / 100K = 30000000`，考虑到缓存资源本身也会占用大量内存，因此应尽量减少索引的内存开销

### 基础结构
**hash节点**

    struct Entry
    {
        void *key;
        void *val;
        Entry *next;
    };
    
**hash表**

    struct HashTable 
    {
        Entry **buckets;   // hash表数组
        HASHCMP *cmp;      // key比较函数
        HASHHASH *hash;    // 对key计算hash值的函数
        unsigned int size; // hash表数组大小
        int used;          // 已存储的hash节点数量
    };
    
为简化设计，暂不考虑rehash，即当HashTable的used远远大于size时，不会扩大hash表使得节点分布更均匀

### 缓存索引
首先考虑最简单的情况，索引即为一个hash表，存储的是URL到缓存资源信息的映射关系，这里的缓存资源信息包括缓存在磁盘上的信息以及在内存中的信息，先假设这些信息全部存储在一张hash表，且全部在内存中

我们定义基本的对象结果如下

    struct Object
    {
        unsigned int type:4; // 对象类型
        void *ptr;           // 指向对象内容的指针
    }

hash的key和value都由基本的对象表示，ptr指向实际的存储内容，URL用字符串（String）表示即可，定义如下结构表示value

    struct MemIndex
    {
        FileInfo* file_info; // 缓存资源在磁盘上的信息 
        MemObject* mem_obj;  // 缓存资源在内存中的信息，仅当缓存资源被加载到内存时才有效
    };

其中FileInfo表示磁盘上的文件信息，如文件路径，修改时间等等，甚至可能包含缓存资源的包头信息，MemObject表示加载到内存的缓存资源，仅当缓存被访问过才会被加载到内存

这样索引格式如下

![http缓存设计-step1](https://raw.githubusercontent.com/handy1989/markdown_doc/master/stnts/pic/http%E7%BC%93%E5%AD%98%E8%AE%BE%E8%AE%A1-step1.png)

这样索引的查找，增加，删除都使用hash表的基本接口，复杂度为O(1)，问题在于内存消耗太大，我们可以大致估算一下

hash表的每一对key-value都由一个Entry结构表示，key为URL，value为MemIndex结构，URL平均长度按100算，MemIndex需要存磁盘路径信息等等，也按100字节算，这样一个Entry的开销为200字节（忽略其它指针的开销），缓存文件总量30000000，总大小为

	30000000 * 200B = 6G

这显然是不现实的，究其原因，主要是因为Entry需要存储Url和MemInfo信息，我们可以将其持久化到磁盘，并且以一个唯一的offset和len来表示，就可大大节省内存

定义如下结构

	Struct DiskIndex
	{
		uint32_t offset;
		uint32_t len;
	};

假设我们将索引信息持久化到1000个1M的小文件并对小文件进行编号，则该结构可唯一确定一段长度为len的磁盘数据，磁盘数据存储的是Url和MemIndex的固话信息，同事我们以URL的hash值（uint32_t）来索引DiskIndex数据，这样可另建一张hash表如下

![http缓存设计-step2](https://raw.githubusercontent.com/handy1989/markdown_doc/master/stnts/pic/http%E7%BC%93%E5%AD%98%E8%AE%BE%E8%AE%A1-step2.png)

因为不同URl可能有同样的hash值，所以这里的value并不是一个DiskIndex对象，而是一个链表

这样，新增一个URl对应缓存资源的步骤如下

1. 计算Url hash值
2. 通过hash值去hash_table里查找DiskIndex链表
3. 若未找到，则在持久化文件里获取一个可用的分片，将Url和MemIndex信息写入，创建一个hash节点并在DiskIndex链表头插入一个节点

查找一个Url对应缓存资源步骤如下

1. 计算Url的hash值
2. 通过hash值去hash_table里查找DiskIndex链表
3. 遍历链表，根据DiskIndex读取磁盘分片，得到具体的Url和MemIndex信息，并和查询Url进行比较，定位到MemIndex
4. 根据MemIndex信息定位到缓存资源（此时只有磁盘缓存信息，没有内存缓存信息）

删除一个URl对应缓存资源的步骤如下

1. 计算Url hash值
2. 通过hash值去hash_table里查找DiskIndex链表
3. 遍历链表，根据DiskIndex读取磁盘分片，得到具体Url和MemIndex信息，并和Url进行比较，定位到MemIndex
4. 根据MemIndex信息删除缓存资源，并且根据DiskIndex信息读取完整1M索引持久化信息，并更新之

所有操作复杂度均为O(1)，只是多了一次磁盘IO（仅当查找到hash值时才会有，当hash值冲突时才会增加磁盘IO），并且key value一共只有12个字节，即便加上指针开销，平均一个Entry的开销也只有大约20字节，索引30000000文件需要的内存开销为

	30000000 * 20B = 600M

所以，以这样的索引方式即可解决海量文件索引的内存开销问题，我们可以采用两种hash表并存的方式设计索引，如下

	struct CacheIndex
	{
		HashTable tb[2];
	};

其中tb[0]中存的是Url到MemIndex的映射，tb[1]存的是hash(Url)到DiskIndex的映射，在查找时，先查tb[0]，如果没有找到再查tb[1]

### 两层索引
我们可能需要对缓存资源做站点级别的操作，比如清楚某个站点的缓存，持久化某个站点的缓存，统计某个站点的缓存等等，如果只以一层hash结构无法快速得到指定站点信息，因此可以做两层索引如下

![](http://i.imgur.com/L5OFWS6.png)

这里只列出了MemIndex的两层索引，同理可得DiskIndex的两层索引，这样缓存结构仍旧为CacheIndex保持不变，只是每次查找先根据site查找，得到的是一个HashObject，然后在根据URL后半段查找，具体步骤就不详述了。由于站点数量优先，所以增加一级索引对内存开销并不大

### 对于带偏移的资源的索引
分片请求主要有两种表现形式

1. 将参数直接写在URL中，比如爱奇艺的视频链接

		http://58.51.148.170/videos/v0/20151214/b2/f6/f517e225fac731abe250ec1c7ea71292.f4v?key=08d356f6f7fdc2bb29f5d3c566edfb6ed&src=iqiyi.com&ran=363&qyid=c1bacdc0dcad75c1bd2ee3f710aff9f7&qypid=429905600_11&ran=363&uuid=3bafe99a-567248c9-50&range=0-747230&qypid=429905600_01010011010000000000_96&ran=389

	其中的`range=0-747230`字段即代表了请求的分片信息

2. 采用http标准的Range包头，在断点续传和一些视频定点播放时会有这样的请求，包头字段格式为

		Range: bytes=start-end

	其中start是从0开始

	示例如下

	![](http://i.imgur.com/aPge4yh.png)
    
	资源xx一共有10个字节，当我们指定Range字段时会返回对应的字节

形式1太过定制化，链接格式千变万化，且无法预知具体哪个字段标记了实际的Range信息，对于这种链接可将分片链接当作单独的资源缓存。形式2具有普遍性，是http标准协议，下面主要讨论这种形式的处理方式

从示例图片可以看到，在请求不同Range时，客户端发送的URL其实是一样的，比较极端的处理方式可以将URL对应的完整资源缓存称一个文件，当有不通Range请求过来时，直接返回缓存文件对应的分片即可。我们假设带Range信息的请求总是从资源头部开始的，当客户端请求Range: bytes=0-1000时，我们实际上会去请求完整文件，并且在原始服务器返回0-1000字节后即返回给客户端，如果接下来请求Range: bytes=1001-2000时，缓存即为命中，只需等待原始服务器返回1001-2001字节请即可返回给客户端

按这种方式，上面的索引方式无需改变，一个URL仍旧对应一个缓存资源，只需根据Range字段做相应处理即可，但是问题在于，如果客户端一开始请求Range: bytes=1001-2000的分片，我们仍旧需要向原始服务器请求完整文件，并且一直要等到1000-2001字节返回后才能返回给客户端，也就是说原始服务器返回0-1000字节的这段时间内我们是不会响应客户端的，因此会增加客户端的等待时间

如果需要对分片信息进行单独索引，在以上缓存索引设计的基础上，只需要改变一个结构即可

定义分片信息结构

	struct Range
	{
		uint32_t start;
		uint32_t end;
		void* data;
	};

其中start和end描述分片信息里的字段，data为该分片对应的内存数据。假设资源xx在缓存系统中的磁盘缓存文件为test_xx，我们约定Range: bytes=0-1000请求对应的缓存文件为test_xx.0-1000这种格式，因此根据Range字段即可定位到磁盘缓存文件