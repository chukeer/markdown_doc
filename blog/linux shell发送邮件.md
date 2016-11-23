Title: linux shell发送邮件
Date: 2016-11-23
Modified: 2016-11-23
Category: Skill
Tags: linux, shell
Slug: linux shell发送邮件
Author: littlewhite

[TOC]

## 一封最简单的邮件
    echo -e "To: handy1989@qq.com\nCC: handy1989@qq.com\nFrom: handy<handy@test.com>\nSubject: test\n\nhello world" | sendmail -t
    
看上去很复杂，其实就是sendmail程序从标准输入读取邮件源码，-t参数表示从邮件源码提取收件人信息，然后发送到收件人的邮件服务器，我们稍做整理，将邮件源码保存在email.txt中如下

    To: handy1989@qq.com
    CC: handy1989@qq.com
    From: handy<handy@test.com>
    Subject: test
    
    hello world

将以上命令改为`cat email.txt | sendmail -t`，这样就一目了然了。收到的邮件信息如下

![](http://littlewhite.us/pic/mail-helloworld.png)

## 邮件的格式
从前面的邮件源码可以看到，邮件是和http类似的文本协议，由邮件头和邮件内容两部分组成，中间以空行分隔，邮件头每行对应一个字段，和http头类似，比如这里的To，CC，From，Subject，分别代表收件人，抄送人，发件人，标题，如果有多个收件人或抄送人，用逗号分隔，邮件内容才是我们在邮件客户端真正看到的东西

邮件客户端都可以查看邮件源码，比如下面就是我收到的一封邮件的源码
![](http://littlewhite.us/pic/mail-qq.png)


## 邮件标题使用中文
如果邮件标题直接使用中文字符会导致收到的邮件乱码，为了避免这种情况，应该对中文进行base64编码，而这也是邮件最常用的编码方式，当然，在进行base64编码之前先得对中文字符进行编码（UTF-8或GBK等等），这和html的编码是一样的概念，采用UTF-8和base64编码的格式如下

    =?UTF-8?B?xxxxxx?=
    
其中`xxxxxx`为编码后的数据，用python可以快速对中文进行编码，比如对中文'测试'先进行utf-8编码再进行base64编码结果为

    >>> import base64
    >>> base64.standard_b64encode(u'测试'.encode('utf-8'))
    '5rWL6K+V'
    
在From和Subject中使用中文，邮件源码如下

    To: handy1989@qq.com
    CC: handy1989@qq.com
    From: =?UTF-8?B?5rWL6K+V?=<handy@test.com>
    Subject: =?UTF-8?B?5rWL6K+V?=
    
    hello world

这里将发件人的名字和邮件标题都改为了'测试'，收到的邮件效果为

![](http://littlewhite.us/pic/mail-chinese-title.png)

## 邮件内容使用html
如果邮件内容是html代码，则需要在邮件头添加Content-type字段来标记文本类型，同时还需要标记邮件内容的字符编码，以下邮件源码发送的正是html内容

    To: handy1989@qq.com
    CC: handy1989@qq.com
    From: =?UTF-8?B?5rWL6K+V?=<handy@test.com>
    Subject: =?UTF-8?B?5rWL6K+V?=
    Content-type: text/html;charset=utf-8
    
    <h1>hello world</h1>

收到的邮件效果为

![](http://littlewhite.us/pic/mail-html.png)

    