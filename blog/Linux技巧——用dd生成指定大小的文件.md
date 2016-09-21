Title: Linux技巧——用dd生成指定大小的文件
Date: 2014-12-07
Modified: 2014-12-07
Category: skill
Tags: linux
Slug: Linux技巧——用dd生成指定大小的文件
Author: littlewhite

[TOC]

我们在测试或调试的时候，有时候会需要生成某个size的文件，比如在测试存储系统时，需要将磁盘剩余空间减少5G，最简单的办法就是拷贝一个5G的文件过来，但是从哪儿去弄这样大小的文件呢，或许你想到随便找一个文件，不停的拷贝，最后合并，这也不失为一种办法，但是有了dd，你会更容易且更灵活的实现

我们来case by case的介绍dd的用法。先看第一个

>生成一个大小为5G的文件，内容不做要求

命令如下

    $ dd if=/dev/zero of=tmp.5G bs=1G count=5

解释一下这里用到的参数

    if=FILE      : 指定输入文件，若不指定则从标注输入读取。这里指定为/dev/zero是Linux的一个伪文件，它可以产生连续不断的null流（二进制的0）
    of=FILE      : 指定输出文件，若不指定则输出到标准输出
    bs=BYTES     : 每次读写的字节数，可以使用单位K、M、G等等。另外输入输出可以分别用ibs、obs指定，若使用bs，则表示是ibs和obs都是用该参数
    count=BLOCKS : 读取的block数，block的大小由ibs指定（只针对输入参数）
    
这样上面生成5G文件的命令就很好理解了，即从/dev/zero每次读取1G数据，读5次，写入tmp.5G这个文件

再看下面一个问题

>将file.in的前1M追加到file.out的末尾

命令如下

    $ file_out_size=`du -b file.out | awk '{print $1}'`
    $ dd if=./file.in ibs=1M count=1 of=./file.out seek=1 obs=$file_out_size
    
这里ibs和obs设置为了不同的值，和前面的命令相比，只多了一个seek参数

    seek=BLOCKS : 在拷贝数据之前，从输出文件开头跳过BLOCKS个block，block的大小由obs指定

命令的意思就是从file.in读取1个1M的数据块写入file.out，不过写入位置并不在file.out的开头，而是在1*$file_out_size字节偏移处（也就是文件末尾）

在此基础上再增加一个要求

>将file.in的第3M追加到file.out的末尾
    
    $ file_out_size=`du -b file.out | awk '{print $1}'`
    $ dd if=./file.in skip=2 ibs=1M count=1 of=./file.out seek=1 obs=$file_out_size
    
这里多了一个参数skip

    skip=BLOCKS : 拷贝数据前，从输入文件跳过BLOCKS个block，block的大小由ibs指定。这个参数和seek是对应的

上面命令的意思就是，从文件file.in开始跳过2*1M，拷贝1*1M数据，写入文件file.out的1*$file_out_size偏移处

这样基本的参数都介绍全了，无非就是设置输入输出文件以及各自的偏移，设置读写数据块大小和读取数据块个数，下面总结一下

    输入参数：
        if
        skip
        ibs
        count
    输出参数：
        of
        seek
        obs
        
最后来一道终极题。前面创建的都是null流，这次换一个

>指定某个字符，创建一个全是这个字符的指定大小的文件。比如创建一个文件，大小为123456字节，每个字节都是字符A

这问题看似没什么意义，但有时候确实需要用到。比如我通过/dev/zero创建了一个1G的文件，但是出于测试需求我想修改中间100M数据，这时我需要创建一个100M的文件，将该文件写入到那个1G文件的指定位置，而这个100M的文件是不能从/dev/zero创建的，否则达不到修改的目的，这时候就需要这样的功能了

话不多说，直接上脚本，有了前面的基础，相信都能看得懂

    #!/bin/bash
    if [ $# -ne 3 ];then
        echo "usage : $0 character out_file file_size(Byte)"
        exit 1
    fi
    
    echo "$1" | grep -q "^[a-zA-Z]$"
    if [ $? -ne 0 ];then
        echo "arg1 must be character"
        exit 1
    fi
    
    character=$1
    out_file=$2
    target_size=$3
    
    # echo输出默认是带'\n'字符的，所以需要通过dd指定输入字节数
    echo "$character" | dd of=$out_file ibs=1 count=1
    while true
    do
        cur_size=`du -b $out_file | awk '{print $1}'`
        if [ $cur_size -ge $target_size ];then
            break
        fi
        remain_size=$((target_size-$cur_size))
        if [ $remain_size -ge $cur_size ];then
            input_size=$cur_size
        else
            input_size=$remain_size
        fi
        dd if=$out_file ibs=$input_size count=1 of=$out_file seek=1 obs=$cur_size || exit 1
    done

有了这些技巧，在对文件内容无要求的前提下，你就可以任意创建指定大小的文件，任意修改文件指定字节数，这会让某些测试场合变得非常方便
