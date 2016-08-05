#python import备忘笔记
python的模块有两种组织方式，一种是单纯的python文件，文件名就是模块名，一种是包，包是一个包含了若干python文件的目录，目录下必须有一个文件`__init__.py`，这样目录名字就是模块名，包里的python文件也可以通过`包名.文件名`的方式import
###import语法
import语法有两种

1. 直接import模块

        import Module
        import Module as xx
2. 从模块import对象（下级模块，类，函数，变量等）

        from Module import Name
        from Module immport Name as yy
        
as语法是用来设置对象（这里用对象泛指模块，类，函数等等）别名，import将对象名字引入了当前文件的名字空间

假设有如下目录结构

    ├── A.py
    └── pkg
        ├── B.py
        └── __init__.py
        
在当前目录下，以下语句都是有效的
    
    import A 
    import pkg
    import pkg.B
    from pkg import B
    
为了简化讨论，下面将不会对as语法进行举例

###import步骤

python所有加载的模块信息都存放在sys.modules结构中，当import一个模块时，会按如下步骤来进行

1. 如果是`import A`，检查sys.modules中是否已经有A，如果有则不加载，如果没有则为A创建module对象，并加载A
2. 如果是`from A import B`，先为A创建module对象，再解析A，从中寻找B并填充到A的`__dict__`中

###嵌套import
在import模块时我们可能会担心一个模块会不会被import多次，假设有A，B，C三个模块，A需要import B和C，B又要import C，这样A会执行到两次import C，一次是自己本身import，一次是在import B时执行的import，但根据上面讲到的import步骤，在第二次import时发现模块已经被加载，所以不会重复import

但如下情况却会报错


    #filename: A.py
    from B import BB
    class AA:pass
    
    #filename: B.py
    from A import AA
    class BB:pass
    
这时不管是执行A.py还是B.py都会抛出ImportError的异常，假设我们执行的是A.py，究其原因如下

1. 文件A.py执行`from B import BB`，会先扫描B.py，同时在A的名字空间中为B创建module对象，试图从B中查找BB
2. 扫描B.py第一行执行`from A import AA`，此时又会去扫描A.py
3. 扫描A.py第一行执行`from B import BB`，由于步骤1已经为B创建module对象，所以会直接从B的module对象的`__dict__`中获取BB，此时显然BB是获取不到的，于是抛出异常

解决这种情况有两种办法，

1. 将`from B import BB`改为`import B`，或将`from A import AA`改为`import A`
2. 将A.py或B.py中的两行代码交换位置

总之，import需要注意的是，尽量在需要用到时再import

###包的import
当一个目录下有`__init__.py`文件时，该目录就是一个python的包 
 
import包和import单个文件是一样的，我们可以这样类比：  
>import单个文件时，文件里的类，函数，变量都可以作为import的对象
>import包时，包里的子包，文件，以及\_\_init\_\_.py里的类，函数，变量都可以作为import的对象
 
假设有如下目录结构

    pkg
    ├── __init__.py
    └── file.py
其中`__init__.py`内容如下
    
    argument = 0
    class A:pass
在和pkg同级目录下执行如下语句都是OK的

    >>> import pkg
    >>> import pkg.file
    >>> from pkg import file
    >>> from pkg import A
    >>> from pkg import argument
但如下语句是错误的

    >>> import pkg.A
    >>> import pkg.argument
报错`ImportError: No module named xxx`，因为当我们执行`import A.B`，A和B都必须是模块（文件或包）

###相对导入和绝对导入
绝对导入的格式为`import A.B`或`from A import B`，相对导入格式为`from . import B`或`from ..A import B`，`.`代表当前模块，`..`代表上层模块，`...`代表上上层模块，依次类推。当我们有多个包时，就可能有需求从一个包import另一个包的内容，这就会产生绝对导入，而这也往往是最容易发生错误的时候，还是以具体例子来说明

目录结构如下

    app    ├── __inti__.py    ├── mod1    │   ├── file1.py    │   └── __init__.py    ├── mod2    │   ├── file2.py    │   └── __init__.py    └── start.py
其中app/start.py内容为`import mod1.file1`   
app/mod1/file1.py内容为`from ..mod2 import file2`

为了便于分析，我们在所有py文件（包括`__init__.py`）第一行加入`print __file__, __name__`

现在app/mod1/file1.py里用到了相对导入，我们在app/mod1下执行`python file1.py`或者在app下执行`python mod1/file1.py`都会报错**<font  color="red">`ValueError: Attempted relative import in non-package`</font>**

在app下执行`python -m mod1.file1`或`python start.py`都会报错**<font  color="red">`ValueError: Attempted relative import beyond toplevel package`</font>**

具体原因后面再说，我们先来看一下导入模块时的一些规则

在没有明确指定包结构的情况下，python是根据`__name__`来决定一个模块在包中的结构的，如果是`__main__`则它本身是顶层模块，没有包结构，如果是`A.B.C`结构，那么顶层模块是A。基本上遵循这样的原则

* 如果是绝对导入，**一个模块只能导入自身的子模块或和它的顶层模块同级别的模块及其子模块**
* 如果是相对导入，**一个模块必须有包结构且只能导入它的顶层模块内部的模块**

有目录结构如下

    A
    ├── B1
    │   ├── C1
    │   │   └── file.py
    │   └── C2
    └── B2
其中A，B1，B2，C1，C2都为包，这里为了展示简单没有列出`__init__.py`文件，当file.py的包结构为`A.B1.C1.file`（注意，是根据`__name__`来的，而不是磁盘的目录结构，在不同目录下执行file.py时对应的包目录结构都是不一样的）时，在file.py中可采用如下的绝对的导入

    import A.B1.C2
    import A.B2
    
和如下的相对导入

    from .. import C2
    from ... import B2
    
什么情况下会让file.py的包结构为`A.B1.C1.file`呢，有如下两种

1. 在A的上层目录执行`python -m A.B1.C1.file`， 此时明确指定了包结构
2. 在A的上层目录建立文件start.py，在start.py里有`import A.B1.C1.file`，然后执行`python start.py`，此时包结构是根据file.py的`__name__`变量来的

再看前面出错的两种情况，第一种执行`python file1.py`和`python mod1/file1.py`，此时file.py的`__name__`为`__main__`，也就是说它本身就是顶层模块，并没有包结构，所以会报错

第二种情况，在执行`python -m mod1.file1`和`python start.py`时，前者明确告诉解释器mod1是顶层模块，后者需要导入file1，而file1.py的`__name__`为mod1.file1，顶层模块为也mod1，所以在file1.py中执行`from ..mod2 import file2`时会报错 ，因为mod2并不在顶层模块mod1内部。通过错误堆栈可以看出，并不是在start.py中绝对导入时报错，而是在file1.py中相对导入报的错

那么如何才能偶正确执行呢，有两种方法，一种是在app上层目录执行python -m app.mod1.file1，另一种是改变目录结构，将所有包放在一个大包中，如下

    app
    ├── pkg
    │   ├── __init__.py
    │   ├── mod1
    │   │   ├── __init__.py
    │   │   └── file1.py
    │   └── mod2
    │       ├── __init__.py
    │       └── file2.py
    └── start.py
    
start.py内容改成`import pkg.mod1.file1`，然后在app下执行`python start.py`



 