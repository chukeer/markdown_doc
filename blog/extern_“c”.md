Title: extern “C”用法详解
Date: 2016-09-06
Modified: 2016-09-06
Category: Language
Tags: c++
Slug: extern “C”用法详解
Author: littlewhite

今天是接着昨天谈extern的用法，纯技术贴。目前用户数以每天1-2的数量在增长，突破100不知何时到头啊，不过昨天的文章阅读数竟然超过了用户数，看来宣传宣传还是有用的，而且看到有更多人阅读，也给了我更大写作的动力，于是我决定不定期的在这里发放小米F码！周围有朋友有需求的赶紧号召过来关注哇，不过数量有限，每次发放一个，我会提前一天预告，第二天文章推送时将F码奉上，如果你看到F码并且也需要，请赶紧使用，不然有可能被别人抢走的哦^_^ ，明天要发放的F码是<font color="red">**米4联通3G版**</font>
<hr>

简单来说，extern “C”是C++声明或定义C语言符号的方法，是为了与C兼容。说来容易，要理解起来还是得费些周折，首先我们要从C++和C的区别说起。

符号
---
大家都知道，从代码到可执行程序需要经过编译和链接两个过程，其中编译阶段会做语法检测，代码展开，另外它还会做一件事，就是将变量转成符号，链接的时候其实是通过符号来定位的。编译器在编译C和C++代码时，将变量转成符号的过程是不同的。本文所使用的编译器为gcc4.4.7
	
我们先来看一段简单的代码

	/* hello.c */
	#include <stdio.h>
	
	const char* g_prefix = "hello ";
	
	void hello(const char* name)
	{
		printf("%s%s", g_prefix, name);
	}
	
注意，这里的文件名为hello.c，我们执行编译`gcc -c hello.c`得到目标文件hello.o，在Linux下用nm查看目标文件的符号表得到如下结果(`$`符号代表shell命令提示符)

	$ nm hello.o
	0000000000000000 D g_prefix
	0000000000000000 T hello
	                 U printf
这是C代码编译后的符号列表，其中第三列为编译后的符号名，我们主要看自己定义的全局变量g_prefix和函数hello，它们的编译后的符号名和代码里的名字是一样的。我们将hello.c重命名为hello.cpp，重新编译`gcc -c hello.cpp`得到hello.o，在用nm查看，结果如下

	0000000000000000 T _Z5helloPKc
	                 U __gxx_personality_v0
	0000000000000000 D g_prefix
	                 U printf
这是C++代码编译后的符号列表，gcc会自动根据文件后缀名来识别C和C++代码，这时我们发现g_prefix的符号没变，但函数hello的符号变成了`_Z5helloPKc`，这就说明gcc在编译C和C++代码时处理方式是不一样的，对于C代码，变量的符号名就是变量本身（在早期编译器会为C代码变量前加下划线`_`，现在默认都不会了，在编译时可以通过编译选项`-fno-leading-underscore`和`-fleading-underscore`来显式设置），而对于C++代码，如果是数据变量并且没有嵌套，符号名也是本身，如果变量名有嵌套（在名称空间或类里）或者是函数名，符号名就会按如下规则来处理
 
1、 符号以`_Z`开始  
2、 如果有嵌套，后面紧跟`N`，然后是名称空间、类、函数的名字，名字前的数字是长度，以`E`结尾  
3、 如果没嵌套，则直接是名字长度后面跟着名字  
4、 最后是参数列表，类型和符号对应关系如下  

		int    -> i  
		float  -> f  
		double -> d  
		char   -> c  
		void   -> v  
		const  -> K  
		*      -> P  
		
这样就很好理解为什么C++代码里的void hello(const char\*)编译之后符号为_Z5helloPKc（PKc翻译成类型要从右到左翻译为`char const *`，这是编译器内部的表示方式，我们习惯的表示方式是`const char*`，两者是一样的），`c++filt`工具可以从符号反推名字，使用方法为`c++filt _Z5helloPKc`

下面列举几个函数和符号的对应例子

函数和变量            | 符号
:-------------:| :----------:
int func(int, int)  | _Z4funcii
float func(float) | _Z4funcf
int C::func(int) | _ZN1C4funcEi
int C::C2::func(int) | _ZN1C2C24funcEi
int C::var | _Z1C3varE

这样也很容易理解为什么C++支持函数重载而C不支持了，因为C++将函数修饰为符号时把函数的参数类型加进去了，而C却没有，所以在C++下，即便函数名相同，只要参数不同，它们的符号名是不会冲突的。我们可以通过下面一个例子来验证变量名和符号的这种关系

	/ * filename : test.cpp */
	#include <stdio.h>
	
	namespace myname
	{
		int var = 42;
	}
	
	extern int _ZN6myname3varE;
	
	int main()
	{
		printf("%d\n", _ZN6myname3varE);
		return 0;
	}	
这里我们在名称空间namespace定义了全局变量var，根据前面的内容，它会被修饰为符号`_ZN6myname3varE`，然后我们手动声明了外部变量`_ZN6myname3varE`并将其打印出来。编译并运行，它的值正好就是var的值

	$ gcc test.cpp -o test -lstdc++
	$ ./test
	42

##extern "C"
有了符号的概念我们再来看extern “C”的用法就很容易了

	extern "C"
	{
		int func(int);
		int var;
	}
它的意思就是告诉编译器将extern “C”后面的括号里的代码当做C代码来处理，当然我们也可以以单条语句来声明

	extern "C" int func(int);
	extern "C" int var;
这样就声明了C类型的func和var。很多时候我们写一个头文件声明了一些C语言的函数，而这些函数可能被C和C++代码调用，当我们提供给C++代码调用时，需要在头文件里加extern “C”，否则C++编译的时候会找不到符号，而给C代码调用时又不能加extern “C”，因为C是不支持这样的语法的，常见的处理方式是这样的，我们以C的库函数memset为例

	#ifdef __cplusplus
	extern "C" {
	#endif
	
	void *memset(void*, int, size_t);
	
	#ifdef __cplusplus
	}
	#endif
	
其中`__cplusplus`是C++编译器定义的一个宏，如果这份代码和C++一起编译，那么memset会在extern "C"里被声明，如果是和C代码一起编译则直接声明，由于`__cplusplus`没有被定义，所以也不会有语法错误。这样的技巧在系统头文件里经常被用到。
<hr>
点击阅读原文查看我的博客，如果觉得本文有价值，请为我点个赞，或者为我增加一个读者
