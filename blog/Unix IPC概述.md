# Unix IPC概述
IPC全称Inter-Process Communication，即进程间通信。我们知道一个进程可以有多个线程，他们可以共享进程的全部资源，比如打开的文件句柄，创建的全局变量等，因此线程间通信相对就容易一些，而不同进程拥有独立的虚拟地址空间，他们之间想要通信就需要特定的IPC方法。我们只讨论同一主机上的进程间通信

这里主要介绍管道，FIFO，消息队列三种IPC通信机制

## 管道
管道是一种单向通信的数据通道，其表现为一对文件句柄，一个写入端和一个读取端，模型如下
![](http://littlewhite.us/pic/pipe1.png)

即用户空间创建管道，得到两个文件句柄，往fd[1]写数据，从fd[0]读数据，虽然管道是由单个进程创建，但很少在单个进程内使用，最常用的是父子进程之间进行通信，如下
![](http://littlewhite.us/pic/pipe2.png)

首先父进程创建管道后调用fork生成子进程，子进程继承了父进程的管道句柄，接着父进程关闭管道的读取端，子进程关闭管道的写入端，这样父子进程之间就生成了一个单向数据流，如下
![](http://littlewhite.us/pic/pipe3.png)

这样父进程往fd[1]写数据，子进程就能从fd[0]读到

管道最常用的场景是unix shell中，比如以下命令

    who | sort | head -1
    
创建了两个管道，还把每个管道的读取端复制到相应进程的标准输入，把写入端复制到相应进程的标准输出，其数据流通如下
![](http://littlewhite.us/pic/pipe4.png)

说了这么多模型，最后来看一下创建管道的API

    int pipe(int pipefd[2]);    
    
创建两个句柄，pipefd[0]是读取端，pipefd[1]是写入端，切勿弄反，句柄操作和普通文件句柄操作一样，通过close来关闭，用read和write来读写

## FIFO
管道没有名字，它必须由一个进程创建，只能由进程自己和它fork出的进程使用，对于没有亲缘关系的进程则不能使用，FIFO又称命名管道，可以在任意进程间使用

先来看创建fifo的API

    int mkfifo(const char *pathname, mode_t mode);
    
pathname为文件路径，mode为文件权限，和open的含义一样，返回为文件句柄。也就是说FIFO必须和文件名绑定，并且在Linux上FIFO本身就是一种文件，我们可以通过mkfifo的命令创建FIFO如下
    
     $ mkfifo fifo && ls -li fifo
    1187053 prw-rw-r-- 1 zhangmenghan zhangmenghan 0 12月  6 22:14 fifo
    
可以看到文件fifo的的类型为p，代表的是管道，并且管道文件也是占用一个inode的。既然是文件，那就可以通过write/read/close/unlink这一系列文件API来操作了，不同点在于，对管道和FIFO的write总是往末尾添加数据，read则总是从开头返回数据，如果对管道或FIFO进行lseek操作，会反回ESPIPE错误

FIFO的读写模型和管道类似，只不过管道返回的是两个句柄，一个写一个读，而FIFO只有一个句柄，其读写属性是在open时指定的，并且它们都可以通过非阻塞方式进行IO操作，只需对句柄设置O_NONBLOCK即可，可通过fcntl函数设置

管道和FIFO的读写还具有以下特性

1. 往一个空管道或FIFO读取数据，默认会阻塞直到有数据写入，若设置了O\_NONBLOCK则返回EAGAIN错误
2. 如果请求读取的数据多余管道或FIFO中的数据，那么只返回这些可用的数据
3. 如果请求写入字节数小于等于PIPE\_BUF，那么write操作保证是原子的，比如两个进程同时请求写同一个管道或FIFO，要么第一个进程先写完要么反之，不会导致数据交错，如果请求写入字节数大于PIPE\_BUF则不能保证原子性，具体的PIPE\_BUF值和操作系统相关，Posix.1要求PIPE\_BUF至少为512字节，通过`long pathconf(char *path, int name);`可查看该值，其中name指定为`_PC_PIPE_BUF`
4. 如果设置了O\_NONBLOCK且待写入字节数小于等于PIPE\_BUF
    * 如果管道或FIFO剩余空间足够，那么所有数据都写入
    * 如果管道或FIFO剩余空间不够，那么立即返回EAGAIN错误。因为此时要保证原子性，所以不会只写入部分数据

5. 如果设置了O\_NONBLOCK且待写入字节数大于PIPE\_BUF
    * 如果管道或FIFO剩余空间足够，则所有数据都写入，否则值写入剩余字节数
    * 如果管道或FIFO已满，则返回EAGAIN错误 

6. 当关闭管道或FIFO，里面的数据会被丢弃

## 消息队列
管道和FIFO都是面向字节流的通信，也就是说读取方并不知道数据的边界，如果写入方将一组的数据写入管道或FIFO，读取方必须知道这组数据的实际长度和格式才能准确将数据读取并解析出来，消息队列提供了一种面相消息的通信方式

消息队列可以被认为是一个消息链表，有写权限的进程往链表放置消息，有读权限的进程从里面取消息。写入者可以随时往消息队列放置消息，而不用管此时是否有读取这，这和管道以及FIFO不同，消息队列有随内核的持续性，即便读写进程退出，消息队列仍然存在

unix上的消息队列实现有两种，Posix消息队列和System V消息队列，这两者都应用的比较广泛，System V消息队列诞生的更早，后来的Posix消息队列加入了一些新特性，因此也被一些新开发的程序所使用，两者提供的API有很多相似性，也有如下一些差别

1. 对Posix消息队列的读总是返回最高优先级的最早消息，对System V消息队列的读则可以返回任意指定优先级的消息
2. 当往一个空队列放置消息时，Posix消息队列允许产生一个信号或启动一个线程，System V消息队列则不提供类似极致

消息队列中的每个消息都具有如下属性
1. 一个无符号整数优先级（Posix）或一个长整形类型（System V）
2. 消息的数据部分长度（可以为0）
3. 数据本身（如果长度大于0）

对消息队列的操作也基本类似，接下来分别介绍两种消息队列的API

### Posix
#### 创建/打开消息队列

    mqd_t mq_open(const char *name, int oflag);
    mqd_t mq_open(const char *name, int oflag, mode_t mode,
                     struct mq_attr *attr);
                     
其中name是消息队列名字，其格式必须符合文件系统路径名，但并不要求是真实存在的文件，oflag是O\_RANDLY，O\_WRONLY或O\_RDWR之一，还可以按位或上O\_CREATE，O\_EXCL，O\_NONBLOCK之一，这和文件API open的参数类似，如果是打开已存在消息队列，这两个参数就够了，如果是新建消息队列，则需要带上mode和atrr参数，mode也和open参数一样是指定读写权限，attr可以设置消息队列属性，如果为NULL则使用默认属性

其返回值为消息队列句柄，作用和文件句柄类似，在消息队列的其它API的第一个参数中都会用到

#### 设置／获取属性

    int mq_getattr(mqd_t mqdes, struct mq_attr *attr);
    int mq_setattr(mqd_t mqdes, struct mq_attr *newattr,
                    struct mq_attr *oldattr);
                    
其属性结构如下
        
    struct mq_attr {
       long mq_flags;       /* Flags: 0 or O_NONBLOCK */
       long mq_maxmsg;      /* Max. # of messages on queue */
       long mq_msgsize;     /* Max. message size (bytes) */
       long mq_curmsgs;     /* # of messages currently in queue */
    };
    
有两个值比较关键，分别是最大消息数(mq\_maxmsg)和单个消息最大长度(mq\_msgsize)
    
#### 发送/接收消息

    int mq_send(mqd_t mqdes, const char *msg_ptr,
                 size_t msg_len, unsigned msg_prio);
    ssize_t mq_receive(mqd_t mqdes, char *msg_ptr,
                 size_t msg_len, unsigned *msg_prio);
                 
前三个参数和write/read类似，最后一个参数代表消息优先级

其中mq\_send返回0代表成功，其它代表失败，这一点和write不同。mq\_receive返回值为实际读取的字节数

#### 消息通知

    int mq_notify(mqd_t mqdes, const struct sigevent *sevp);
    
当往一个空消息队列放置消息时，Posix消息队列可以发送一个通知，这个通知就是会向接收进程发送一个信号。这是System V消息队列所不具备的

mq\_notify就是接受者用来注册或反注册通知信号的，若sevp非空则代表注册，若sevp为空则代表反注册。这种通知机制还有以下特点

1. 任一时刻只有一个进程可以被注册为接收某个指定队列的通知
2. 如果接受者阻塞在mq_received中，通知不会发出
3. 当通知信号发给注册进程，其注册即被撤销，该进程必须再次注册（如果想要的话）

#### 关闭/删除消息队列

    int mq_close(mqd_t mqdes);
    int mq_unlink(const char *name)
    
功能和文件API的close，unlink类似

### System V
### 创建/打开消息队列

    int msgget(key_t key, int msgflg)
    
其返回值是一个整形标识符，用来唯一表示消息队列，用在其它msg函数的第一个参数中。它是基于指定的key产生的，key既可以是ftok的返回值也可以是IPC\_PRIVATE

以下情况会创建新的消息队列

1. key指定为IPC\_PRIVATE
2. key对应的消息队列不存在且msgflag指定了IPC\_CREAT

oflag和open函数的mode参数类似，同时还可以或上IPC\_CREAT和IPC\_EXCL，其含义和O\_CREATE，O\_EXCL类似

#### 发送消息

    int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);

其中msqid为消息标识符，msgp为指向消息结构的指针，消息结构具有如下模板

    struct msgbuf {
       long mtype;       /* message type, must be > 0 */
       char mtext[1];    /* message data */
    };
    
mtype为消息类型，必须大于0，mtext为消息数据，它可以是任何类型的数据，不管是二进制还是文本，其大小由msgsz指定，也就是说参数中msgsz的大小是msgbuf结构中除mtype之外的大小，比如我们可以定义自己的消息结构

    struct my_msgbuf {
        long mtype;
        short myshort;
        char mydata[1024];
    };
    
此时msgsz的大小为`sizeof(struct my_msgbuf) - sizeof(long)`

msgflag可以指定为0，也可以指定为IPC\_NOWAIT，当指定了IPC\_NOWAIT时，若消息无法发送出去，msgsnd函数会立马返回EAGAIN错误，否则会一直阻塞直到发送成功或消息队列被删除

#### 接收消息

    ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp,
                   int msgflg);
                   
msgp，msgsz，msgflg的含义和msgsnd函数一样，msgtyp代表希望读取的消息类型

若msgtyp为0，则返回队列中第一个消息，若msgtyp大于0，则返回队列中类型值（msgbuf模板中的mtype字段）小于或等于msgtyp的绝对值的消息中类型值最小的第一个消息

#### 修改消息队列

    int msgctl(int msqid, int cmd, struct msqid_ds *buf)
    
该函数提供在一个消息队列上的各种控制操作，cmd可以指定如下三个命令

1. IPC\_RIMD 从系统中删除msqid指定的消息，此时第三个参数被忽略
2. IPC\_SET 设置消息队列msqid\_ds结构中的以下4个成员：`msg_qbytes, msg_perm.uid, msg_perm.gid, msg_perm.mode`，其它成员不会被修改
3. IPC_STAT 获取消息队列的msqid\_ds结构

## 命令行操作IPC对象
以上介绍了常用的IPC通信方式，并且介绍了相应的API，和文件操作类似，操作系统还提供了通过shell命令行操作IPC对象的方式，主要有mkfifo，ipcmk，ipcs，ipcrm等，具体使用方式可以直接查看man手册，这里举几个例子说明一下

1. mkfifo 创建FIFO，创建完之后可以直接使用echo，cat，rm等命令，和操作文件类似
2. ipcmk -Q 创建消息队列
3. ipcs -q 查看消息队列
4. ipcrm -q msgid 删除msgid指定的消息队列
 
                      
