Title: gtest
Date: 2017-01-13
Modified: 2017-01-13
Category: Tool
Tags: gtest
Slug: gtest
Author: littlewhite

[TOC]

## 安装
项目地址： [https://github.com/google/googletest](https://github.com/google/googletest)

```sh
git clone https://github.com/google/googletest
cd googletest/googletest
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make
sudo make install
```

其中`-DCMAKE_INSTALL_PREFIX`指定的是安装目录，这里安装到了/usr/local目录，后续编译测试代码均使用此目录下的gtest库

## 使用
### 检查结果
检查结果可用`EXPECT_`和`ASSERT_`两组宏，前者如果验证失败会继续执行，后者会退出当前测试用例，但仍旧会执行后续的测试用例，两组宏的使用方式完全一致，下面列出`EXPECT_`宏的使用方式

    EXPECT_TRUE(condition);
    EXPECT_FALSE(condition);
    EXPECT_EQ(val1,val2);
    EXPECT_NE(val1,val2);
    EXPECT_LT(val1,val2);
    EXPECT_LE(val1,val2);
    EXPECT_GT(val1,val2);
    EXPECT_GE(val1,val2);
    EXPECT_STREQ(str1,str_2);
    EXPECT_STRNE(str1,str2);
    EXPECT_STRCASEEQ(str1,str2);
    EXPECT_STRCASENE(str1,str2);

顾名思义，就不多解释了
### 简单用例
使用TEST宏，每个宏定义一个测试用例，宏的两个参数分别代表测试类名和测试名，可随意定义

```c
// filename: test1.cpp
#include "gtest/gtest.h"

int Add(int x, int y)
{
    return x + y;
}

TEST(TestClass, TestName1)
{
    EXPECT_EQ(2, Add(1, 1)) << "Add error";
}

TEST(TestClass, TestName2)
{
    ASSERT_NE(3, Add(2, 3));
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

`EXPECT_EQ(2, Add(1, 1)) << "Add error";`表示在EXPECT_EQ宏比较失败时会打印后面的错误信息

**编译**

    g++ test1.cpp -I/usr/local/include -L/usr/local/lib -lgtest -lpthread -o test1
    
注意编译时需指定pthread库

**运行**

`./test1 -h` 查看参数说明  
`./test1 --gtest_list_tests` 查看用例  
`./test1` 运行所有用例  
`./test1 --gtest_filter=TestClass.Testname1` 运行指定用例  
`./test1 --gtest_filter='TestClass.*'` 使用通配符  
`./test1 --gtest_filter=-TestClass.Testname1` 排除指定用例  

### 共享成员变量
使用TEST\_F宏，宏参数含义和TEST一样，区别是第一个参数必须是已定义的类，每个类对应一组测试用例，可以选择定义如下几组函数

1. SetUp和TearDown， 在每个测试用例调用前和调用后执行
2. SetUpTestCase和TearDownTestCase，在每组测试用例调用前和调用后执行，必须为static void类型

亦可定义成员变量在该组例间共享，以上定义都必须是public或protected类型

```c
#include <stdio.h>
#include <vector>
#include "gtest/gtest.h"

using std::vector;

class TestFixture : public testing::Test
{
protected:
    static void SetUpTestCase()
    {
        printf("SetUpTestCase\n");
    }
    // static void TearDownTestCase() {}
    virtual void SetUp()
    {
        v1_.push_back(1);
        printf("SetUp\n");
    }
    //virtual void TearDown() {}

    vector<int> v1_;
    vector<int> v2_;
};

TEST_F(TestFixture, TestName1)
{
    EXPECT_EQ(1, v1_.size()) << "v1_.size() error";
}

TEST_F(TestFixture, TestName2)
{
    EXPECT_EQ(0, v2_.size());
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

### 全局事件
全局事件可以在所有测试用例运行前后执行

```c
#include <stdio.h>
#include "gtest/gtest.h"
using std::vector;

class FooEnvironment : public testing::Environment
{
public:
    virtual void SetUp()
    {
        printf("Foo FooEnvironment SetUp\n");
    }
    virtual void TearDown()
    {
        printf("Foo FooEnvironment TearDown\n");
    }
};

class TestFixture : public testing::Test
{
protected:
    static void SetUpTestCase()
    {
        printf("SetUpTestCase\n");
    }
    // static void TearDownTestCase() {}
    virtual void SetUp()
    {
        v1_.push_back(1);
        printf("SetUp\n");
    }
    //virtual void TearDown() {}

    vector<int> v1_;
    vector<int> v2_;
};

TEST_F(TestFixture, TestName1)
{
    EXPECT_EQ(1, v1_.size()) << "v1_.size() error";
}

TEST_F(TestFixture, TestName2)
{
    EXPECT_EQ(0, v2_.size());
}

TEST(TestClass, TestName1)
{
    EXPECT_EQ(1, 1);
}

int main(int argc, char** argv)
{
    testing::AddGlobalTestEnvironment(new FooEnvironment);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

定义一个类`FooEnvironment`继承自`testing::Environment`，并定义`SetUp`和`TeawDown`成员函数函数，在main函数里调用`testing::AddGlobalTestEnvironment(new FooEnvironment)`即可