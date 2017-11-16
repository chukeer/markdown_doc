Title: 详解Linux shell命令帮助格式
Date: 2017-01-14
Modified: 2017-01-14
Category: Skill
Tags: shell
Slug: 详解Linux shell命令帮助格式
Author: littlewhite

[TOC]

linux shell命令通常可以通过`-h`或`--help`来打印帮助说明，或者通过man命令来查看帮助，有时候我们也会给自己的程序写简单的帮助说明，其实帮助说明格式是有规律可循的

## 帮助示例

下面是`git reset`命令的帮助说明，通过`man git-reset`可以查看

    git reset [-q] [<tree-ish>] [--] <paths>...
    git reset (--patch | -p) [<tree-ish>] [--] [<paths>...]
    git reset [--soft | --mixed | --hard | --merge | --keep] [-q] [<commit>]

对于命令和参数大致有如下几种类型

    没有任何修饰符参数 : 原生参数
    <>  : 占位参数
    []  : 可选组合
    ()  : 必选组合
    |   : 互斥参数
    ... : 可重复指定前一个参数
    --  : 标记后续参数类型
    
下面来一一介绍

## 参数类型解读
### 原生参数
说明文档里的字符即为命令需要使用的字符，比如以上命令的
    
    git reset
    
这种参数在使用时必需指定，且和说明文档里的一致

### 占位参数
表示方式：`<>`

和原生参数类似，都是必需指定的，只不过占位参数的实际字符是在使用时指定的，同时为了方便阅读会用一个描述词汇来表示，并以`<>`包围，比如

    <paths>
    
表示路径，使用时可以指定为具体的路径，而paths只是起一个说明作用，有些帮助说明里也会用大写来表示占位参数，比如将以上参数说明写成`PATHS`

### 可选组合
表示方式： `[]`

括号里的参数为可选参数，比如usage第二个里面的`[-q]`，则`-q`为可选参数

可选项和占位参数也可以同时使用，如

    [<commit>]
    
表示该参数可以指定某次提交，也可以不指定

### 必选组合
表示方式: `()`

括号里的参数必需指定，通常里面会是一些互斥参数，比如

    (--patch | -p)
    
表示`--patch`和`-p`这两个参数必需指定一个

### 互斥参数
表示方式: `|`

互斥参数一般都在`()`和`[]`里，表示该参数只能指定其中一个，比如

    [--mixed | --soft | --hard | --merge | --keep]
    
### 重复参数
表示方式: `...`

表示前一个参数可以被指定多个，比如

    <paths>...
    
`<paths>`是一个占位参数，使用时必需指定为路径，`...`并表示可以指定多个路径。重复参数的一个典型使用场景就是移动文件，将多个文件移动到一个目录下，比如如下命令

    git mv [<options>] <source>... <destination>
    
我们可以这样使用
    
    git mv -f a.cpp b.py dir
    
此时options对应为`-f`参数，source对应为`a.cpp b.py`，destination对应为`dir`

### 标记后续参数类型
表示方式: `--`

表示后续参数的某种类型，比如这里如果使用如下命令

    git reset -p -- xx
    
对比第一个命令，这里的xx对应的应该是`<paths>`参数，当我们指定`--`之后，则git会认为xx就是一个路径，那怕它是特殊符号或者路径并不存在。这是shell命令的一个通用方式，比如我们有一个文件名为`-h`，如果想删除这个文件，执行
    
    rm -h
    
肯定是无法删除的，因为这时-h会被认为是rm的一个参数选项，应该使用

    rm -- -h
    
这时shell会将-h解释为一个文件名传递给rm命令

## 解读实战
最后来解释一个比较复杂的帮助说明

    git cat-file (-t [--allow-unknown-type]|-s [--allow-unknown-type]|-e|-p|<type>|--textconv) <object>
    
该命令参数由四个部分，其中`git`和`cat-file`为原生参数，`()`里的为可选组合，`<object>`为占位参数

组合又由6部分组成，为互斥关系

    -t [--allow-unknown-type]
    -s [--allow-unknown-type]
    -e
    -p
    <type>
    --textconv
    
因此该命令的帮助说明可以拆分如下

    git cat-file -t <object>
    git cat-file -t --allow-unknown-type <object>
    git cat-file -s <object>
    git cat-file -s --allow-unknown-type <object>
    git cat-file -e <object>
    git cat-file -p <object>
    git cat-file <type> <object>
    git cat-file --textconv <object>