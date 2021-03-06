<script type="text/javascript" src="http://cdn.mathjax.org/mathjax/latest/MathJax.js?config=default"></script>
# http缓存服务器淘汰策略分析

根据设计需求，一共有三级缓存，分别是内存，SSD，磁盘，所以缓存资源淘汰路径可以是

* 内存 -> SSD
* SSD -> 硬盘
* 硬盘 -> 删除

也会有资源的优先级提升，比如从磁盘提升到SSD或内存。这三种缓存资源可采用同一个优先级队列来管理，新增一个资源时先计算其优先级，得到其在优先级队列中的位置，通过位置可决定存储到哪种媒介，同样，当访问资源时更新其优先级即其在队列中的位置，如果该位置对应的媒介发生变化，则需要做资源的迁移，并且在迁移时可能对目的媒介做调整以满足迁移需求。

具体有哪些存储媒介涉及具体实现，淘汰算法本身不关心这些，淘汰算法要做的只是调整资源在优先级队列中的位置，至于调整之后的操作则由业务层去负责，因此下面只针对淘汰算法本身来讨论

## LRU
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

## 抽象
前面主要描述了LRU算法结合hash索引的各种操作，实际上任何一个淘汰策略模型都可以被抽象为一个有序队列，每个节点持有一个value，这个value由具体的函数计算得到，队列根据value排序，这样，淘汰策略模型具体操作可描述如下

* 添加节点： 计算节点value，插入队列，并对队列重新排序
* 删除指定节点： 将节点从队列中删除
* 访问节点： 重新计算该节点value，并对队列重新排序
* 淘汰节点： 从value最小的节点开始依次淘汰

模型的关键在于保持队列有序和计算节点vlaue值，假设我们已经有一个模型能满足基本的插入删除等操作，并保持队列有序，我们只需要实现不同的value计算函数即可实现不同的淘汰算法

以LRU为例，其value计算函数可描述为 

$$ V\_i = LatestRefTime $$

即节点最近访问时间，每次访问节点均更新时间，这样新添加和最近被访问的节点优先级最高

基于这种抽象模型，下面介绍几种其它淘汰策略

## squid淘汰策略
除了LRU以外，squid还实现了另外两种淘汰策略，这两种策略均可减少LRU缓存污染的缺点，并针对资源命中率和资源字节命中率做了优化

### GDSF
GDSF（GreddyDual-Size with Frequency）会同时考虑资源访问频次和资源大小，越小的文件被缓存的可能性越大，因此该算法可提高资源命中率，其value计算函数描述如下

$$ V\_i = F\_i * C\_i/S\_i + L$$

* \\( V\_i \\) 代表对象\\( i \\)计算的value值
* \\( F\_i \\) 代表对象的访问频次
* \\( C\_i \\) 代表将对象加入缓存的开销，根据squid论文，该值取1时效果最佳
* \\( S\_i \\) 代表对象大小
* \\( L \\) 为动态age，随着对象的加入而递增

### LFU-DA
LFU-DA（Least Frequently Used with Dynamic Aging）是基于LFU（Least Frequently Used）增加了动态age，它更倾向于缓存被访问频次大的对象，而不论对象大小是多少，因此它可以获得更大的资源字节命中率，其value计算函数描述如下

$$ V\_i = C\_i * F\_i + L$$

* \\( V\_i \\) 代表对象\\( i \\)计算的value值
* \\( F\_i \\) 代表对象的访问频次
* \\( C\_i \\) 代表将对象加入缓存的开销
* \\( L \\) 为动态age，随着对象的加入而递增

当\\( C\_i \\) 取值为1时，该算法等价于在LFU基础上添加动态age

squid中均有以上两种策略的实现，均采用heap管理，只是提供不同计算value的函数
