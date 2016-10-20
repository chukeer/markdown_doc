Title: gdb调试技巧备忘
Date: 2015-09-11
Modified: 2015-09-11
Category: Skill
Tags: gdb
Slug: gdb调试技巧备忘
Author: littlewhite

[TOC]


## 准备工作
为了能让程序更直观的被调试，在编译时应该添加一些选项

* `-g`: 添加调试选项
* `-ggdb3`: 调试宏定义

## 启动方式
### 不带参数
    gdb ./a.out
    
### 带参数
    gdb ./a.out
    set args -a -b -c any_argument_you_need  
    b main  
    run 

### 调试core文件
    gdb bin_name core_name

### 调试正在运行的程序
大致按如下步骤

1. `ps axu | grep bin_name` ， 获取进程id
2. `gdb attach pid`，启动gdb
3. `b somewhere`，设置断点
4. `c`，继续运行程序

## 基本命令
括号里是命令缩写，详细命令介绍见[http://www.yolinux.com/TUTORIALS/GDB-Commands.html](http://www.yolinux.com/TUTORIALS/GDB-Commands.html)，这里只列出常用的

命令	| 描述
:--- | :---
**查看信息**	|
info break(b)	|查看断点
info threads	|查看线程
info watchpoints	|查看观察点
thread *thread-number*	|进入某个线程
**删除信息**	|
delete(d)	|删除所有断点，观察点
delete(d) *breakpoint-number* </br>delete(d) *watchpoint*	|删除指定断点，观察点
**调试**	|
step(s)	|进入函数
next(n)	|执行一行
until *line-number*	|执行到指定行
continue(c)	|执行到下一个断点/观察点
finish	|执行到函数完成
**堆栈**	|
backtrace(bt)	|打印堆栈
frame(f) *number*	|查看某一帧
up/down	|查看上一帧/下一帧
thread apply all bt	|打印所有线程堆栈信息
**源码**	|
list(l) </br>list *function*	|查看源码，函数
directory(dir) *directory-name	*|添加源码搜索路径
**查看变量**	|
print(p) *variable-name*	|打印变量
p *array-variable@length	|打印数组的前length个变量
p/*format* *variable-name*</br>*format*和printf格式近似</br>d: 整数</br>u: 无符号整数</br>c: 字符</br>f: 浮点数</br>x: 十六进制</br>o: 八进制</br>t: 二进制</br>r: raw格式	|按指定格式打印变量，如p/x variable-name代表以十六进制打印变量
x/nfu *address*</br>nfu为可选的三个参数</br>n代表要打印的数据块数量</br>f为打印的格式，和p/format中一致</br>u为打印的数据块大小，有如下选择</br>b/h/w/g： 单/双/四/八字节，默认为4字节	|按指定格式查看内存数据，如x/7xh address表示从内存地址address开始打印7个双字节，每个双字节以十六进制显示
ptype *variable*	|打印变量数据类型
**运行和退出**	|
run(r)	|运行程序
quit(q)|	退出调试
**设置**	|
set print pretty on/off	|默认off。格式化结构体的打印
set print element 0	|打印完整字符串
set logging file *log-file* |设置日志文件，默认是gdb.txt
set logging on/off	|打开/关闭日志

#### case说明

1. 手动加载源代码
    
    当我们服务器上调试程序时，由于没有加载源码路径而无法查看代码，此时可以将源码目录拷贝到服务器上，然后在gdb调试时通过`dir directory-name`命令加载源码，注意，这里的directory-name一般是程序的makefile所在的路径
    
2. 打印调试信息到日志文件
    
    有时候需要对打印的信息进行查找分析，这种操作在gdb界面不太方便，可以将内容打印到日志，然后通过shell脚本处理。先打开日志调试开关`set logging on`，然后打印你需要的信息，再关闭开关`set logging off`，这期间打印的信息就会被写入gdb.txt文件，如果不想写入这个文件，可以在打开日志开关前先设置日志文件名`set logging file log-file`

## 可视化调试
gdb自带TUI（Text User Interface）模式，详细介绍见[https://sourceware.org/gdb/onlinedocs/gdb/TUI.html](https://sourceware.org/gdb/onlinedocs/gdb/TUI.html)

基本使用方式如下

* `Ctrl-x a`：启动/结束TUI ，启动TUI还可以使用win命令
* `Ctrl-x o`：切换激活窗口
* `info win`：查看窗口
* `focus next / prev / src / asm / regs / split`：激活指定窗口
* `PgUp`：在激活窗口上翻
* `PgDown`：在激活窗口下翻
* `Up/Down/Left/Right`：在激活窗口上移一行/下移一行/左移一列/右移一列
* `layout next / prev`：上一个/下一个窗口布局
* `layout src`：只展示源码窗口
* `layout asm`：只展示汇编窗口
* `layout split`：展示源码和汇编窗口
* `layout regs`：展示寄存器窗口
* `winheight name +count/-count`：调整窗口高度（慎用，可能会让屏幕凌乱）

需要注意的是，在cmd窗口上，原本Up/Down是在历史命令中选择上一条/下一条命令，若想使用该功能，必须先将焦点转移到cmd窗口，即执行focus cmd

TUI的窗口一共有4种，src, cmd, asm, regs， 默认是打开src和cmd窗口，可以通过layout选择不同的窗口布局。最终的效果图是这样的

![](http://littlewhite.us/pic/gdb_tui.png)

可以看到上面是代码区(src)，可以查看当前执行的代码和断点信息，当前执行的代码被高亮显示，并且在代码最左边有一个符号`>`，设置了断点的行最左边的符号是`B`，下面是命令区（cmd），可以键入gdb调试命令

这样调试的时候执行到哪一行代码就一清二楚了，当然，用gdb调试最关键的还是掌握基本命令，TUI只是一中辅助手段

## 打印STL和boost数据结构
当我们要查看某种数据结构的变量，如果gdb不认识该数据结构，它会按照`p/r variable-name`的方式打印数据的原始内容，对于比较复杂的数据结构，比如map类型，我们更关心的是它存储的元素内容，而不是它的数据结构原始内容，还好gdb7.0提供Python接口可以通过实现Python脚本打印特殊的数据结构，已经有一些开源代码提供对boost以及STL数据结构的解析

### 打印STL数据结构
首先查看系统下是否有/usr/share/gdb/python/libstdcxx目录，如果有，说明gdb已经自带对STL数据类型的解析，如果没有可以自己安装，详细介绍见[https://sourceware.org/gdb/wiki/STLSupport](https://sourceware.org/gdb/wiki/STLSupport)，这里简单说明一下

1. svn co svn://gcc.gnu.org/svn/gcc/trunk/libstdc++-v3/python
2. 新建~/.gdbinit，键入如下内容

         python
         import sys
         sys.path.insert(0, '/home/maude/gdb_printers/python')
         from libstdcxx.v6.printers import register_libstdcxx_printers
         register_libstdcxx_printers (None)
         end
         
    其中/home/maude/gdb_printers/python是你实际下载svn代码的路径

###打印boost数据结构
[souceforge](http://sourceforge.net/)上有现成的[boost-gdb-printers](http://sourceforge.net/projects/boost-gdb-printers/)，但根据我的试验发现在打印unordered\_map等数据结构时会报错，因此我做了一些修改并放在github上[https://github.com/handy1989/boost-gdb-printers](https://github.com/handy1989/boost-gdb-printers)，经测试在boost的1.55和1.58版本下均可用

下载boost-gdb-printers，找到里面的boost-gdb-printers.py，修改`boost.vx_y`为实际的版本，并获取文件绝对路径，假设为your_dir/boost-gdb-printers.py，在~/.gdbinit里添加

    source your_dir/boost-gdb-printers.py

这时即可打印boost数据结构，我们用以下代码做一个简单的测试
    
    // filename: gdb_test.cpp
    
    #include <stdio.h>
    #include <string>
    #include <boost/shared_ptr.hpp>
    #include <boost/unordered_map.hpp>
    
    using std::map;
    using std::string;
    
    struct TestData
    {
        int x;
        string y;
    };
    
    void break_here()
    {
    }
    
    int main()
    {
        boost::shared_ptr<TestData> shared_x(new TestData());
        shared_x->x = 100;
        shared_x->y = "hello world";
    
        TestData data1;
        data1.x = 100;
        data1.y = "first";
        TestData data2;
        data2.x = 200;
        data2.y = "second";
    
        boost::unordered_map<int, TestData> unordered_map_x;
        unordered_map_x[1] = data1;
        unordered_map_x[2] = data2;
    
        break_here();
        return 0;
    }

编译如下

    g++ -g gdb_test.cpp -I/your-boost-include-dir

your-boost-include-dir替换为实际的boost头文件所在路径，编写gdb脚本gdb_test.gdb如下

    b break_here
    r
    fin
    
    p/r shared_x
    p shared_x
    
    p/r unordered_map_x
    p unordered_map_x
    
    q
    y
    
执行gdb ./a.out -x gdb_test.gdb，查看变量的输出如下

![](http://littlewhite.us/pic/gdb_test.png)

\$1和\$3分别是shared_ptr和unordered\_map数据类型的原始打印格式，$2和$4是加载boost-gdb-printers之后的打印格式
