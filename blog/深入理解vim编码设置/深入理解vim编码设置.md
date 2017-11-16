Title: 深入理解vim编码设置
Date: 2017-02-22
Modified: 2017-02-22
Category: Tool
Tags: vim
Slug: understand-vim-encoding
Author: littlewhite

[TOC]

vim的使用环境比较复杂，可以通过terminal在本地使用（比如Mac或Linux主机），也可以ssh连接到远程服务器使用，还可以使用gvim。这里主要讨论terminal下的使用，搞清楚了vim在terminal下的编码设置，gvim相对更简单，自然也就了解了

首先我们要理解字符和字节的区别，字符是用来显示的，而字节是存储和传输时使用，网络传输的是字节流，文件存储的也是字节流，而编辑器要显示文件内容，就需要转化为字符来显示，字符和字节之间的关系可以定义如下

    encode(字符, 编码方案) -> 字节
    decode(字节, 编码方案) -> 字符
    
可见encode和decode是一对逆向操作，它们都需要指定编码方案，如果编码方案不一致，则会操作失败

通过terminal操作远程vim时，其数据流向可以表示如下

    terminal -> 本地shell -> ssh -> 远程shell -> vim
    
在这个流向里，只有terminal和vim需要显示字符，其它进程或服务只是做数据传输，如果只是单纯传递二进制数据，是不需要涉及编码解码的，只有当显示字符的时候才需要进行解码，因此只有terminal和vim需要配置编码，而terminal需要和本地shell打交道，远程vim也需要和shell打交道，shell的编码也至关重要

## Terminal编码
terminal本身也是一个进程，最终的字符显示都需要由terminal来完成，我们在terminal上输入字符也会由它进行编码之后再传递，简单来说就是

1. 输入时，terminal将字符编码为二进制，传递给其它进程或服务
2. 显示时，terminal接收到其它进程或服务的二进制数据流，解码为字符进行显示

这里编解码方案就是terminal需要配置的

## shell编码
locale命令也可查看shell编码设置，以LC\_开头的代表系统不同类别的编码方案，分为如下几类

1. 语言符号及其分类(LC_CTYPE)
2. 数字(LC_NUMERIC)
3. 比较和排序习惯(LC_COLLATE)
4. 时间显示格式(LC_TIME)
5. 货币单位(LC_MONETARY)
6. 信息主要是提示信息,错误信息,状态信息,标题,标签,按钮和菜单等(LC_MESSAGES)
7. 姓名书写方式(LC_NAME)
8. 地址书写方式(LC_ADDRESS)
9. 电话号码书写方式(LC_TELEPHONE)
10. 度量衡表达方式 (LC_MEASUREMENT)
11. 默认纸张尺寸大小(LC_PAPER)
12. 对locale自身包含信息的概述(LC_IDENTIFICATION)。

至于最终选什么方案，其优先级如下

    LC_ALL > LC_* > LANG
    
也就是说一切都以LC\_ALL为主，如果没有设置，则查找LC\_*对应的设置项，如果仍旧没有，则使用LANG的设置，影响字符显示的为LC\_CTYPE项，为了便于描述，后续提到shell编码时一律指LC\_ALL项，设置shell编码方式如下

    export LC_ALL=zh_CN.GBK
    export LC_ALL=zh_CN.UTF-8

> terminal编码和shell编码不一致会出现什么情况？

假设我们本地terminal编码设置为UTF-8，shell编码设置为GBK，当我们在terminal上输入中文字符时，会显示为乱码或不显示

我们分析一下在终端输入shell命令时的数据交互

    输入法 -> terminal -> shell -> terminal
    
1. 在terminal上输入字符，terminal根据自己的编码设置，将字符编码为utf8字节，传递给shell
2. shell收到二进制数据，转码为gbk格式数据，再回传给terminal显示。这一步转码会出错
3. terminal收到二进制数据，按utf8方式解码为字符并显示。这一步解码会出错

将terminal和shell看做两个服务，它们之间需要进行数据交互，在发送数据时进行编码，在收到数据时会进行解码，如果编码方案和解码方案不一致，就会导致乱码或失败，表现形式就是在terminal上输入中文命令时会显示异常，执行结果也不符合预期

如果用ssh登陆远程shell，则远程shell的编码配置和本地shell一致，在通过`ssh -v`可以打印ssh在登陆过程中做了哪些事

因此我们第一个要点是  

>**terminal和shell编码必须设置为一样**

## Vim编码
vim和编码相关的有4个设置项

* `fileencodings` 一个编码列表，以逗号分隔，打开文件时会依次以列表里的编码方式去解码，如果执行成功，则该编码为文件编码，并设置fileencoding
* `fileencoding` 检测到的文件编码
* `encoding` vim内部使用的编码，包括文件内容，寄存器等
* `termencoding` terminal编码，在terminal中使用vim时会用到，gvim不需要设置

可见vim的编码设置相当复杂，我们还是以具体的实例来分析这些编码设置的作用

不管是打开本地vim，还是打开远程vim，我们首先保证本地shell的编码设置和terminal一致，这样涉及到编解码的数据流可以简化为

    terminal -> vim
    
### 打开文件
vim打开文件，最终还是在terminal上显示，这个过程和编码设置相关的有

1. 打开文件，根据fileencodings设置项检测文件编码，并设置fileencoding为对应编码
2. 根据encoding设置项，将文件二进制进行编码转换，存储到内存中
3. 根据termencoding设置项，将内存中的二进制转化为对应编码，传输给terminal
4. terminal根据自身的编码设置，将收到的数据解码并显示

可见vim在打开文件并显示的过程中有大量的编码转化操作，将二进制从编码A转化为编码B的步骤为

    字节流 -> decode(A) -> encode(B) -> 字节流
    
最终输出仍旧为字节流，如果A和B不同，则输出字节流和输入就不一样（ascii字节流除外，在所有编码方案里ascii字符对应的字节流都是一样的）。转换成功的前提是，decode所采用的编码方案必须和输入字节流编码方案一致，也就是说如果输入字节流是采用C编码方案生成的，采用A编码方案去解码就会失败

如果vim的某些编码项没有设置，会使用其依赖项的设置或默认设置，依赖关系如下

    termencoding -> encoding -> shell

vim的这些编码设置项里通常我们只设置fileencodings和encoding，如果只在中英文环境下使用，可设置如下

    set fileencodings=utf8,gbk
    set encoding=utf8
    
encoding一定要设置utf8，因为utf8可以表示所有字符

>terminal编码和vim的encoding编码不一致会出现什么情况？

假设terminal编码设置为gbk，vim的encoding为utf8，此时我们打开一个文件，不管这个文件是utf8还是gbk编码，它都无法正常显示

前面提到，vim的termencoding默认会继承encoding的设置，对应前面打开文件的步骤如下

1. 打开文件，检测文件编码。这里不管是utf8还是gbk都没有影响
2. 将文件内容转化为utf8格式，存储到内存中。这一步由于知道了文件的原始编码，因此转换不会出错
3. 将内存数据转化为termencoding对应的编码，传输给terminal。由于termencoding继承自encoding设置，因此这一步实际上不需要做编码转换
4. terminal按gbk解码。问题出在这一步，由于terminal不知道vim传过来的数据是什么编码，它会直接按照自己的编码设置进行解码，编码不一致导致出错

如果要正常显示，只需要临时修改vim的termencoding编码和terminal编码一致即可，termencoding只涉及到显示，不涉及文件内容的改变，切勿修改encoding项，准确来说，在任何时候都不要试图修改encoding设置

因此我们的第二个要点是

>**vim的termencoding（继承自encoding）必须和terminal编码设置一致**

### 修改文件
如果说打开文件的数据流是从vim到terminal，那修改文件则是从terminal到vim再到terminal这么一个来回

    terminal -> vim -> terminal

和编码相关的步骤如下，打开文件显示的过程前面已经描述过，这里只说修改和保存的过程

1. 在terminal输入字符，根据terminal编码方案进行编码，传输给vim
2. vim收到二进制数据，将数据由termencoding编码转换为encoding编码并保存在内存中
3. 保存文件时，将数据从encoding编码转为fileencoding编码，若fileencoding为空，则直接以encoding方案保存

fileencoding有两种情况

1. 打开空文件，fileencoding默认为空
2. 打开已经存在的文件，fileencoding是根据fileencodings中的编码列表匹配到的编码方案，若都没匹配上，则为空

由上可见，encoding方案编码的数据在vim中是一个中转站，接收数据时（从文件读取或从终端输入）都要转化为encoding编码方案，保存文件时再由encoding编码方案转化为fileencoding编码方案。因此encoding必须设定为一个能表示所有字符的编码方案，通常我们设置为utf8

>vim的encoding编码设置和terminal编码设置不同，如何正常输入文字

假设terminal和shell的编码设置均为gbk，vim的encoding设置为utf8，如果想正常输入和显示字符，必须将termencoding设置和terminal编码一致，这是不管是显示字符还是输入字符保存文件，都可以正常工作

我们可以设置编码不一致只是为了演示编码的影响，在实际环境中，必须保证这些编码设置都一致，因此终极要点是

>**terminal编码，shell编码，以及vim的encoding均设置为utf8**

## vim编码配置终极方案

1. terminal编码设置为utf8
2. shell编码设置为utf8, `export LC_ALL=zh_CN.UTF-8`
3. vim设置encoding为utf8，`set encoding=utf-8`
4. vim设置fileencodings，`set fileencodings=utf-8, gbk`