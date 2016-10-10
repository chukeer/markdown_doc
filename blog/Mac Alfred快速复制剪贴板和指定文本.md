Title: Mac Alfred快速复制剪贴板和指定文本
Date: 2016-10-10
Modified: 2016-10-10
Category: Skill
Tags: Mac
Slug: Mac Alfred快速复制剪贴板和指定文本
Author: littlewhite

[TOC]

这里主要考虑如下两种需求

1. 快速唤出剪贴板历史，并复制某一项
2. 快速复制某一段固定内容的文本

第一项在Mac上有很多小工具实现，第二项在输入密码时经常会碰到，比如我在终端sudo执行命令或者连接redis数据库时需要输入密码，这些密码我又不想人肉记住，希望每次要输入时一个快捷键就能搞定，这个有点类似windows上xshell的快速命令集，点一个按钮就可以自动在终端上输入指定的文本，非常方便

这两个需求Alfred都可以轻松搞定

### 打开Alfred剪贴板功能
Alfred的剪贴板功能默认是关闭的，在preferences->Features->Clipboard->History的Clipboard History后面打勾即可，还可以选择剪贴板历史数据保存的时间

![](http://littlewhite.us/pic/Alfred-enable-clipboard-history.jpg)

### 新建snip
snip可以满足需求2，其实就是一个文本片段，并指定一个name和keyword，这样即可通过name或keyword搜索到文本片段，并快速复制粘贴

![](http://littlewhite.us/pic/Alfred-create-snip.jpg)

### 使用
可以通过默认快捷键option+cmd+C唤出剪贴板和snip界面

![](http://littlewhite.us/pic/Alfred-call-clipboard.jpg)

输入关键字可以搜索剪贴板和snip，回车之后会将内容直接粘贴到之前的app上。对于snip还有一个打开方式，就是通过Alfred关键词，先唤出Alfred搜索框，输入默认关键词snip和搜索内容即可对指定的snip进行复制和粘贴

![](http://littlewhite.us/pic/Alfred-call-snip.jpg)

这两种方式选中之后会将内容粘贴到之前的app上，也可以设置选中之后只复制而不粘贴，在preferences->Features->Clipboard->Advanced下去掉Pasting后面的勾即可

![](http://littlewhite.us/pic/Alfred-disable-auto-paste.jpg)

### 小结
之前在Mac上使用iTerm2时很羡慕windows上xshell的快速命令按钮，点一下就可以输入指定文本，现在看来，有了Alfred，Mac才真正称之为Mac
