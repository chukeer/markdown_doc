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
限制索引在内存中的占用量，超出规定的部分会持久化到磁盘，考虑到URL的特性，将索引设计成两级，第一级索引到site，第二级索引到具体到URL，因此不同级别索引的value对象不同，我们定义如下对象结构

    struct Object
    {
        unsigned int type:4; // 对象类型
        void *ptr;           // 指向对象内容的指针
    }
    
定义如下集中对象类型

1. TYPE_STRING， 所有HashTable的key均该类型
2. TEYPE_HASHTABLE, 第一级索引的value为该类型
3. TYPE_MEM_INDEX， 第二级索引的value内容如果在内存中，则为该类型
4. TYPE_DISK_INDEX, 第二级索引的value内容如果在磁盘中，则为该类型

其中String为基础类型，HashTable前面已定义，MemIndex和DiskIndex结构定义如下

    struct MemIndex
    {
        FileInfo* file_info; // 缓存资源在磁盘上的信息 
        MemObject* mem_obj;  // 缓存资源在内存中的信息，仅当缓存资源被加载到内存时才有效
    };
    
    struct DiskIndex
    {
        string file_path;  // 索引信息持久化的文件路径
        int offset;   // 索引信息在持久化文件中的偏移
        int len;      // 索引信息在持久化文件中的大小
    };
    
至此，索引中的所有对象可如下定义

    typedef string StringObject;
    typedef HashTable HashObject;
    typedef MemIndex MemIndexObject;
    typedef DiskIndex DiskIndexObject;
    
    

