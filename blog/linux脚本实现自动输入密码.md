#linux脚本实现自动输入密码
使用Linux的程序员对输入密码这个举动一定不陌生，在Linux下对用户有严格的权限限制，干很多事情越过了权限就得输入密码，比如使用超级用户执行命令，又比如ftp、ssh连接远程主机等等，如下图

<div align="center"><img src="http://littlewhite.us/pic/20141110/ssh_login.png"></div>

那么问题来了，在脚本自动化执行的时候需要输入密码怎么办？比如你的脚本里有一条scp语句，总不能在脚本执行到这一句时手动输入密码吧

针对于ssh或scp命令，可能有人会回答是建立信任关系，关于建立ssh信任关系的方法请自行百度Google，只需要两行简单的命令即可搞定，但这并不是常规的解决方案，如果是ftp连接就没辙了，况且，你不可能为了执行某些命令去给每个你要连接的主机都手动建立ssh信任，这已经偏离了今天主题的本意，今天要说的是在脚本里自动输入密码，我们可以想象下，更优雅的方式应该是在脚本里自己配置密码，当屏幕交互需要输入时自动输入进去，要达到这样的效果就需要用到expect

###安装

CentOS下安装命令很简单，如下

	sudo yum install expect
至于Mac用户，可以通过homebrew安装(需要先安装homebrew，请自行Google)

	brew install expect
	
###测试脚本
我们写一个简单的脚本实现scp拷贝文件，在脚本里配置密码，保存为scp.exp如下

	#!/usr/bin/expect
	set timeout 20

	if { [llength $argv] < 2} {
    	puts "Usage:"
    	puts "$argv0 local_file remote_path"
    	exit 1
	}

	set local_file [lindex $argv 0]
	set remote_path [lindex $argv 1]
	set passwd your_passwd

	set passwderror 0

	spawn scp $local_file $remote_path

	expect {
		"*assword:*" {
			if { $passwderror == 1 } {
            puts "passwd is error"
            exit 2
        	}
        	set timeout 1000
        	set passwderror 1
        	send "$passwd\r"
        	exp_continue
    	}
    	"*es/no)?*" {
        	send "yes\r"
        	exp_continue
    	}
    	timeout {
        	puts "connect is timeout"
        	exit 3
    	}
	}
注意，第一行很重要，通常我们的脚本里第一行是`#!/bin/bash`，而这里是你机器上expect程序的路径，说明这段脚本是由expect来解释执行的，而不是由bash解释执行，所以代码的语法和shell脚本也是不一样的，其中`set passwd your_passwd`设置成你自己的密码，然后执行如下命令

	./scp.exp ./local_file user@host:/xx/yy/
	
执行前确保scp.exp有执行权限，第一个参数为你本地文件，第二个为远程主机的目录，运行脚本如果报错“connect is timeout”，可以把超时设长一点，第二行`set timeout 20`可以设置超时时间，单位是秒。脚本执行效果如下

![http://littlewhite.us/pic/20141110/scp_exp.png](http://littlewhite.us/pic/20141110/scp_exp.png)

###还能做什么
细心的同学一定发现了，其实expect提供的是和终端的一种交互机制，输入密码只是其中一种应用形式，只要是在终端阻塞需要输入时，都可以通过expect脚本完成自动输入，比如前面脚本里配置了两种交互场景，一种是终端提示"password:"时输入密码，还有一种是提示"yes/no)?"时输入“yes”，如果和远程主机是第一次建立连接，执行scp.exp脚本效果是这样的

![http://littlewhite.us/pic/20141110/scp_exp_2.png](http://littlewhite.us/pic/20141110/scp_exp_2.png)

所以我们可以根据终端的提示来配置输入命令，这样就能达到了自动化的效果。至于处理其它交互场景，只需要照着上面的脚本依葫芦画瓢就行了




	