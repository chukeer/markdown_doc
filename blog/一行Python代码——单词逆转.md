Title: 一行Python代码——单词逆转
Date: 2015-05-11
Modified: 2015-05-11
Category: Language
Tags: python
Slug: 一行Python代码——单词逆转
Author: littlewhite

[TOC]

### Question
Given an input string, reverse the string word by word.

For example,  
Given s = "**the sky is blue**",  
return "**blue is sky the**".

### Answer

    def reverseWords(s):
        return ' '.join(filter(lambda x:x != '', s.split(' '))[::-1])
        
详解：

* s.split(' ')

    将字符串s按空格为分隔符进行分割，生成列表，注意，如果有连续空格，列表里会有空元素
    
* lambda x:x!=''

    定义一个匿名函数，等价于如下函数
    
          def func(x):
              return x != ''

* filter(lambda x:x!='', s.split(' '))

    由第1条可知，s.split(' ')是一个列表，lambda x:x!=''是一个函数，filter函数将列表的每个元素依次作为参数传递给lambda函数，如果函数返回true，则该元素被保留，否则被过滤。这里可以过掉列表里为空的元素

* filter(lambda x:x != '', s.split(' '))[::-1]

    列表元素顺序逆转

* ' '.filter(lambda x:x != '', s.split(' '))[::-1]

    用' '连接列表的每个元素




