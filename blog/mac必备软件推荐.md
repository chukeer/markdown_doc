Title: Mac必备软件推荐 
Date: 2016-09-06
Category: Skill
Tags: curl
Slug: Mac必备软件推荐
Author: littlewhite

随着IOS的流行，Mac电脑也越来越多的进入人们的视野，和iPhone系列一样，苹果的Mac产品线也是软硬件完美结合，有着非同凡响的使用体验，而这主要的功劳，当属其操作系统Mac OS X，今天就来推荐一些Mac必备软件

首先要声明一点，OS X系统的很多软件和IOS一样，都是收费的，国人惯用了微软的盗版系统和大量windows盗版软件，转到Mac平台会有少许不适，当然Mac平台也有破解版软件，但本着程序员的良心，本文不会贴出破解软件的下载链接，对于收费软件也会专门指出，经济条件允许的同学，希望能多多支持正版。我主要是站在程序员的角度推荐软件，所以像QQ、搜狗输入法之类的日常软件不在推荐之列，当然，有些软件也适合普通用户，而且是强烈推荐，希望读者能各取所需

##必备

###Alfred
用神器来形容这款软件一点都不为过，至少我在windows平台还没用过让我这么舒适的软件  

####功能介绍  
初级功能：搜索并打开软件与文件  
高级功能：自定义搜索、通过插件实现特殊功能

Alfred的唤出方式为option+空格，下面的所有操作都是先按option+空格再输入的。Alfred的设计理念是将所有操作都集中到一个入口，这个很类似Linux的shell命令，不管你在任何目录下，所有系统命令都可以通过命令行输入使用，这可以省去你大量的查找和定位时间

####搜索软件  
有了Alfred，你不用去整理安装过的软件，只要你记得它的名字，或者哪怕是一个字母，都可以快速定位并打开软件，比如我要打开QQ，输入qq，它就会给我这样的选项

![](http://littlewhite.us/pic/20141011/alfred_1.png)

通过方向键选择软件，回车可以打开选中的软件，或者通过`cmd+数字`打开对应的软件，它会根据你每次的选择来自动对结果进行排序，因为我经常通过这种方式打开企业QQ，而我的QQ是直接在dock栏打开，所以企业QQ会排在QQ的前面，另外，它搜索软件时会通过两种方式进行匹配，一种是软件名，一种是软件对应的文件名，比如企业QQ的软件名是“企业QQ”，而它的文件名是"EIM.app"，这两种方式都可以用来定位并且对中文支持良好

####搜索文件 
搜索文件的方式大同小异，先输入空格，默认就会搜索文件，比如我输入`空格+python`就会有如下的搜索结果，回车打开文件，cmd+回车打开Finder进入文件所在目录

![](http://littlewhite.us/pic/20141011/alfred_2.png)

####自定义网页搜索
接下来我要推荐它的自定义搜索功能，先看图

![](http://littlewhite.us/pic/20141011/alfred_3.png)

这里我输入`jd iphone`，回车之后就会跳转到京东的iphone搜索页面，也就是这个链接[http://search.jd.com/Search?keyword=iphone&enc=utf-8](http://search.jd.com/Search?keyword=iphone&enc=utf-8)，这里用到了Alfred的web search功能，这需要自己进行配置，配置方式也很简单，打开Alfred的配置界面（`option+空格`打开Alfred，`cmd+,`打开配置项），在feature菜单中选择web search一项，点击右下角的Add Custom Search，按下图配置

![](http://littlewhite.us/pic/20141011/alfred_5.png)

最重要的是Search URL一栏，前面已经说过，京东搜索关键词iphone的链接是[http://search.jd.com/Search?keyword=iphone&enc=utf-8](http://search.jd.com/Search?keyword=iphone&enc=utf-8)，这里我们只需要将链接中的iphone替换成{query}即可，这个链接是怎么发现的呢，很简单，你打开京东，随便输入一个关键词进行搜索（最好是搜英文，中文在URL中会被转码），看一下你输入的词在URL中的哪个地方，替换成{query}就可以了，下图是我自定义的一些搜索以及对应的链接

![](http://littlewhite.us/pic/20141011/alfred_4.png)

	京东   ：http://search.jd.com/Search?keyword={query}&enc=utf-8
	百度   ：http://www.baidu.com/s?wd={query}  
	bt天堂 ：http://www.bttiantang.com/s.php?q={query}  
	豆瓣电影：http://movie.douban.com/subject_search?search_text={query}  
	淘宝   ：http://s.taobao.com/search?q={query}  
有了这个，你就可以在任何界面下快速进行搜索，比如你在看一个PDF文档发现一个专有名词想用百度搜索，这时你无须打开浏览器进入百度再输入关键词，而是`option+空格`打开Alfred，输入`bd 你想要的balabala`就可以快速搜索

以上功能都是免费的！应付日常使用完全够了，如果想用高级功能，比如通过编写插件完成更复杂的动作，就需要升级到专业版，个人觉得免费版就已经够用了，除非你想深入研究这个东东的使用
##效率

###BetterTouchTool
这是一款免费软件，可以自定义触摸板和鼠标操作，添加操作的步骤如下

![](http://littlewhite.us/pic/20141011/BetterTouchTool_1.png)

	1. 选择操作的对象，可以对Magic Mouse，触摸板等进行操作
	2. 选择动作执行的对象，可以是全局动作，也可以是针对某个应用的动作
	3. 添加手势
	4. 选择手势
	5. 选择映射的快捷键或操作，二选一

这个软件全是英文说明，需要一点耐心来看，不过都是一些简单句子，相信英语过了四级的理解起来完全无压力。通过上图可以看到，我在全局范围添加了两个手势，分别轻按触摸板顶部中间位置和底部中间位置可以滚动到页面顶部或底部，滚动到页面顶部或底部是我在windows浏览器上最常用的鼠标手势，Mac下虽然没有那些浏览器插件和鼠标可用，但是通过这种方式我们可以实现同样的功能，甚至更加强大，这个动作是对所有软件都有效的！

同理，我们也可以对MagicMouse进行设置，注意必须是苹果的MagicMouse，普通鼠标是不支持的。MagicMouse的动作和触摸板会有所不同，细节就不说了，总之你可以将常用的操作全部集成到鼠标上，那时你就会明白为什么MagicMouse叫做MagicMouse。不了解MagicMouse的人会吐槽它很难用，了解的人只会暗自偷笑

另外，在Basic Settings标签下，建议将左下角的Enable Windows Snapping勾选上，这样可以实现和win7类似的将软件窗口拖到屏幕顶端实现放大的功能，除此之外，你还可以试试将软件窗口拖到屏幕左边、右边以及四个角落，看看是什么效果
###AppClean
轻量级的卸载软件的工具，在windows下如果要卸载软件该如何操作？通过控制面板？那个太高端，很多普通用户都不会使用。通过360安全卫士？拜托，那简直就是一个杂货店，我只想要一瓶啤酒，它非得送我一包卫生纸。Mac下完全不需要像360安全卫士这样臃肿的软件，Unix软件设计的宗旨是只干一件事并做到极致，实现软件卸载，只需要AppClean就可以了

通过Alfred启动软件（现学现用嘛，option+空格唤出Alfred，输入cleaner，回车打开软件），如下图  

![](http://littlewhite.us/pic/20141011/AppCleaner_1.png)

它的搜索功能颜色比较淡，我好长时间才发现，通过搜索找到你要卸载的软件，或者直接在列表里找到，勾选之后点击右下角的Search按键，它会搜索出软件相关的目录，点击delete，搞定！

是不是觉得简单的不可思议，印象中windows下卸载一个软件得花老半天，其实卸载软件无非就是删除文件，在Mac下，软件包含的文件被有规律的组织在一起，这使得安装和卸载都变得异常简单

最后需要注意一点，AppCleaner的搜索功能只能对软件的文件名进行搜索，对于有些软件名和文件名不一致的，输入软件名是搜不到的，比如企业QQ的文件名是EIM.app，只能通过搜索EIM找到软件，或者浏览软件列表选中，至于怎么通过软件名得到文件名，试试Alfred：）
###PhoneClean
如果想深度清理系统垃圾，就需要用到这个软件，收费软件，这里不做过多介绍，使用起来非常容易
###AndroidFileTransfer
浏览安装设备文件，无须多言
###Windows Phone
同步WP设备文件，无须多言

##开发

前面推荐的软件是适用于所有用户的，所以讲的比较详细，有些还贴出了使用步骤截图，下面介绍专门针对程序员的软件，由于程序员都有极强的动手能力和好奇心，所以下面的软件介绍都一笔带过，只做推荐，不做详解
###Xcode
IOS开发必备，即便不做IOS开发，也建议安装，它就像windows下的VS，可能其它软件使用时会依赖它，所以强烈建议安装，AppStore可免费下载
###iTerm
终端模拟程序，虽然Mac自带Terminal程序，但这个更带感配置也更丰富，光看这个透明背景就让人醉了，更重要的是它是免费的！

![](http://littlewhite.us/pic/20141011/iterm_1.png)

###MacVim
vim的GUI版，Mac专有，完美兼容vim所有插件以及语法，vim遇到Mac，是我用过的最好的编辑器！

想要最大发挥它的威力，前提是你必须是一个Vimer，建议先熟练使用vim后再转到MacVim
###Homebrew
二进制包管理工具，类似Ubuntu的apt-get和CentOS的yum。可以通过它安装很多Mac没提供或提供了但不好使的UNIX软件，比如ctags，wget，git等  

官网可下载[http://brew.sh](http://brew.sh) 
 
安装brew

	$ ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
	
通过brew安装软件

	$ brew install wget
`brew -h`查看详细使用说明
###DiffMerge
文件/目录比较工具。虽然vim很强大，也可提供文件比较功能，但这种场景下图形界面会更直观
###Mou
最后登场的是Mou，免费软件，基于Markdown语法的编辑器，我觉得我有必要专门花一篇文章来讲它，原因只有一个，我的所有博客都是用它来写的！但，今天就到这里了
  
EOF
