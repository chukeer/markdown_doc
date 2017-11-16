Title: C++数据库操作之SOCI
Date: 2017-01-22
Modified: 2017-01-22
Category: Language
Tags: database, orm
Slug: C++数据库操作之SOCI
Author: littlewhite

[TOC]

[SOCI](http://soci.sourceforge.net/)是一个数据库操作的库，并不是ORM库，它仍旧需要用户编写sql语句来操作数据库，只是使用起来会更加方便，主要有以下几个特点

1. 以stream方式输入sql语句
2. 通过into和use语法传递和解析参数
3. 支持连接池，线程安全

由此可见它只是一个轻量级的封装，因此也有更大的灵活性，后端支持oracle，mysql等，后续示例均基于mysql

## 安装
git项目地址[https://github.com/SOCI/soci](https://github.com/SOCI/soci)

推荐使用cmake编译

```sh
git clone https://github.com/SOCI/soci.git
cd soci
mkdir build 
cd build
cmake .. -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=/opt/third_party/soci
make
sudo make install
```

## 基本查询
假设有如下表单

```sql
CREATE TABLE `Person` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `first_name` varchar(64) NOT NULL DEFAULT '',
  `second_name` varchar(64) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
```

### 初始化session

```c
using namespace soci;
session sql("mysql", "dbname=test user=your_name password=123456");
```

第一个参数为使用的后端数据库类型，第二个参数为数据库连接参数，可以指定的参数包括`host port dbname user passowrd`等，以空格分隔

### insert

```c
string first_name = "Steve";
string last_name = "Jobs";
sql << "insert into Person(first_name, last_name)"
    " values(:first_name, :last_name)", 
    use(first_name), use(last_name);
long id;
sql.get_last_insert_id("Person", id)
```

通过流的方式传递sql语句，用use语法传递参数

其中`Person(first_name, last_name)`为数据库table名和column名，`values(:first_name, :last_name)`里的为参数的占位符，这里可以随便书写，`get_last_insert_id`函数可以获取自增长字段的返回值

需要注意的是`use`函数里的参数的生命周期，切记不能将函数返回值作为`use`函数的参数

### select

```c
int id = 1;
string first_name;
string last_name;
sql << "select first_name, last_name from Person where id=:id ", 
    use(id), into(first_name), into(last_name);
if (!sql.got_data())
{
    cout << "no record" << endl;
}
```

这里根据id字段查询first\_name和last\_name两个字段，并通过`into`函数将数据复制给变量，`got_data()`方法可判断是否有数据返回

当id为整数时，sql语句也可以写作`sql << "select balabala from Person where id=" << id`，但当id为字符串时这样写会报错，因此建议都采用`use`函数

如果查询结果是多行数据，则需要使用rowset类型并自己提取

```c
rowset<row> rs = (sql.prepare << "select * from Person");
for (rowset<row>::iterator it = rs.begin(); it != rs.end(); ++it)
{
    const row& row = *it;
    cout << "id:" << row.get<long long>(0)
        << " first_name:" << row.get<string>(1)
        << " last_name:" << row.get<string>(2) << endl;
  
}
```

这里get模版的参数类型必需和数据库类型一一对应，varchar和text类型对应string，整数类型按如下关系对应

数据库类型 | soci类型
---- | ----
SMALLINT | int
MEDIUMINT | int
INT | long long
BIGINT | unsigned long long

### update

```c
int id = 1;
string first_name = "hello";
string last_name = "world";
sql << "update Person set first_name=:first_name, last_name=:last_name"
    " where id=:id", 
    use(first_name), use(last_name), use(id);
```

### delete

```c
int id = 1;
sql << "delete from Person where id=:id", use(id);
```

有时候我们需要关注delete操作是否真的删除了数据，mysql本身也会返回操作影响的行数，可以采用如下方法获取

```c
statement st = (sql.prepare << "delete from Person where id=:id", use(id));
st.execute(true);
int affected_rows = st.get_affected_rows();
```

## 使用连接池
使用连接池可以解决多线程的问题，每个线程在操作数据库时先从连接池取出一个session，这个session会被设置为锁定，用完之后再换回去，设置为解锁，这样不同线程使用不同session，互不影响。session对象可以用连接池来构造，构造时自动锁定，析构时自动解锁

```c
int g_pool_size = 3;
connection_pool g_pool(g_pool_size);
for (int i = 0; i < g_pool_size; ++i)
{
    session& sql = g_pool.at(i);
    sql.open("mysql", "dbname=test user=zhangmenghan password=123456");
}
session sql(g_pool);
sql << "select * from Person";
```

此时`session sql(g_pool)`的调用是没有超时时间的，如果没有可用的session，会一直阻塞，如果要设置超时时间，可以采用connection_pool的底层接口

```c
session & at(std::size_t pos);
bool try_lease(std::size_t & pos, int timeout);
void give_back(std::size_t pos);
```

调用方式如下

```c
size_t pos
if (!try_lease(pos, 3000)) // 锁定session，设置超时为3秒
{
    return;
}
session& sql = g_pool.at(pos) // 获取session，此时pos对应的session已被锁定
/* sql操作 ... */
g_pool.give_back(pos); // 解锁pos对应的session
```

需要注意的是，如果`try_lease`调用成功后没有调用`give_back`，会一直锁定对应的session，因此`try_lease`和`give_back`必需成对使用

## 事务
session对象提供了对事务的操作方法

```c
void begin();
void commit();
void rollback();
```

同时也提供了封装好的transaction对象，使用方式如下

```c
{
    transaction tr(sql);

    sql << "insert into ...";
    sql << "more sql queries ...";
    // ...

    tr.commit();
}
```

如果commit没有被执行，则transaction对象在析构时会自动调用session对象的rollback方法

## ORM
soci可以通过自定义对象转换方式从而在use和into语法中直接使用用户对象

比如针对Person表单我们定义如下结构和转换函数

```c
struct Person
{
    uint32_t id;
    string first_name;
    string last_name;
}

namespace soci {
template<>
struct type_conversion<Person>
{
    typedef values base_type;
    static void from_base(const values& v, indicator ind, Person& person)
    {
        person.id = v.get<long long>("id");
        person.first_name = v.get<string>("first_name");
        person.last_name = v.get<string>("last_name");

    }
    static void to_base(const Person& person, values& v, indicator& ind)
    {
        v.set("id", (long long)person.id);
        v.set("first_name", person.first_name);
        v.set("last_name", person.last_name);
    }
};
}
```

需要注意的是这里get模板的参数类型必需和数据库字段对应，对应关系见之前select的示例，对于整数类型，在set时最好也加上强转并且和get一致，否则可能会抛异常`std::bad_cast`。get和set函数的第一个参数是占位符，占位符的名字不一定要和数据库column名一致，但后续操作中`values`语法里的占位符必需和这里指定的一致

定义了`type_conversion`之后，后续在用到use和into语法时可直接使用Person对象，这时soci会根据占位符操作指定字段

### insert

```c
Person person;
person.first_name = "Steve";
person.last_name = "Jobs";
sql << "insert into Person(first_name, last_name)"
    " values(:first_name, :last_name)", use(person);
```

### select

```c
int id = 1;
Person person;
sql << "select * from Person where id=:id", use(id), into(person);

rowset<Person> rs = (sql.prepare << "select * from Person");
for (rowset<Person>::iterator it = rs.begin(); it != rs.end(); ++it)
{
    const Person& person = *it;
    // do something with person
}
```

### update

```c
person.id = 1;
person.first_name = "hello";
person.last_name = "world";
sql << "update Person set first_name=:first_name, last_name=:last_name"
    " where id=:id", use(person);
```

### delete

```c
Person person;
person.id = 1;
sql << "delete from Person where id=:id", use(person);
```

## 完整示例
[https://github.com/handy1989/soci_test](https://github.com/handy1989/soci_test)

