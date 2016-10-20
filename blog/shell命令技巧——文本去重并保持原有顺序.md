Title: shell命令技巧——文本去重并保持原有顺序
Date: 2015-01-18
Modified: 2015-01-18
Category: Skill
Tags: shell
Slug: shell命令技巧——文本去重并保持原有顺序
Author: littlewhite

[TOC]

简单来说，这个技巧对应的是如下一种场景

假设有文本如下

    cccc
    aaaa
    bbbb
    dddd
    bbbb
    cccc
    aaaa
    
现在需要对它进行去重处理，这个很简单，sort -u就可以搞定，但是如果我希望保持文本原有的顺序，比如这里有两个aaaa，我只是希望去掉第二个aaaa，而第一个aaaa在bbbb的前面，去重后仍旧要在它前面，所以我期望的输出结果是

    cccc
    aaaa
    bbbb
    dddd
    
当然，这个问题本身并不难，用C++或python写起来都很容易，但所谓杀机焉用牛刀，能用shell命令解决时，它永远都是我们的首选。答案在最后给出，下面说说我是如何想到这样

我们有时候想把自己的目录加入环境变量PATH时会在~/.bashrc文件中这样写，比如待加入的目录为\$HOME/bin

    export PATH=$HOME/bin:$PATH
    
这样我们等于是在PATH追加了路径\$HOME/bin并让它在最前面被搜索到，但当我们执行`source ~/.bashrc`后，\$HOME/bin目录就会被加入PATH，如果我们下次再添加一个目录，比如

    export PATH=$HOME/local/bin:$HOME/bin:$PATH

再执行`source ~/.bashrc`时，\$HOME/bin目录在PATH中其实会有两份记录，虽然这不影响使用，但对于一个强迫症来说，这是无法忍受的，于是问题就变成了，我们需要去掉$PATH里重复的路径，并且保持原有路径顺序不变，也就是原本谁在前面，去重后仍旧在前面，因为在执行shell命令时是从第一个路径开始查找的，所以顺序很重要

好了，说了这么多我们来揭示最终的结果，以文章开始的数据为例，假设输入文件是in.txt，命令如下

    cat -n in.txt | sort -k2,2 -k1,1n | uniq -f1 | sort -k1,1n | cut -f2-

这些都是很简单的shell命令，下面稍作解释

    cat -n in.txt : 输出文本，并在前面加上行号，以\t分隔
    sort -k2,2 -k1,1n : 对输入内容排序，primary key是第二个字段，second key是第一个字段并且按数字大小排序
    uniq －f1 : 忽略第一列，对文本进行去重，但输出时会包含第一列
    sort -k1,1n : 对输入内容排序，key是第一个字段并按数字大小排序
    cut -f2- : 输出第2列及之后的内容，默认分隔符为\t
    
大家可以从第一条命令开始，并依次组合，看看实际输出效果，那样便更容易理解了。对于$PATH中的重复路径又该如何处理呢，还是以前面的例子来说，只需在前后用tr做一下转换即可

    export PATH=$HOME/local/bin:$HOME/bin:$PATH
    export PATH=`echo $PATH | tr ':' '\n' | cat -n | sort -k2,2 -k1,1n | uniq -f1 | sort -k1,1n | cut -f2- | tr '\n' ':'`
    
其实这样使用PATH会有个问题，比如我们执行了以上命令后，如果想去掉$HOME/bin这个路径，仅仅修改为如下内容是不够的

    export PATH=$HOME/local/bin:$PATH
    export PATH=`echo $PATH | tr ':' '\n' | cat -n | sort -k2,2 -k1,1n | uniq -f1 | sort -k1,1n | cut -f2- | tr '\n' ':'`
    
因为我们已经将$HOME/bin加入了$PATH中，这样做并没有起到删除的作用，也许最好的方式还是自己清楚的知道所有路径，然后显示指定，而不是采取追加的方式