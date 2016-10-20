Title: 一行Python代码——电话簿上的字符
Date: 2015-05-12
Modified: 2015-05-12
Category: Language
Tags: python
Slug: 一行Python代码——电话簿上的字符
Author: littlewhite

[TOC]


### Question
Given a digit string, return all possible letter combinations that the number could represent.

A mapping of digit to letters (just like on the telephone buttons) is given below.

![](http://littlewhite.us/pic/20150512/1.png)
    
    Input:Digit string "23"
    Output: ["ad", "ae", "af", "bd", "be", "bf", "cd", "ce", "cf"].
    
### Answer

    num2letter = {
                '1':'',
                '2':'abc',
                '3':'def',
                '4':'ghi',
                '5':'jkl',
                '6':'mno',
                '7':'pqrs',
                '8':'tuv',
                '9':'wxyz'}
    def letterCombinations(digits):
        return [] if digits == "" else reduce(lambda x, y:reduce(lambda a,b:a+b, [[i+j for i in x] for j in y], []), filter(lambda x:x!='', [num2letter[digit] for digit in digits]), [''])
        
### 答案分析
我们先求得每个数字对应的字符串，然后求这些字符串能组合的所有情况，比如根据数字得到的字符为'abc', 'def','ghi',那么组合数是3*3*3=27种，具体思路就是先求'abc'和'def'的组合['ab','ae','af','bd','be','bf','cd','ce','cf']，再算和'ghi'的组合，以此类推，这本身是每什么难度的，在Python里正好有个内建函数是干这个事的，那就是reduce

reduce的函数原型如下

    def reduce(function, iterable, initializer=None):
        it = iter(iterable)
        if initializer is None:
            try:
                initializer = next(it)
            except StopIteration:
                raise TypeError('reduce() of empty sequence with no initial value')
        accum_value = initializer
        for x in it:
            accum_value = function(accum_value, x)
        return accum_value

我们举个例子来说明

    reduce(lambda a,b:a+b, [1,2,3], 100)
    
这里返回的是100+1+2+3，lambda a,b:a+b定义了一个匿名函数，等价于

    def add(a, b):
        return a + b

这个reduce函数的具体计算步骤如下

    1. redult = 100
    2. result = add(result, 1) # 101
    3. result = add(result, 2) # 103
    4. result = add(result, 3) # 106* 
    5. return result

现在再来分析答案中的letterCombinations函数

为了看得清晰，我们先使用一些中间变量，改写函数如下

    def letterCombinations(digits):
        data = filter(lambda x:x!='', [num2letter[digit] for digit in digits])
        return [] if digits == "" else reduce(lambda x, y:reduce(lambda a,b:a+b, [[i+j for i in x] for j in y], []), data, [''])
        
* [num2letter[digit] for digit in digits]

    根据digits获取对应字符串

* filter(lambda x:x!='', [num2letter[digit] for digit in digits])

    过滤掉空字符串

* return [] if digits == "" else something

    python的三元组表达式，类似C的`condition ? value1 : value2`，Python的形式为`value1 if condition else value2`，这里意思是如果digits为空则返回[]，否则返回后面的something，这里的something就是我们要重点分析的如下表达式
        
          reduce(lambda x, y:reduce(lambda a,b:a+b, [[i+j for i in x] for j in y], []), data, [''])


下面我们来一层层抽丝剥茧。先看外层的reduce，我们将其表示成如下

    reduce(func(x,y), data, [''])

这是这道题的核心解题思路，我们还是以例子来说明，假设data=['abc','def','ghi']，那么计算步骤应该是

    1. result = ['']
    2. result = func(result, 'abc')
    3. result = func(result, 'def')
    4. result = func(result, 'ghi')
    5. return result

我们只需要实现func函数，问题就迎刃而解

其实func函数要做的就是将y里的每个字符添加到x里的每个字符串的末尾，比如

    x=['ab', 'cd']  
    y='ef'
    
那么func(x,y)应该返回`['abe','cde', 'abf', 'cdf']`，然后就是求func函数的实现

很容易我们想到列表解析，这里需要两层嵌套如下

    def func(x,y):
        return [[i + j for i in x] for j in y] 

但是这样的结果是[['abe', 'cde'], ['abf', 'cdf']]，此时我们还需要将列表的每个元素（注意，元素类型也为列表）连接起来，这不就是元素求和吗，只是这里的元素是列表而已，我们以文章开头reduce为参考

    reduce(lambda a,b:a+b, [1,2,3], 100)

只需稍作替换即可

    reduce(lambda a,b:a+b, [[i + j for i in x] for j in y], [])

这样func(x,y)函数的实现就成了下面这样

    def func(x,y):
        return reduce(lambda a,b:a+b, [[i + j for i in x] for j in y, [])

我们再将最外层reduce中的func函数改写成lambda表达式

    reduce(lambda x,y:reduce(lambda a,b:a+b, [[i + j for i in x] for j in y], []), data, [''])

大功告成！






