>python解析网页，无出BeautifulSoup左右，此是序言

## 安装
BeautifulSoup4以后的安装需要用eazy_install，如果不需要最新的功能，安装版本3就够了，千万别以为老版本就怎么怎么不好，想当初也是千万人在用的啊。安装很简单

```
wget "http://www.crummy.com/software/BeautifulSoup/download/3.x/BeautifulSoup-3.2.1.tar.gz"
tar zxvf BeautifulSoup-3.2.1.tar.gz
```

然后把里面的BeautifulSoup.py这个文件放到你python安装目录下的site-packages目录下
site-packages是存放Python第三方包的地方，至于这个目录在什么地方呢，每个系统不一样，可以用下面的方式找一下，基本上都能找到

	sudo find / -name "site-packages" -maxdepth 5 -type d

当然如果没有root权限就查找当前用户的根目录

	find ~ -name "site-packages" -maxdepth 5 -type d

如果你用的是Mac，哈哈，你有福了，我可以直接告诉你，Mac的这个目录在/Library/Python/下，这个下面可能会有多个版本的目录，没关系，放在最新的一个版本下的site-packages就行了。使用之前先import一下

```python
from BeautifulSoup import BeautifulSoup
```

## 使用

在使用之前我们先来看一个实例
现在给你这样一个页面

[http://movie.douban.com/tag/%E5%96%9C%E5%89%A7](http://movie.douban.com/tag/%E5%96%9C%E5%89%A7)

它是豆瓣电影分类下的喜剧电影，如果让你找出里面评分最高的100部，该怎么做呢
好了，我先晒一下我做的，鉴于本人在CSS方面处于小白阶段以及天生没有美术细菌，界面做的也就将就能看下，别吐

[http://littlewhite.us/douban/xiju/](http://littlewhite.us/douban/xiju/)

接下来我们开始学习BeautifulSoup的一些基本方法，做出上面那个页面就易如反掌了 鉴于豆瓣那个页面比较复杂，我们先以一个简单样例来举例，假设我们处理如下的网页代码

```html
<html>
<head><title>Page title</title></head>
<body>
    <p id="firstpara" align="center">
    This is paragraph
        <b>
        one
        </b>
        .
    </p>
    <p id="secondpara" align="blah">
    This is paragraph
        <b>
        two
        </b>
        .
    </p>
</body>
</html>
```

你没看错，这就是官方文档里的一个样例，如果你有耐心，看官方文档就足够了，后面的你都不用看

[http://www.leeon.me/upload/other/beautifulsoup-documentation-zh.html](http://www.leeon.me/upload/other/beautifulsoup-documentation-zh.html)

### 初始化

首先将上面的HTML代码赋给一个变量html如下，为了方便大家复制这里贴的是不带回车的，上面带回车的代码可以让大家看清楚HTML结构

```python
html = '<html><head><title>Page title</title></head><body><p id="firstpara" align="center">This is paragraph<b>one</b>.</p><p id="secondpara" align="blah">This is paragraph<b>two</b>.</p></body></html>'

```

初始化如下：

```python
soup = BeautifulSoup(html)
```

我们知道HTML代码可以看成一棵树，这个操作等于是把HTML代码解析成一种树型的数据结构并存储在soup中，注意这个数据结构的根节点不是<html>，而是soup，其中html标签是soup的唯一子节点，不信你试试下面的操作

```python
print soup
print soup.contents[0]
print soup.contents[1]
```

前两个输出结果是一致的，就是整个html文档，第三条输出报错IndexError: list index out of range

### 查找节点

查找节点有两种反回形式，一种是返回单个节点，一种是返回节点list，对应的查找函数分别为find和findAll

#### 单个节点

* 根据节点名

	```python
	## 查找head节点
	print soup.find('head') ## 输出为<head><title>Page title</title></head>
	## or
	## head = soup.head
	```

	这种方式查找到的是待查找节点最近的节点，比如这里待查找节点是soup，这里找到的是离soup最近的一个head（如果有多个的话）

* 根据属性

	```python
	 ## 查找id属性为firstpara的节点
	 print soup.find(attrs={'id':'firstpara'})  
	 ## 输出为<p id="firstpara" align="center">This is paragraph<b>one</b>.</p>
	 ## 也可节点名和属性进行组合
	 print soup.find('p', attrs={'id':'firstpara'})  ## 输出同上
	```
	
* 根据节点关系

	节点关系无非就是兄弟节点，父子节点这样的

	```python
	 p1 = soup.find(attrs={'id':'firstpara'}) ## 得到第一个p节点
	 print p1.nextSibling ## 下一个兄弟节点
	 ## 输出<p id="secondpara" align="blah">This is paragraph<b>two</b>.</p>
	 p2 = soup.find(attrs={'id':'secondpara'}) ## 得到第二个p节点
	 print p2.previousSibling ## 上一个兄弟节点
	 ## 输出<p id="firstpara" align="center">This is paragraph<b>one</b>.</p>
	 print p2.parent ## 父节点，输出太长这里省略部分 <body>...</body>
	 print p2.contents[0] ## 第一个子节点，输出u'This is paragraph'
	```

	contents上面已经提到过，它存储的是所有子节点的序列

#### 多个节点
将上面介绍的find改为findAll即可返回查找到的节点列表，所需参数都是一致的

* 根据节点名

	```python
	## 查找所有p节点
	soup.findAll('p')
	```

* 根据属性查找

	```python
	## 查找id=firstpara的所有节点
	soup.findAll(attrs={'id':'firstpara'}) 
	```

	需要注意的是，虽然在这个例子中只找到一个节点，但返回的仍是一个列表对象

上面的这些基本查找功能已经可以应付大多数情况，如果需要各个高级的查找，比如正则式，可以去看官方文档

### 获取文本
getText方法可以获取节点下的所有文本，其中可以传递一个字符参数，用来分割每个各节点之间的文本

```python
## 获取head节点下的文本
soup.head.getText()         ## u'Page title'
## or
soup.head.text
## 获取body下的所有文本并以\n分割
soup.body.getText('\n')     ## u'This is paragraph\none\n.\nThis is paragraph\ntwo\n.'
```

## 实战
有了这些功能，文章开头给出的那个Demo就好做了，我们再来回顾下豆瓣的这个页面

[http://movie.douban.com/tag/%E5%96%9C%E5%89%A7](http://movie.douban.com/tag/%E5%96%9C%E5%89%A7)

如果要得到评分前100的所有电影，对这个页面需要提取两个信息：1、翻页链接；2、每部电影的信息（外链，图片，评分、简介、标题等）

当我们提取到所有电影的信息后再按评分进行排序，选出最高的即可，这里贴出翻页提取和电影信息提取的代码

```python
## filename: Grab.py
from BeautifulSoup import BeautifulSoup, Tag
import urllib2
import re
from Log import LOG
 
def LOG(*argv):
    sys.stderr.write(*argv)
    sys.stderr.write('\n')
 
class Grab():
    url = ''
    soup = None
    def GetPage(self, url):
        if url.find('http://',0,7) != 0:
            url = 'http://' + url
        self.url = url
        LOG('input url is: %s' % self.url)
        req = urllib2.Request(url, headers={'User-Agent' : "Magic Browser"})
        try:
            page = urllib2.urlopen(req)
        except:
            return
        return page.read()  
 
    def ExtractInfo(self,buf):
        if not self.soup:
            try:
                self.soup = BeautifulSoup(buf)
            except:
                LOG('soup failed in ExtractInfo :%s' % self.url)
            return
        try:
            items = self.soup.findAll(attrs={'class':'item'})
        except:
            LOG('failed on find items:%s' % self.url)
            return
        links = []
        objs = [] 
        titles = []
        scores = []
        comments = []
        intros = []
        for item in items:
            try:
                pic = item.find(attrs={'class':'nbg'})
                link = pic['href']
                obj = pic.img['src']
                info = item.find(attrs={'class':'pl2'})
                title = re.sub('[ \t]+',' ',info.a.getText().replace('&amp;nbsp','').replace('\n',''))
                star = info.find(attrs={'class':'star clearfix'})
                score = star.find(attrs={'class':'rating_nums'}).getText().replace('&amp;nbsp','')
                comment = star.find(attrs={'class':'pl'}).getText().replace('&amp;nbsp','')
                intro = info.find(attrs={'class':'pl'}).getText().replace('&amp;nbsp','')
            except Exception,e:
                LOG('process error in ExtractInfo: %s' % self.url)
                continue
            links.append(link)
            objs.append(obj)
            titles.append(title)    
            scores.append(score)
            comments.append(comment)
            intros.append(intro)
        return(links, objs, titles, scores, comments, intros)
 
    def ExtractPageTurning(self,buf):
        links = set([])
        if not self.soup:
            try:
                self.soup = BeautifulSoup(buf)
            except:
                LOG('soup failed in ExtractPageTurning:%s' % self.url)
                return
        try:
            pageturning = self.soup.find(attrs={'class':'paginator'})
            a_nodes = pageturning.findAll('a')
            for a_node in a_nodes:
                href = a_node['href']
                if href.find('http://',0,7) == -1:
                    href = self.url.split('?')[0] + href
                links.add(href)
        except:
            LOG('get pageturning failed in ExtractPageTurning:%s' % self.url)
 
        return links
 
    def Destroy(self):
        del self.soup
        self.soup = None
```

接着我们再来写个测试样例

```python
## filename: test.py
#encoding: utf-8
from Grab import Grab
import sys
reload(sys)
sys.setdefaultencoding('utf-8')
 
grab = Grab()
buf = grab.GetPage('http://movie.douban.com/tag/喜剧?start=160&amp;type=T')
if not buf:
        print 'GetPage failed!'
        sys.exit()
links, objs, titles, scores, comments, intros = grab.ExtractInfo(buf)
for link, obj, title, score, comment, intro in zip(links, objs, titles, scores, comments, intros):
        print link+'\t'+obj+'\t'+title+'\t'+score+'\t'+comment+'\t'+intro
pageturning = grab.ExtractPageTurning(buf)
for link in pageturning:
        print link
grab.Destroy()
```

OK，完成这一步接下来的事儿就自个看着办吧  

本文只是介绍了BeautifulSoup的皮毛而已，目的是为了让大家快速学会一些基本要领，想当初我要用什么功能都是去BeautifulSoup的源代码里一个函数一个函数看然后才会的，一把辛酸泪啊，所以希望后来者能够通过更便捷的方式去掌握一些基本功能，也不枉我一字一句敲出这篇文章，尤其是这些代码的排版，真是伤透了脑筋

The end.
