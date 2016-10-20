Title: vim比较目录diff
Date: 2015-04-27
Modified: 2015-04-27
Category: Skill
Tags: Vim
Slug: vim比较目录diff
Author: littlewhite

[TOC]

虽然现在有很多图形界面的diff工具，但对于有命令行情节的人来说，当飞快的在terminal下敲击键盘时，总不希望再拿鼠标去点击其它地方，况且有时候图形界面占用资源多，我的MBA就经常启动diffmerge时卡住，但vimdiff又只能在一个标签里比较一组文件的diff，如果想比较两个目录下文件的diff，它就显得无能为力了

假设我们要实现一个工具叫diffdir，先让我们脑洞打开设想一下它应该是怎样的

1. 我希望能列出两个目录下文件名相同但内容不同的所有文件，并进行编号
2. 我希望通过选择编号，打开需要比较diff的文件
3. 如果想比较多组文件的diff，我希望每个vim标签打开一组文件比较
4. 最好能过滤掉非文本文件，因为我不希望用vim打开一对二进制乱码
5. 最好还能有交互，我可以选择只查看我感兴趣的文件，而不是一次打开所有文件的diff，当退出vim时我还可以继续选择

假设有两个目录分别是A和B，目录结构如下

    A
    ├── file1
    ├── file2
    └── file3 
    
B和A目录结构以及对应文件名都相同，其中file1和file2的内容不同，file3内容相同，那么当我们运行`diffdir A B`时，它应该是这样的界面

![](http://littlewhite.us/pic/20150427/1.png)

当我们选择编号1时，vim会打开一个标签对比两个目录下file1的差异  
当我们选择`1,2`或`1-2`时，vim会打开两个标签分别比较file1和file2的差异  
由于这个例子有diff的文件数量较少，我们还可以选择a一次打开所有文件的diff  
如果diff文件个数较多，我们可以分批打开，并且当我们退出vim后还可以继续选择

接下来是实现

### vim比较文件diff
我们都知道vimdiff的用法，其实`vimdiff A/file1 B/file1`等价于`vim -d A/file1 B/file2`，又或者更原始一点，我们可以分两步来比较两个文件的diff

1. 执行`vim A/file1`
2. 在normal模式下输入`:vertical diffsplit B/file1`

虽然人们不会用这么麻烦的命令去比价文件的diff，但往往最基本的命令反而能组合出更多的功能，就像搭积木一样，我们只需要几个基本的形状，就可以通过自己的想象搭建多彩的世界，而vim的这些基本命令就像积木一样，我们要做的是利于好这些积木

### vim在新标签比较文件diff
假设我们已经用上面的命令打开了vim并比较file1的diff，如果我们希望新建一个标签来比较file2的diff呢，还是要用到基本的ex命令

1. 在normal模式下执行`:tabnew A/file2`
2. 在normal模式下执行`:vertical diffsplit B/file1`

### vim批量执行命令
以上两个示例就是我们需要的积木，有了积木，我们就可以组合出强大的命令，现在要做的是同时打开两组文件的diff，并且每个标签一组diff

通过查看vim帮助我们发现vim有如下两个参数

    -c <command>         加载第一个文件后执行 <command>
    -S <session>         加载第一个文件后执行文件 <session>


这两个参数都可以让vim启动时执行一些命令，其中-c是从参数读取命令，-S是从文件读取命令，于是我们就可以将需要执行的命令存入文件，启动vim时通过-S参数加载该文件，就能达到我们批量执行命令的目的。假设我们需要打开两个标签，分别比较A，B目录下file1和file2的diff，事先创建vim.script如下（文件名随意，最好采用绝对路径，以免受到vim配置里autochdir的影响）

    edit A/file1
    vertical diffsplit B/file1
    tabnew A/file2
    vertical diffsplit B/file2

然后执行`vim -S vim.script`，看看是否如你所愿，打开了两个标签，分别比较file1和file2的diff。注意，为了

### 最终实现
既然有了这些积木，那我们就可以灵活的根据需要编写脚本实现我们的需求，下面是我最终的实现，也可以在github上查看源码

[https://github.com/handy1989/vim/blob/master/diffdir](https://github.com/handy1989/vim/blob/master/diffdir)

    #!/bin/bash
    if [ $# -ne 2 ];then
        echo "Usage:$0 dir1 dir2"
        exit 1
    fi
    if [ ! -d $1 -o ! -d $2 ];then
        echo "$1 or $2 is not derectory!"
        exit 1
    fi
    
    ## 注意，Mac的readlink程序和GNU readlink功能不同，Mac需要下载greadlink
    arg1=`greadlink -f $1`
    arg2=`greadlink -f $2`
    
    tmp_dir=/tmp/tmp.$$
    rm -rf $tmp_dir
    mkdir -p $tmp_dir || exit 0
    #echo $tmp_dir
    
    trap "rm -rf $tmp_dir; exit 0" SIGINT SIGTERM
    
    ## 注意，Mac和Linux的MD5程序不同，请根据需求使用，这里是Mac版的用法
    function get_file_md5
    {
        if [ $# -ne 1  ];then
            echo "get_file_md5 arg num error!"
            return 1
        fi
        local file=$1
        md5 $file | awk -F"=" '{print $2}'
    }
    
    function myexit
    {
        rm -rf $tmp_dir
        exit 0
    }
    
    function show_diff
    {
        if [ $# -ne 1 ];then
            return 1
        fi
        local diff_file=$1
        echo "diff file:"
        printf "    %-55s  %-52s\n" $arg1 $arg2
        if [ -f $tmp_dir/A_ony_file ];then
            awk '{printf("    [%2d] %-50s\n", NR, $1)}' $tmp_dir/A_ony_file
            python -c 'print "-"*100'
        fi
        awk '{printf("    [%2d] %-50s  %-50s\n", NR, $1, $1)}' $diff_file
        echo "(s):show diff files (a):open all diff files (q):exit"
        echo
    }
    
    function check_value
    {
        local diff_file=$1
        local value=$2
        tmp_file=$tmp_dir/tmp_file
        >$tmp_file
        for numbers in `echo "$value" | tr ',' ' '`
        do
            nf=`echo "$numbers" | awk -F"-" '{print NF}'`
            if [ $nf -ne 1 -a $nf -ne 2 ];then
                return 1
            fi
            begin=`echo "$numbers" | awk -F"-" '{print $1}'`
            end=`echo "$numbers" | awk -F"-" '{print $2}'`
            if [ -z "$end" ];then
                sed -n $begin'p' $diff_file >> $tmp_file
            else
            if [ "$end" -lt $begin ];then
                return 1
            fi
            sed -n $begin','$end'p' $diff_file >> $tmp_file
            fi
            if [ $? -ne 0 ];then
            return 1
            fi
        done
        awk -v dir1=$arg1 -v dir2=$arg2 '{
        if (NR==1)
            {
            printf("edit %s/%s\nvertical diffsplit %s/%s\n", dir1, $0, dir2, $0)
            }
            else
            {
            printf("tabnew %s/%s\nvertical diffsplit %s/%s\n", dir1, $0, dir2, $0)
            }
        }' $tmp_file
    }
    
    #############################################################
    # 获取diff info
    #############################################################
    for file in `find $arg1 | grep -v "/\." | grep -v "^\."`
    do
        file_relative_name=${file#$arg1/}
        file $file | grep -Eq "text"
        if [ $? -ne 0 ];then
            continue
        fi
        if [ -f $arg2/$file_relative_name ];then
            file $arg2/$file_relative_name | grep -Eq "text"
            if [ $? -ne 0 ];then
                continue
            fi
            md5_1=`get_file_md5 $file`
            md5_2=`get_file_md5 $arg2/$file_relative_name`
            if [[ "$md5_1" = "$md5_2" ]];then
                continue
            fi
            ## file not same
            echo "$file_relative_name" >> $tmp_dir/diff_file
        else
            echo "$file_relative_name" >> $tmp_dir/A_ony_file
        fi
    done
    
    #############################################################
    # 根据输入标签打开用vim打开文件比较diff
    #############################################################
    if [ ! -f $tmp_dir/diff_file ];then
        exit
    fi
    
    show_diff $tmp_dir/diff_file
    while true
    do
        echo -n "Please choose file number list (like this:1,3-4,5):"
        read value
        if [[ "$value" = "s" ]] || [[ "$value" = "S" ]];then
            show_diff $tmp_dir/diff_file
            continue
        elif [[ "$value" = "q" ]] || [[ "$value" = "Q" ]];then
            myexit
        elif [[ "$value" = "a" ]] || [ "$value" = "A" ];then
            value="1-$"
        fi
        vim_script=`check_value $tmp_dir/diff_file "$value" 2>/dev/null`
        if [ $? -ne 0 ];then
            echo "invalid parameter[$value]!"
        else
            vim -c "$vim_script"
        fi
    done





