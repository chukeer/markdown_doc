<<<<<<< HEAD
# cppcheck使用方式详解
=======
Title: cppcheck使用方式详解 
Date: 2017-11-09
Modified: 2017-11-09
Category: Tool
Tags: cppcheck
Slug: cppcheck使用方式详解
Author: littlewhite

[TOC]
>>>>>>> deafb6bb75f8350a7d4f1bebb85ba7f79f9f4369

顾名思义，cppcheck是对C++代码的静态检查工具，只需对源代码进行静态扫描，即可发现一些隐患，包括如下分类

* Dead pointers
* Division by zero
* Integer overflows
* Invalid bit shift operands
* Invalid conversions
* Invalid usage of STL
* Memory management
* Null pointer dereferences
* Out of bounds checking
* Uninitialized variables
* Writing const data

并按严重程度又进行了如下区分

* error: 直接判定为bug
* warning: 可能会引起bug
* style: 代码风格的问题，如包含未使用函数，重复代码等
* performance: 性能相关的问题，如函数参数中复杂对象的值传递
* portability: 代码兼容性相关的问题
* information: 可以忽略不看

以上警告的准确率并非100%，我们还需人为的去对照代码具体分析

## 基本使用方式
### 检查文件和目录
检查单个文件

	cppcheck file1.cpp

检查目录下所有文件

	cppcheck path

检查目录下所有文件，排除某些目录

	cppcheck -i path/sub1 -i path/sub2 path

### 参数选项介绍
#### 只打印扫描结果
**`-q, --quiet`**
#### 指定消息级别
**`--enable`**

指定展示消息级别，默认只展示error类型的消息，展示warning和performance如下

	cppcheck --enable=warning --enable=performance path

#### 并发扫描
**`-j`**

多进程扫描，和make的选项含义相同

#### 输出不确定项

**`--inconclusive`**

cppcheck只会输出它自己确定的问题，使用该参数相当于放宽对bug的标准

#### 指定源代码平台

**`--platform`**

可指定为unix32, unix64, win32A, win32W, win64，默认为cppcheck编译所使用平台

#### 以xml格式输出

**`--xml-version`**

可指定为1和2

	cppcheck --xml-version=2 file1.cpp

#### 格式化输出信息

**`--template`**

提供的模板包括vs和gcc

比如`cppcheck --template=vs gui/test.cpp`输出格式为

	Checking gui/test.cpp...
	gui/test.cpp(31): error: Memory leak: b
	gui/test.cpp(16): error: Mismatching allocation and deallocation: k

`cppcheck --template=gcc gui/test.cpp`输出格式为

	Checking gui/test.cpp...
	gui/test.cpp:31: error: Memory leak: b
	gui/test.cpp:16: error: Mismatching allocation and deallocation: k

也可以自定义格式，`cppcheck --template="{file},{line},{severity},{id},{message}" gui/test.cpp`

	Checking gui/test.cpp...
	gui/test.cpp,31,error,memleak,Memory leak: b
	gui/test.cpp,16,error,mismatchAllocDealloc,Mismatching allocation and deallocation

可使用的标记包括

标记名 | 含义
:--: | :--:
callstack | callstack - if available
file | filename
id | message id
line | line number
message | verbose message text
severity | a type/rank of message

#### 忽略指定类型的消息
按如下方式定义一个消息类型

	[error id]:[filename]:[line]
	[error id]:[filename2]
	[error id]

**`--suppress`**

通过命令行指定

	cppcheck --suppress=memleak:src/file1.cpp src/

**`--suppressions-list`**

通过文件指定，假设有文件suppressions.txt内容如下

	// suppress memleak and exceptNew errors in the file src/file1.cpp
	memleak:src/file1.cpp
	exceptNew:src/file1.cpp

	// suppress all uninitvar errors in all files
	uninitvar

使用方式如下

	cppcheck --suppressions-list=suppressions.txt src/

**`--inline-suppr`**

在源代码中指定，假设如下文件

	void f() {
		char arr[5];
		arr[10] = 0;
	}

扫描报错为

	# cppcheck test.c
	Checking test.c...
	[test.c:3]: (error) Array 'arr[5]' index 10 out of bounds

将文件修改如下

	void f() {
		char arr[5];
		// cppcheck-suppress arrayIndexOutOfBounds
		arr[10] = 0;
	}

采用如下命令可屏蔽该错误

	cppcheck --inline-suppr test.c

##结果分析
将扫描结果存入xml文件，再通过cppcheck-htmlreport工具可展示为html格式，方便查看错误信息以及对应源代码，cppcheck-htmlreport是一个Python脚本，可从cppcheck的github仓库下载[https://github.com/danmar/cppcheck.git](https://github.com/danmar/cppcheck.git)


	cppcheck --quiet --std=c++03 --platform=unix64 \
		--language=c++ -j 8 --enable=warning --enable=performance  \
		--xml-version=2 src/ > cppcheck-result.xml 2>&1
	
	cppcheck-htmlreport --file cppcheck-result.xml \
		--source-dir=. \
		--report-dir=/usr/share/nginx/html/cppcheck/ \
		--title=balabala

cppcheck-htmlreport参数选项说明如下

	--file: cppcheck输出结果文件
	--source-dir: 源代码路径，如果cppcheck扫描的是src目录，则这里为src的父目录
    --report-dir: 报告存储目录
	--titie: 报告的标题名字

展示结果如下

<<<<<<< HEAD
![](images/cppcheck.png)
=======
![](http://littlewhite.us/pic/cppcheck.png)
>>>>>>> deafb6bb75f8350a7d4f1bebb85ba7f79f9f4369

## 集成到其它工具
cppcheck可以和git, svn, jenkins等平台和工具结合，在svn和git commit之前执行相应脚本，或者在jenkins项目构建之后执行相应检查，具体用法参考以下链接

* CLion - [Cppcheck plugin](https://plugins.jetbrains.com/plugin/8143-cppcheck)
* Code::Blocks - integrated
* CodeDX (software assurance tool) - [integrated](http://codedx.com/code-dx-standard/)
* CodeLite - integrated
* CppDepend 5 - [integrated](http://www.cppdepend.com/CppDependV5.aspx)
* Eclipse - [Cppcheclipse](https://github.com/kwin/cppcheclipse/wiki/Installation)
* KDevelop - [integrated since v5.1](https://kdevelop.org/)
* gedit - [gedit plugin](http://github.com/odamite/gedit-cppcheck)
* Hudson - [Cppcheck Plugin](http://wiki.hudson-ci.org/display/HUDSON/Cppcheck+Plugin)
* Jenkins - [Cppcheck Plugin](http://wiki.jenkins-ci.org/display/JENKINS/Cppcheck+Plugin)
* Mercurial (Linux) - [pre-commit hook](http://sourceforge.net/p/cppcheck/wiki/mercurialhook/) - Check for new errors on commit (requires interactive terminal)
* Tortoise SVN - [Adding a pre-commit hook script](http://omerez.com/automatic-static-code-analysis/)
* Git (Linux) - [pre-commit hook](https://github.com/danmar/cppcheck/blob/master/tools/git-pre-commit-cppcheck) - * * Check for errors in files going into commit (requires interactive terminal)
* Visual Studio - [Visual Studio plugin](https://github.com/VioletGiraffe/cppcheck-vs-addin/releases/latest)
* tCreator - [Qt Project Tool (qpt)](https://sourceforge.net/projects/qtprojecttool/files)

## 参考文档
* cppcheck官网 [http://cppcheck.net/](http://cppcheck.net/)
<<<<<<< HEAD
* cppcheck1.8.1使用文档 [http://cppcheck.net/manual.pdf](http://cppcheck.net/manual.pdf)
=======
* cppcheck1.8.1使用文档 [http://cppcheck.net/manual.pdf](http://cppcheck.net/manual.pdf)
>>>>>>> deafb6bb75f8350a7d4f1bebb85ba7f79f9f4369
