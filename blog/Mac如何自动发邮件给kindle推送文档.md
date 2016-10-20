Title: Mac如何自动发邮件给kindle推送文档
Date: 2015-03-06
Modified: 2015-03-06
Category: Skill
Tags: Mac
Slug: Mac如何自动发邮件给kindle推送文档
Author: littlewhite

[TOC]


买过kindle的人一定对于它推送的服务印象深刻，只要你的kindle联网在，即便它被放在家里，你也可以在办公室给它发送书籍，等你回家就会发现，书籍已经自动下载好了，在不同平台下（Mac，windows等）都有相应的Send to kindle应用程序，有些程序是不支持中国亚马逊账户的，但我们可以采用通用的方式，通过邮件推送，可能有些人觉得发邮件很麻烦，但如果能实现自动发送邮件，你是否还这样觉得呢

首先需要准备如下几点

* kindle绑定一个Amazon账号
* 在Amazon账户的个人文档设置里添加自己的邮箱，并获取kindle推送邮箱（为了防止你的kindle收到垃圾推送，只要自己添加的邮箱才能向你的kindle推送）
* 在Mac上创建邮件账号

假设Amazon账号为xx@qq.com，个人邮件账号为xx@qq.com（也可以用其它邮箱），kindle推送邮箱为yy@kindle.cn，现在你在kindle上登录了xx@qq.com，通过邮件推送的流程可以理解为如下几步

1. 用xx@qq.com给yy@kindle.cn发送一封邮件，附件是需要推送的文档
2. 亚马逊邮件服务器收到你发送的邮件，判断xx@qq.com可以向你的kindle推送文档，将文档经过处理（转格式等，比如doc转PDF）后发送给你的kindle设备

试想一下，我下载了电子书在电脑上，点击右键，选择发送给我的kindle，整个过程貌似和Amazon的Send to kindle程序做的事情一样，只不过我们是通过邮箱来实现的，这里要用到的就是Mac上特有的Automator，下面我们一步一步来讲解

## kindle绑定Amazon账户
这一步很简单，很多人在买kindle之前就有Amazon账户，即便没有也没关系，打开kindle，进入主页，按如下顺序设置

*点击右上角菜单键 -> 点击设置选项 -> 点击注册选项 -> 如果已有账号则直接注册，否则注册新账号*

这样你的Amazon账号就和kindle绑定了

## 设置推送邮箱
登陆[z.cn](http://z.cn)，在我的账户标签下选择管理我的设备和内容选项如下

![](http://littlewhite.us/pic/20150306/1.png)

在我的设备选项下找到自己的kindle邮箱，如下

![](http://littlewhite.us/pic/20150306/2.png)

然后在设备选项下找到如下内容，并添加你用来给kindle推送的邮箱

![](http://littlewhite.us/pic/20150306/3.png)

这样便获取到了kindle邮箱和用来给kindle推送的邮箱，比如我这里的kindle邮箱是476114695_95@kindle.cn，用来给kindle推送的邮箱是handy1989@qq.com


## 在Mac上登录邮箱
Mac有自带的邮件APP，只需先在系统偏好设置 -> 互联网账户里添加对应的邮箱就行，如下图

![](http://littlewhite.us/pic/20150306/4.png)

然后打开邮件APP，添加对应的账户，如下图

![](http://littlewhite.us/pic/20150306/5.png)

注意，如果添加的是QQ邮箱，对应密码是QQ邮箱独立密码，必须先去QQ邮箱进行设置，并打开imap和pop3的开关，具体方式是进入网页版QQ邮箱，点击设置，在账户标签下找到如下部分，并开启对应选项，如果事先没有开启，在开启时需要设置独立密码，设置独立密码之后记得检查各项开关是否开启

![](http://littlewhite.us/pic/20150306/6.png)

这样，个人推送账户也设置好了，我这里是handy1989@qq.com，下面就是见证奇迹的时刻！

## 用Automator自动发送邮件
Automator是啥，下面是官方的解释

>这是一款 OS X 附带的 Apple 应用程序，可让您创建工作流程来自动执行重复任务。Automator 可以配合许多其他应用程序使用，包括 Finder、Safari、“日历”、“通讯录”、Microsoft Office 和 Adobe Photoshop

简单来说就是它提供了一些基本的操作，你可以将其组合去自动实现一些简单的任务，比如自动发送邮件，给文件批量重命名，批量压缩照片等等，今天用到的是自动发送邮件

打开Automator，在顶部菜单栏点击文件，新建一个服务

![](http://littlewhite.us/pic/20150306/7.png)

然后设置成下图的样子

![](http://littlewhite.us/pic/20150306/8.png)

新建邮件信息和发送待发邮件两个操作是从左侧的操作栏直接拖过来的，注意顺序。画箭头的地方是必选的

1. 设置服务作用对象为文件或文件夹
2. 设置收件人为你的kindle邮箱
3. 设置发件人地址

这个服务的意思很简单，当你选中一个文件时，点击对应的服务，它会新建一个邮件，将文件作为附件，其中收件人和发件人使用的是你指定的邮箱，然后再将邮件发出去，当然你也可以删除“发送待发邮件”这个标签，自己手动点击发送也可以

cmd+s存储服务，命名为Send to kindle，搞定！

## 验证
随便找一个电子书文档，点击鼠标右键，看看服务菜单里是不是出现了你刚才创建的服务，如下图

![](http://littlewhite.us/pic/20150306/9.png)

点击Send to kindle，该文件就推送到了你的kindle上，你可以在邮件APP的已发出邮件里找到你发送的内容

如果想删掉该服务，只需删掉对应的workflow文件即可，文件存储路径是~/Library/Services，在Finder上点击`前往 -> 前往文件夹`，输入地址可以进入




