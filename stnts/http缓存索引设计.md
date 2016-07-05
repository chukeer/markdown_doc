### 应用场景
单机的http缓存的索引管理，索引主要以内存存储为主，当超出期望内存时，会将索引持久化到磁盘

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
        uint32_t size;     // hash表数组大小
        uint32_t used;     // 已存储的hash节点数量
    };
    
为简化设计，暂不考虑rehash，即当HashTable的used远远大于size时，不会扩大hash表使得节点分布更均匀

### 缓存索引
首先考虑最简单的情况，索引即为一个hash表，存储的是URL到缓存资源信息的映射关系，这里的缓存资源信息包括缓存在磁盘上的信息以及在内存中的信息，先假设这些信息全部存储在一张hash表，且全部在内存中

我们定义基本的对象结果如下

    struct Object
    {
        uint32_t type:4; // 对象类型
        void *ptr;       // 指向对象内容的指针
    }

hash的key和value都由基本的对象表示，ptr指向实际的存储内容，URL用字符串（String）表示即可，定义如下结构表示value

    struct MemIndex
    {
        FileInfo* file_info; // 缓存资源在磁盘上的信息 
        MemObject* mem_obj;  // 缓存资源在内存中的信息，仅当缓存资源被加载到内存时才有效
    };

其中FileInfo表示磁盘上的文件信息，如文件路径，修改时间等等，甚至可能包含缓存资源的包头信息，MemObject表示加载到内存的缓存资源，仅当缓存被访问过才会被加载到内存

这样索引格式如下

![http缓存设计-step1](http://littlewhite.us/pic/stnts/http-cache-step1.png)

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

假设我们将索引信息持久化到1000个1M的小文件并对小文件进行编号，则该结构可唯一确定一段长度为len的磁盘数据，磁盘数据存储的是Url和MemIndex的固化信息，同时我们以URL的hash值（uint32_t）来索引DiskIndex数据，这样可另建一张hash表如下

![http缓存设计-step2](http://littlewhite.us/pic/stnts/http-cache-step2.png)

因为不同URl可能有同样的hash值，所以这里的value并不是一个DiskIndex对象，而是一个链表

这样，新增一个缓存索引的步骤如下

1. 计算Url hash值
2. 通过hash值去hash_table里查找DiskIndex链表
3. 若未找到，则在持久化文件里获取一个可用的分片，将Url和MemIndex信息写入，创建一个hash节点并在DiskIndex链表头插入一个节点

查找缓存资源步骤如下

1. 计算Url的hash值
2. 通过hash值去hash_table里查找DiskIndex链表
3. 遍历链表，根据DiskIndex读取磁盘分片，得到具体的Url和MemIndex信息，并和查询Url进行比较，定位到MemIndex
4. 根据MemIndex信息定位到缓存资源（此时只有磁盘缓存信息，没有内存缓存信息）

删除缓存资源的步骤如下

1. 计算Url hash值
2. 通过hash值去hash_table里查找DiskIndex链表
3. 遍历链表，根据DiskIndex读取磁盘分片，得到具体Url和MemIndex信息，并和Url进行比较，定位到MemIndex
4. 根据MemIndex信息删除缓存资源，并且根据DiskIndex信息读取完整1M索引持久化信息，并更新之，需要对受影响的部分重新计算offset

所有操作复杂度均为O(1)，只是多了一次磁盘IO（仅当查找到hash值时才会有磁盘IO，当hash值冲突时才会增加磁盘IO），并且key value一共只有12个字节，即便加上指针开销，平均一个Entry的开销也只有大约20字节，索引30000000文件需要的内存开销为

	30000000 * 20B = 600M

所以，以这样的索引方式即可解决海量文件索引的内存开销问题，我们可以采用两种hash表并存的方式设计索引，如下

	struct CacheIndex
	{
		HashTable tb[2];
	};

其中tb[0]中存的是Url到MemIndex的映射，tb[1]存的是hash(Url)到DiskIndex的映射，在查找时，先查tb[0]，如果没有找到再查tb[1]

### 优化
索引的目的是通过URL快速定位到缓存文件，在定位缓存文件的过程中，我们希望尽量减少磁盘IO，查找CacheIndex的tb[0]时是不会有磁盘IO的，但是查找tb[1]可能会有多次磁盘IO，以下是其hash结构

![hash桶结构1](http://littlewhite.us/pic/stnts/hash-bucket1.png)

当我们通过URL的hash值查找时，假设落到了bucket[0]，此时bucket[0]上挂有3个hash节点，这些hash节点的hash值可能相同也可能不同（不同的hash值可能映射到相同的bucket），根据前面的设计，我需要对hash值相同的节点（Entry1, Entry2, Entry3）和被查找的URL进行比较，而Entry里并没有存储URL，只存了持久化文件的offset和len，因此需要读一次磁盘才能进行比较，相同hash值越多，查找一次需要读磁盘的次数就越多

我们将hash值相同的Entry做一个合并，如下

![hash桶结构2](http://littlewhite.us/pic/stnts/hash-bucket2.png)

Entry1下对应的offset和len其实是原来的三个节点的持久化信息，这样我们可以通过一次磁盘IO读取所有hash值相同的索引信息，不管hash冲突多少次，每次查找都只有一次磁盘IO

### 索引块大小
当tb[1]进行删除或新增操作时，我们需要更新其对应的索引块，这样会对后面所有索引的offset发生影响，需要全部做一次调整，索引块按1M算，每个索引结构按200字节算，假设更新的正好在1M开头部分，则后面一共约50000个索引需要更新offset，这将是带来很大的CPU浪费，因此索引块大小需要调整为合适的大小，并且一个索引块能容纳下一个Entry对应的持久化信息，若不能容纳，可将索引块通过指针串起来

### 索引信息持久化和恢复
以上两张hash表在服务运行过程都是在内存中，当服务器重启之后有必要重建两张hash表，因此要考虑hash的持久化和恢复


第一张hash由于数据量较少，可以直接存储到一个文件中，按key-value格式存储即可，如下

![hash1-persist](http://littlewhite.us/pic/stnts/hash1-persist.png)

根据这样的文件格式，恢复hash表也很容易，读取一个数据块后将URL-MemInfo写入hash表即可

第二张表由于本身就对应磁盘文件，可以不需要做持久化，我们设计每个hash结构的对应磁盘分片格式如下

![hash1-persist](http://littlewhite.us/pic/stnts/hash2-persist.png)

通过从磁盘读取这样的数据，可直接恢复对应的hash节点

### 站点索引
我们可能需要对缓存资源做站点级别的操作，比如清除某个站点的缓存，持久化某个站点的缓存，统计某个站点的缓存等等，如果只以一层hash结构无法快速得到指定站点信息，因此可以做两层索引如下

![http缓存设计-step3](http://littlewhite.us/pic/stnts/http-cache-step3.png)

这里只列出了MemIndex的两层索引，同理可得DiskIndex的两层索引，这样缓存结构仍旧为CacheIndex保持不变，只是每次查找先根据site查找，得到的是一个HashObject，然后在根据URL后半段查找，具体步骤就不详述了。由于站点数量有限，所以增加一级索引对内存开销并不大

### 分片请求的索引
分片请求主要有两种表现形式

1. 将参数直接写在URL中，比如爱奇艺的视频链接

		http://58.51.148.170/videos/v0/20151214/b2/f6/f517e225fac731abe250ec1c7ea71292.f4v?key=08d356f6f7fdc2bb29f5d3c566edfb6ed&src=iqiyi.com&ran=363&qyid=c1bacdc0dcad75c1bd2ee3f710aff9f7&qypid=429905600_11&ran=363&uuid=3bafe99a-567248c9-50&range=0-747230&qypid=429905600_01010011010000000000_96&ran=389

	其中的`range=0-747230`字段即代表了请求的分片信息

2. 采用http标准的Range包头，在断点续传和一些视频定点播放时会有这样的请求，包头字段格式为

		Range: bytes=start-end

	其中start是从0开始

	示例如下

	![curl Range](http://littlewhite.us/pic/stnts/Range.png)
    
	资源xx一共有10个字节，当我们指定Range字段时会返回对应的字节

形式1太过定制化，链接格式千变万化，且无法预知具体哪个字段标记了实际的Range信息，对于这种链接可将分片链接当作单独的资源缓存。形式2具有普遍性，是http标准协议，下面主要讨论这种形式的处理方式

从示例图片可以看到，在请求不同Range时，客户端发送的URL其实是一样的，比较极端的处理方式可以将URL对应的完整资源缓存成一个文件，当有不同Range请求过来时，直接返回缓存文件对应的分片即可。我们假设带Range信息的请求总是从资源头部开始的，当客户端请求Range: bytes=0-1000时，我们实际上会去请求完整文件，并且在原始服务器返回0-1000字节后即返回给客户端，如果接下来请求Range: bytes=1001-2000时，缓存即为命中，只需等待原始服务器返回1001-2001字节请即可返回给客户端

按这种方式，上面的索引方式无需改变，一个URL仍旧对应一个缓存资源，只需根据Range字段做相应处理即可，但是问题在于，如果客户端一开始请求Range: bytes=1001-2000的分片，在没有缓存的时候我们仍旧需要向原始服务器请求完整文件，并且一直要等到1000-2001字节返回后才能返回给客户端，也就是说原始服务器返回0-1000字节的这段时间内我们是不会响应客户端的，因此会增加客户端的等待时间

>PS: squid即可通过参数配置按以上方式工作

如果需要对分片信息进行单独索引，在以上缓存索引设计的基础上，只需要改变一个结构即可

定义分片信息结构

	struct Range
	{
		uint32_t start;
		uint32_t end;
		void* data;
	};

其中start和end描述分片信息里的字段，data为该分片对应的内存数据。假设资源xx在缓存系统中的磁盘缓存文件为test\_xx，我们约定Range: bytes=0-1000请求对应的缓存文件为test\_xx.0-1000这种格式，因此根据Range字段即可定位到分片的磁盘缓存文件

我们在MemIndex中增加一个字段，定义如下

	struct MemIndex
    {
        FileInfo* file_info; // 缓存资源在磁盘上的信息 
        MemObject* mem_obj;  // 缓存资源在内存中的信息，仅当缓存资源被加载到内存时才有效
		list<Range>* ranges; // 记录分片请求信息，以start字段升序排列
    };

假设我们已经接受了Range分别为0-1000， 1001-2000， 3001-4000的请求，则Range缓存信息如下

![Range链表示例](http://littlewhite.us/pic/stnts/Range-list-case.png)

当我们请求Range: bytes=0-1500时，在链表里查找第一个start<=0的节点和第一个end>=1500的节点，并且这两个几点之间的所有节点的Rang必须连续，即当前节点的start比前一个节点的end大1，此时即认为命中，依次将这些节点里的void* data按需返回给客户端即可，若data为NULL，则根据start和end参数读取磁盘文件返回给客户端

如果请求Range: bytes=1001-4000，根据规则，由于node2和node3的Range信息不连续，因此不命中

**Range的合并**

以上示例中node1和node2有着连续的Range，可采取某种触发机制让其合并，合并后的Range信息为

![Range链表示例2](http://littlewhite.us/pic/stnts/Range-list-case2.png)

**Range的覆盖**

当新增加的Range范围大于原来的Range时可将原Rang信息覆盖，还是以上图为例，假设请求Range为3001-5000，当请求结束后，node2将会被替换，如下

![Range链表示例3](http://littlewhite.us/pic/stnts/Range-list-case3.png)

这样，原索引结构无需改变，仍旧以Url为key进行查找，只是在MemIndex里增加一个结构专门管理Range信息，当请求带Range字段时需要额外查找Range信息