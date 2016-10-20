Title: Linux程序编译链接动态库版本的问题
Date: 2015-08-17
Modified: 2015-08-17
Category: Skill
Tags: linux
Slug: Linux程序编译链接动态库版本的问题
Author: littlewhite

[TOC]

不同版本的动态库可能会不兼容，如果程序在编译时指定动态库是某个低版本，运行是用的一个高版本，可能会导致无法运行。Linux上对动态库的命名采用libxxx.so.a.b.c的格式，其中a代表大版本号，b代表小版本号，c代表更小的版本号，我们以Linux自带的cp程序为例，通过ldd查看其依赖的动态库
    
     $ ldd /bin/cp                                                                                                                                                                                        
    linux-vdso.so.1 =>  (0x00007ffff59df000)
    libselinux.so.1 => /lib64/libselinux.so.1 (0x00007fb3357e0000)
    librt.so.1 => /lib64/librt.so.1 (0x00007fb3355d7000)
    libacl.so.1 => /lib64/libacl.so.1 (0x00007fb3353cf000)
    libattr.so.1 => /lib64/libattr.so.1 (0x00007fb3351ca000)
    libc.so.6 => /lib64/libc.so.6 (0x00007fb334e35000)
    libdl.so.2 => /lib64/libdl.so.2 (0x00007fb334c31000)
    /lib64/ld-linux-x86-64.so.2 (0x00007fb335a0d000)
    libpthread.so.0 => /lib64/libpthread.so.0 (0x00007fb334a14000)
    
左边是依赖的动态库名字，右边是链接指向的文件，再查看libacl.so相关的动态库

      $ ll /lib64/libacl.so*                                                                                                                                                                               
    lrwxrwxrwx. 1 root root    15 1月   7 2015 /lib64/libacl.so.1 -> libacl.so.1.1.0
    -rwxr-xr-x. 1 root root 31280 12月  8 2011 /lib64/libacl.so.1.1.0
    
我们发现libacl.so.1实际上是一个软链接，它指向的文件是libacl.so.1.1.0，命名方式符合我们上面的描述。也有不按这种方式命名的，比如

    $ ll /lib64/libc.so*                                                                                                                                                                                  
    lrwxrwxrwx 1 root root 12 8月  12 14:18 /lib64/libc.so.6 -> libc-2.12.so
    
不管怎样命名，只要按照规定的方式来生成和使用动态库，就不会有问题。而且我们往往是在机器A上编译程序，在机器B上运行程序，编译和运行的环境其实是有略微不同的。下面就说说动态库在生成和使用过程中的一些问题

## 动态库的编译
我们以一个简单的程序作为例子

    // filename:hello.c
    #include <stdio.h>
    
    void hello(const char* name)
    {
        printf("hello %s!\n", name);
    }
    
    // filename:hello.h
    void hello(const char* name);
    
采用如下命令进行编译

    gcc hello.c -fPIC -shared -Wl,-soname,libhello.so.0 -o libhello.so.0.0.1

需要注意的参数是`-Wl,soname`（中间没有空格），`-Wl`选项告诉编译器将后面的参数传递给链接器，
-soname则指定了动态库的soname(简单共享名，Short for shared object name)

现在我们生成了libhello.so.0.0.1，当我们运行`ldconfig -n .`命令时，当前目录会多一个软连接

    $ ll libhello.so.0                                                                                                                                                                                   
    lrwxrwxrwx 1 handy handy 17 8月  17 14:18 libhello.so.0 -> libhello.so.0.0.1

这个软链接是如何生成的呢，并不是截取libhello.so.0.0.1名字的前面部分，而是根据libhello.so.0.0.1编译时指定的-soname生成的。也就是说我们在编译动态库时通过-soname指定的名字，已经记载到了动态库的二进制数据里面。不管程序是否按libxxx.so.a.b.c格式命名，但Linux上几乎所有动态库在编译时都指定了-soname，我们可以通过readelf工具查看soname，比如文章开头列举的两个动态库

    $ readelf -d /lib64/libacl.so.1.1.0                                                                                                                                                                   
    
    Dynamic section at offset 0x6de8 contains 24 entries:
    Tag        Type                         Name/Value
    0x0000000000000001 (NEEDED)             Shared library: [libattr.so.1]
    0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]
    0x000000000000000e (SONAME)             Library soname: [libacl.so.1]
    
这里省略了一部分，可以看到最后一行SONAME为libacl.so.1，所以/lib64才会有一个这样的软连接

再看libc-2.12.so文件，该文件并没有采用我们说的命名方式

    $ readelf -d /lib64/libc-2.12.so                                                                                                                                                                      
    
    Dynamic section at offset 0x18db40 contains 27 entries:
    Tag        Type                         Name/Value
    0x0000000000000001 (NEEDED)             Shared library: [ld-linux-x86-64.so.2]
    0x000000000000000e (SONAME)             Library soname: [libc.so.6]
    
同样可以看到最后一行SONAME为libc.so.6，即便该动态库没有按版本号的方式命名，但仍旧有一个软链指向该动态库，而该软链的名字就是soname指定的名字

**所以关键就是这个soname，它相当于一个中间者，当我们的动态库只是升级一个小版本时，我们可以让它的soname相同，而可执行程序只认soname指定的动态库，这样依赖这个动态库的可执行程序不需重新编译就能使用新版动态库的特性**

## 可执行程序的编译
还是以hello动态库为例，我们写一个简单的程序

    // filename:main.c
    #include "hello.h"
    
    int main()
    {
        hello("handy");
        return 0;
    }

现在目录下是如下结构

    ├── hello.c
    ├── hello.h
    ├── libhello.so.0 -> libhello.so.0.0.1
    ├── libhello.so.0.0.1
    └── main.c
    
libhello.so.0.0.1是我们编译生成的动态库，libhello.so.0是通过ldconfig生成的链接，采用如下命令编译main.c

     $ gcc main.c -L. -lhello -o main                                                                                                                                                                            
    /usr/bin/ld: cannot find -lhello
    
报错找不到hello动态库，在Linux下，编译时指定-lhello，链接器会去寻找libhello.so这样的文件，当前目录下没有这个文件，所以报错。建立这样一个软链，目录结构如下

    ├── hello.c
    ├── hello.h
    ├── libhello.so -> libhello.so.0.0.1
    ├── libhello.so.0 -> libhello.so.0.0.1
    ├── libhello.so.0.0.1
    └── main.c
    
让libhello.so链接指向实际的动态库文件libhello.so.0.0.1，再编译main程序

    gcc main.c -L. -lhello -o main

这样可执行文件就生成了。通过以上测试我们发现，**在编译可执行程序时，链接器会去找它依赖的libxxx.so这样的文件，因此必须保证libxxx.so的存在**

用ldd查看其依赖的动态库

     $ ldd main                                                                                                                                                                                            
            linux-vdso.so.1 =>  (0x00007fffe23f2000)
            libhello.so.0 => not found
            libc.so.6 => /lib64/libc.so.6 (0x00007fb6cd084000)
            /lib64/ld-linux-x86-64.so.2 (0x00007fb6cd427000)

我们发现main程序依赖的动态库名字是libhello.so.0，既不是libhello.so也不是libhello.so.0.0.1。其实在生成main程序的过程有如下几步

* 链接器通过编译命令`-L. -lhello`在当前目录查找libhello.so文件
* 读取libhello.so链接指向的实际文件，这里是libhello.so.0.0.1
* 读取libhello.so.0.0.1中的SONAME，这里是libhello.so.0
* 将libhello.so.0记录到main程序的二进制数据里

也就是说libhello.so.0是已经存储到main程序的二进制数据里的，不管这个程序在哪里，通过ldd查看它依赖的动态库都是libhello.so.0

而为什么这里ldd查看main显示libhello.so.0为not found呢，因为ldd是从环境变量\$LD\_LIBRARY\_PATH指定的路径里来查找文件的，我们指定环境变量再运行如下

     $ export LD_LIBRARY_PATH=. && ldd main                                                                                                                                                                
        linux-vdso.so.1 =>  (0x00007fff7bb63000)
        libhello.so.0 => ./libhello.so.0 (0x00007f2a3fd39000)
        libc.so.6 => /lib64/libc.so.6 (0x00007f2a3f997000)
        /lib64/ld-linux-x86-64.so.2 (0x00007f2a3ff3b000)

## 可执行程序的运行
现在测试目录结果如下
    
    ├── hello.c
    ├── hello.h
    ├── libhello.so -> libhello.so.0.0.1
    ├── libhello.so.0 -> libhello.so.0.0.1
    ├── libhello.so.0.0.1
    ├── main
    └── main.c

这里我们把编译环境和运行环境混在一起了，不过没关系，只要我们知道其中原理，就可以将其理清楚

前面我们已经通过ldd查看了main程序依赖的动态库，并且指定了LD\_LIBRARY\_PATH变量，现在就可以直接运行了

    $ ./main                                                                                                                                                                                              
    hello Handy!

看起来很顺利。那么如果我们要部署运行环境，该怎么部署呢。显然，源代码是不需要的，我们只需要动态库和可执行程序。这里新建一个运行目录，并拷贝相关文件，目录结构如下

    ├── libhello.so.0.0.1
    └── main
    
这时运行会main会发现

    $ ./main                                                                                                                                                                                              
    ./main: error while loading shared libraries: libhello.so.0: cannot open shared object file: No such file or directory

报错说libhello.so.0文件找不到，也就是说**程序运行时需要寻找的动态库文件名其实是动态库编译时指定的SONAME**，这也和我们用ldd查看的一致。通过`ldconfig -n .`建立链接，如下

    ├── libhello.so.0 -> libhello.so.0.0.1
    ├── libhello.so.0.0.1
    └── main

再运行程序，结果就会符合预期了

从上面的测试看出，程序在运行时并不需要知道libxxx.so，而是需要程序本身记载的该动态库的SONAME，所以main程序的运行环境只需要以上三个文件即可

## 动态库版本更新
假设动态库需要做一个小小的改动，如下

    // filename:hello.c
    #include <stdio.h>
    
    void hello(const char* name)
    {
        printf("hello %s, welcom to our world!\n", name);
    }

由于改动较小，我们编译动态库时仍然指定相同的soname

    gcc hello.c -fPIC -shared -Wl,-soname,libhello.so.0 -o libhello.so.0.0.2

将新的动态库拷贝到运行目录，此时运行目录结构如下
    
    ├── libhello.so.0 -> libhello.so.0.0.1
    ├── libhello.so.0.0.1
    ├── libhello.so.0.0.2
    └── main
    
此时目录下有两个版本的动态库，但libhello.so.0指向的是老本版，运行`ldconfig -n .`后我们发现，链接指向了新版本，如下

    ├── libhello.so.0 -> libhello.so.0.0.2
    ├── libhello.so.0.0.1
    ├── libhello.so.0.0.2
    └── main

再运行程序
    
     $ ./main                                                                                                                                                                                              
    hello Handy, welcom to our world!
    
没有重新编译就使用上了新的动态库， wonderful！

同样，假如我们的动态库有大的改动，编译动态库时指定了新的soname，如下

    gcc hello.c -fPIC -shared -Wl,-soname,libhello.so.1 -o libhello.so.1.0.0

将动态库文件拷贝到运行目录，并执行`ldconfig -n .`，目录结构如下

    ├── libhello.so.0 -> libhello.so.0.0.2
    ├── libhello.so.0.0.1
    ├── libhello.so.0.0.2
    ├── libhello.so.1 -> libhello.so.1.0.0
    ├── libhello.so.1.0.0
    └── main
    
这时候发现，生成了新的链接libhello.so.1，而main程序还是使用的libhello.so.0，所以无法使用新版动态库的功能，需要重新编译才行

## 最后
在实际生产环境中，程序的编译和运行往往是分开的，但只要搞清楚这一系列过程中的原理，就不怕被动态库的版本搞晕。简单来说，按如下方式来做

* 编译动态库时指定`-Wl,-soname,libxxx.so.a`，设置soname为libxxx.so.a，生成实际的动态库文件libxxx.so.a.b.c，
* 编译可执行程序时保证libxx.so存在，如果是软链，必须指向实际的动态库文件libxxx.so.a.b.c
* 运行可执行文件时保证libxxx.so.a.b.c文件存在，通过ldconfig生成libxxx.so.a链接指向libxxx.so.a.b.c
* 设置环境变量LD\_LIBRARY\_PATH，运行可执行程序






