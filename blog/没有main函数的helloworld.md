#没有main函数的helloworld
几乎所有程序员的第一堂课都是学习helloworld程序，下面我们先来重温一下经典的C语言helloworld

	/* hello.c */
	#include <stdio.h>

	int main()
	{
		printf("hello world!\n");
		return 0;
	}
这是一个简单得不能再单的程序，但它包含有一个程序最重要的部分，那就是我们在几乎所有代码中都能看到的main函数，我们编译成可执行文件并查看符号表，过滤出里面的函数如下（为了方便查看我手动调整了grep的输出的格式，所以和你的输出格式是不一样的）
	
	$ gcc hello.c -o hello
	$ readelf -s hello | grep FUNC
	Num:    Value          Size Type    Bind   Vis      Ndx Name
	27: 000000000040040c     0 FUNC    LOCAL  DEFAULT   13 call_gmon_start
    32: 0000000000400430     0 FUNC    LOCAL  DEFAULT   13 __do_global_dtors_aux
    35: 00000000004004a0     0 FUNC    LOCAL  DEFAULT   13 frame_dummy
    40: 0000000000400580     0 FUNC    LOCAL  DEFAULT   13 __do_global_ctors_aux
    47: 00000000004004e0     2 FUNC    GLOBAL DEFAULT   13 __libc_csu_fini
    48: 00000000004003e0     0 FUNC    GLOBAL DEFAULT   13 _start
    51: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND puts@@GLIBC_2.2.5
    52: 00000000004005b8     0 FUNC    GLOBAL DEFAULT   14 _fini
    53: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND __libc_start_main@@GLIBC_
    58: 00000000004004f0   137 FUNC    GLOBAL DEFAULT   13 __libc_csu_init
    62: 00000000004004c4    21 FUNC    GLOBAL DEFAULT   13 main
    63: 0000000000400390     0 FUNC    GLOBAL DEFAULT   11 _init
		
大家都知道用户的代码是从main函数开始执行的，虽然我们只写了一个main函数，但从上面的函数表可以看到还有其它很多函数，比如_start函数。实际上程序真正的入口并不是main函数，我们以下面命令对hello.c代码进行编译

	$ gcc hello.c -nostdlib
	/usr/bin/ld: warning: cannot find entry symbol _start; defaulting to 0000000000400144
-nostdlib命令是指不链接标准库，报错说找不到entry symbol \_start，这里是说找不到入口符号\_start，也就是说程序的真正入口是_start函数

实际上main函数只是用户代码的入口，它会由系统库去调用，在main函数之前，系统库会做一些初始化工作，比如分配全局变量的内存，初始化堆、线程等，当main函数执行完后，会通过exit()函数做一些清理工作，用户可以自己实现_start函数

	/* hello_start.c */
	#include <stdio.h>
	#include <stdlib.h>

	_start(void)
	{
		printf("hello world!\n");
		exit(0);
	}

执行如下编译命令并运行
	
	$ gcc hello_start.c -nostartfiles -o hello_start
	$ ./hello_start
	hello world!

这里的-nostartfiles的功能是Do not use the standard system startup files when linking，也就是不使用标准的startup files，但是还是会链接系统库，所以程序还是可以执行的。同样我们查看符号表

	$ readelf -s hello_start | grep FUNC
	Num:    Value          Size Type    Bind   Vis      Ndx Name
	20: 0000000000400350    24 FUNC    GLOBAL DEFAULT   10 _start
	21: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND puts@@GLIBC_2.2.5
	22: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND exit@@GLIBC_2.2.5	
	
现在就只剩下三个函数了，并且都是我们自己实现的，其中printf由于只有一个参数会被编译器优化为puts函数，在编译时加-fno-builtin选项可以关掉优化

如果我们在\_start函数中去掉exit(0)语句，程序执行会出core，这是因为\_start函数执行完程序就结束了，而我们自己实现的\_start里面没有调用exit()去清理内存

好不容易去掉了main函数，这时又发现必须得有一个\_start函数，是不是让人很烦，其实\_start函数只是一个默认入口，我们是可以指定入口的

	/* hello_nomain.c */
	#include <stdio.h>
	#include <stdlib.h>

	int nomain()
	{
		printf("hello world!\n");
		exit(0);
	}
	
采用如下命令编译
	
	$ gcc hello_nomain.c -nostartfiles -e nomain -o hello_nomain
	
其中-e选项可以指定程序入口符号，查看符号表如下
	
	$ readelf -s hello_nomain | grep FUNC
	Num:    Value          Size Type    Bind   Vis      Ndx Name
	20: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND puts@@GLIBC_2.2.5
	21: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND exit@@GLIBC_2.2.5
	22: 0000000000400350    24 FUNC    GLOBAL DEFAULT   10 nomain
	
对比hello_start的符号表发现只是将_start换成了nomain

到这里我们就很清楚了，程序默认的入口是标准库里的\_start函数，它会做一些初始化工作，调用用户的main函数，最后再做一些清理工作，我们可以自己写\_start函数来覆盖标准库里的_start，甚至可以自己指定程序的入口