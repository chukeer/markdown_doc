Title: C++ const知多少 
Date: 2016-09-06
Modified: 2016-09-06
Category: Language
Tags: c++
Slug: C++ const知多少
Author: littlewhite

###const修饰变量

关于const最常见的一个面试题是这样的：char \*const和const char\*有什么区别，大家都知道const修饰符代表的是常量，即const修饰的变量一旦被初始化是不能被更改的，这两个类型一个代表的是指针不可变，一个代表指针指向内容不可变，但具体哪个对应哪个，很多人一直搞不清楚。

有这样一个规律，const修饰的是它前面所有的数据类型，如果const在最前面，那么把它和它后面第一个数据类行交换.比如上面的const char\*交换之后就是char const \*，这样一来就很清楚了，char \*const p中的const修饰的是char *（注意，我们这里把char和\*都算作一种类型，这时候const修饰的是char和\*的组合，也就是字符串指针），是一个指针类型，所以这时候指针p是不能变的，比如下面这段代码就会报错  
		
	char str1[]="str1";
	char str2[]="str2";
	char *const p = str1;
	p = str2;
这时候p是一个指针常量，它是不能指向别的地方的，但是它本身指向的内容是可以变的，比如下面的操作就是允许的

	char str1[]="str1";
	char *const p = str1;
	p[0] = 'X';
	printf("%s", str1);
这时候str1的值就变成了"Xtr1"  
我们再来看const char \*p，根据前面提到的规律，将const和它后面一个类型交换变成char const \*p（其实这种写法也是允许的，只是人们习惯将const写在最前面），这时候const修饰的是char，也就是说p指向的字符内容是不能变的。将上面两个例子的char \*const p全部改成const char \*p，则结果正好相反，第一个可以编译通过，第二个会报错。

其它时候就很好区分了，比如const int ，const string等等，总之，const修饰的是什么类型，这个类型的变量就不能被改变。
###const修饰函数
先来看这样一个函数  

	const char * func(const char *str) const;  
这样的函数比较夸张，有三个const，我们从左到右来一一说明：  

1、第一个const修饰的是返回值，前面已经说过，这里的const修饰的是char，也就是说返回值的内容是不能被更改的  
2、第二个const和第一个是一样的，这种用的比较多，它作为函数参数，表示的是这个参数在函数体内是不能被改动的（被传进来的实参并不要求是const类型），这样做是为了防止函数对实参做一些意外的操作，你试想下，当你调用一个函数时，你传进去一个变量是"hello world!"，调完函数之后变成了"fuck the world!"，这实在是不可忍的，所以我们在设计函数的时候，如果传进来的参数只作为读取使用，最好是将参数设成const类型。很多公司在面试让写代码的时候都会看中这个细节，你注意了这个细节不一定说明你牛逼，但你若没注意那肯定是会减分的。  
3、再来说第三个const，按照我们最开始说的规律，const修饰的是它前面的所有数据类型，这里它前面的所有数据类型组合起来就是一个函数，这种类型一般出现在类成员函数里，当然，这里并不是说这个函数是不能变的，它代表的时这个函数不能改变类的成员变量，不管是public的还是private的

我们下面举例主要说明第三种情况，来看这样一个简单的程序  

	#include<stdio.h>

	class A
	{
	public:
       	A() : x(0), y(0){}
       	void func(const int p)
       	{
       		x = p;
       		y = p;
       	}
       	int getY()
       	{
       		return y;
       	}
       	int x;
   	private:
   		int y;
   	};

	int main(int argc, char* argv[])
	{
		A a;
		printf("x:%d y:%d\n", a.x, a.getY());
		a.func(2);
		printf("x:%d y:%d\n", a.x, a.getY());
		return 0;
	}
	
这段代码是可以直接编译过的，运行结果是

	x:0 y:0
	x:2 y:2
我们稍作修改，将void func(const int p)改成void func(const int p) const再编译，就会直接报错，报错的两行代码是
	
	x = p;
	y = p;
也就是说const类型的函数试图去修改类的成员变量是非法的，但是有一种情况例外，我们再在上面修改的基础上做一点修改，将int x改成mutable int x，将int y改成mutable int y，这时候程序又可以正常运行了，也就是说，如果成员变量是mutable类型的，它可以在任何场景下被修改。
