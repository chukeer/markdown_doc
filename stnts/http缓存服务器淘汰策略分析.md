# http缓存服务器淘汰策略分析

根据设计需求，一共有三级缓存，分别是内存，SSD，磁盘，所以缓存资源淘汰路径可以是

* 内存 -> SSD
* SSD -> 硬盘
* 硬盘 -> 删除

不同的淘汰路径可以使用不同淘汰策略，对于淘汰策略本身来说，哪条路径并没有什么影响，因此下面只讨论通用的淘汰方法

### LRU
最常见也是实现最简单的策略就是LRU（Least Recently Used，最近最少使用）算法，根据数据的历史访问记录来进行淘汰数据，其核心思想是“如果数据最近被访问过，那么将来被访问的几率也更高”

LRU一般采用双向链表实现，基本结构如下

    struct LruNode
    {
        LruNode* prev;
        LruNode* next;
        void* data;
    };
    
    struct LruList
    {
        LruNode* head;
        LruNode* tail;
    };
    
LruNode中的data成员即指向实际缓存索引数据，假设缓存索引以hash结构表示，则淘汰链结构可设计如下

![](http://littlewhite.us/pic/stnts/http-removal-1.png)

这样hash结构和LRU链表结构分离，分别持有对方指针。下面考虑资源的三种操作

**删除节点**  
假设删除key2，先通过key2查找到ValueObject，得到指向LruNode的指针，删除该节点即可，时间复杂度O(1)

![](http://littlewhite.us/pic/stnts/http-removal-2.png)

**新增节点**  
新增节点直接加入LruList头部，时间复杂度O(1)如下
![](http://littlewhite.us/pic/stnts/http-removal-3.png)

新增节点可能会导致缓存达到上限，比如限定内存缓存上限2G，新增一个内存缓存后会操过2G，则需要删除一些资源腾出空间，此时只需要从LruList尾部开始遍历，依次删除直到内存满足需求为止，时间复杂度O(M)，M为需要删除的节点个数。假设加入节点key5时需要删除key1，则结构如下

![](http://littlewhite.us/pic/stnts/http-removal-4.png)
**访问节点**  
在LRU算法中，一个节点被访问后只需将该节点移动到链表头即可，时间复杂度O(1)，假设访问key4，则结构如下
![](http://littlewhite.us/pic/stnts/http-removal-5.png)

**LRU优缺点**  

* 优点：实现简单
* 缺点：当存在热点数据时，LRU的效率很好，但偶发性的、周期性的批量操作会导致LRU命中率急剧下降，缓存污染情况比较严重

    
