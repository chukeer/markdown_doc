Title: vim自动更新tag
Date: 2015-04-10
Modified: 2015-04-10
Category: Skill
Tags: Vim
Slug: vim自动更新tag
Author: littlewhite

[TOC]

用vim浏览C/C++代码时可以用ctags生成tag文件，这样可以很方便跳转到函数定义的地方，这个功能几乎所有的图形界面编辑器都有，比如VS，source insight等等，但是vim的tags文件是静态的，也就是说如果我们在源代码里新增了一些函数，原来的tags是不会自动更新的，我们也无法跳转到新增的函数定义处，这个问题怎么怎么办呢

我在网上搜索了很多地方，普遍给的方案就是将ctags命令映射到一个快捷键，这样只需要按一下快捷键就会生成新的tags文件，但这样有几个不方便的地方

1. 每次tags文件都是全量生成，如果工程很大，生成tags文件可能需要十多秒，而运行命令的过程中是不能编辑文件的，也不可能每次修改文件都去更新tags
2. vim下运行ctags命令其实是在命令模式下输入`!ctags -R .`，它会在vim的工作目录下生成tags文件，而如果你当前工作目录并不是你想要生成tags的目录，还得切换目录

总之是，想要自动更新tags，没有这么简单的事儿！

但是VS和source insight就可以做到，我们秉着凡是其它编辑器能实现的功能vim都能实现的原则来分析下问题的实质，其实不管是VS还是source insight它们都需要建立工程，然后将源代码导入工程，我们可以猜想到这些编辑器会对工程里的源文件建立索引，这样就可以实现各种跳转功能，当有代码更新或是新增源文件时，编辑器自然也可以检测到，这时它暗地里对源文件重新建立索引，我们就可以对新增的函数进行跳转。既然编辑器可以暗地里做很多事，我们为何不也这样呢，计算机的处理器大多数时候都是空闲的，不用白不用

试想我们在Linux下有一堆源代码，我们需要经常编辑和阅读这些代码，它的根目录结构应该是这样的

    ├── auto_tags
    ├── build
    ├── common
    ├── include
    ├── libs
    ├── message
    ├── metadata
    ├── network
    ├── nodes
    ├── privacy
    ├── tags
    └── tests

其中tags文件是对当前目录下的代码生产的tag，其它目录存放的则是你的源代码，这样当我们打开目录下的源代码时，vim就可以根据tags文件来定位变量的位置

这里的auto_tags目录存放的是自己实现的脚本，它的功能是自动检测当前目录下的源代码是否有更新，如果有，则生产新的tags文件并替换老的，auto_tags目录结构如下

    ├── auto_tags.conf
    ├── auto_tags.sh
    └── run_tags.sh
    
auto_tags.conf为配置文件，配置项如下

    CTAGS=/usr/bin/ctags
    
    # 生成tags文件存放的目录
    tags_dir=../
    
    # 源代码所在目录
    source_root=../
    
    # 需要创建tags的目录名，注意只有目录名字，不是路径
    source_dirs="common message nodes"
    
    # 日志相关
    max_log_size=10 # Unit: Mb
    log_file="auto_tags.log"
    

将source_dirs变量替换为你需要建立tags的目录名称即可，注意需要用双引号包围，且只写目录名字，不需要添加`../`

使用方式如下

     sh ./run_tags.sh start # 启动脚本
     sh ./run_tags.sh stop  # 停止脚本

核心脚本是auto_tags.sh，至于脚本是如何实现的就不贴出来了，毕竟这只是一个程序员自娱自乐实现的一个小小的功能，它并不完善但简单易用。github地址如下

    https://github.com/handy1989/vim/tree/master/auto_tags

