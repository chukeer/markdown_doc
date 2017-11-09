# STPush REST API
## 概述
STPush对外提供遵从REST规范的HTTP API，首先约定如下几点约束

1. 如果不涉及到消息的修改则采用GET方法，如查询消息状态，统计消息到达数等，否则采用POST方法，如发布消息，编辑消息，删除消息。如果请求不是对应的HTTP方法，则返回失败
2. 所有请求参数均采用utf-8编码，GET请求参数还需要进行urlencode
3. API返回数据为JSON格式

对于第2点，比如查询参数为`tag=测试`，则编码步骤为

1. 对`tag`进行utf-8编码，然后进行urlencode，得到`tag`
2. 对`测试`进行utf-8编码，然后进行urlencode，得到`%E6%B5%8B%E8%AF%95`
3. 将编码后的参数用`=`连接得到`tag＝%E6%B5%8B%E8%AF%95`，以python为例，操作如下

        >>> urllib.urlencode({'tag':u'测试'.encode('utf-8')})
        'tag=%E6%B5%8B%E8%AF%95'
        

## API资源列表

名称    | 资源 |  描述
------- | ------------- | --- 
PUSH  | POST /v1/push  | 发布消息，由服务端调用  
UPDATE | POST /v1/update | 更新消息，由服务端调用
REMOVE | POST /v1/remove | 删除消息，由服务端调用
STAT   | GET /v1/stat | 统计消息状态，由服务端调用
FETCH  | GET /v1/fetch | 获取消息，由客户端调用

以上服务端可以是web控制台，也可以是业务线自己的服务程序，客户端指终端设备

## 公共请求参数
服务端和客户端都有各自的公共请求参数，并且服务端需要做权限认证，客户端暂时不需要

### 服务端公共参数

参数名 | 类型 | 必需 | 描述 
--- | ---- | ---- | ----
apikey | string | 是 | 业务线ID，创建业务线时获取
sign   | string | 是 | 签名，与api_key成对出现（[签名算法](#sign)）

### 客户端公共参数

参数名 | 类型 | 必需 | 描述 
--- | ---- | ---- | ----
apikey | string | 是 | 业务线ID
user_id   | string | 否 | 用户ID，从用户中心获取
device_id | string | 是 | 设备ID，可以是mac地址或其它

## 返回格式
服务端返回数据均为JSON格式，错误信息按如下格式响应

    {
        'request_id' : 12345678,
        'err_code' : 1001,
        'err_msg' : 'Request params not valid'
    }

每个消息正确响应格式均不相同，详见后面描述

<span id="sign"></span>
## 签名算法
采用如下签名算法：

* 获取请求的http method，采用大写格式
* 获取请求的url，包括host和sheme，但不包括query\_string的部分
* 将所有参数（包括GET或POST的参数，但不包含签名字段）格式化为“key=value”格式，如“k1=v1”、“k2=v2”、“k3=v3”
* 将格式化好的参数键值对以字典序升序排列后，拼接在一起，如“k1：v1，k2 ：v2，k3：v3”，并将http method和url按顺序拼接在这个字符串前面
* 在拼接好的字符串末尾追加上业务线的secret\_key，并进行urlencode，形成base\_string
* 对base_string计算MD5得到签名，即

    `sign = MD5( urlencode( $http_method$url$k1=$v1$k2=$v2$k3=$v3$secret_key ))`
    
secret\_key为业务线创建时生成，请前往web控制台查询

比如发送POST给https://api.push.stnts.com/v1/test，参数为

    apikey: 12345667
    secret_key: 87654321
    user_id: 00000000
    
则参与签名的字符串为

    POSThttps://api.push.stnts.com/v1/testapikey=12345678user_id=0000000087654321
    
对以上字符串进行urlencode后再计算MD5即为本次请求对签名
    
## 发布消息
### 功能说明
向单个设备或批量设备推送消息
### 调用地址
POST https://api.push.stnts.com/v1/push
### 调用参数
除公共字段外，还需指定以下参数（后续API调用参数均未列出公共参数）

参数名 | 类型 | 必需 | 描述 
--- | ---- | ---- | ----
msg_type | int | 是 | 消息类型
msg_content   | string | 是 | 消息内容
platform | JSON Array | 是 | 推送平台
audience | string or JSON object | 是  | 推送目标
valid_time | int | 是 | 消息生效时间戳，采用unix时间戳
expires_time | int | 是 | 消息过期时间戳，采用unix时间戳
receiver_num | int | 否 | 推送目标数，若未指定则不限目标数，每个客户端视为一个目标
extras | JSON Object | 否 | 指定附加信息

### msg_type
消息类型字段，定义如下

msg_type | 描述 
:----: | ----
1 | 透传消息，客户端自己解析

### audience数据格式 
如果是广播消息，则直填写"all"，否则需指定为如下格式

参数名 | 类型  | 描述 
--- | ----  | ----
user_id | JSON Array | 指定用户ID
tag | JSON Array  | 指定分组名，取并集
tag_and | JSON Array | 指定分组名，取交集

以上字段均为JSON数组类型，user\_id和tag数组里元素为OR关系，取并集，比如tag为['tag1', 'tag1']，则推送给具有tag1或tag2的用户，tag\_and数组里的元素为AND关系，取交集，比如tag_and为['tag1', 'tag1']，则推送给同时具有tag1和tag2的用户。至少有一个字段必需存在，否则消息视为无效

### extras
extra为附加信息，其类型为JSON Object，里面的参数和类型均为string，可指定为任意内容，客户端在获取这样的消息时必需将属性带在请求参数中，服务通过匹配决定是否返回消息

### 示例
* 广播消息

        {
            'msg_type' : 1,
            'msg_content' : 'hello world',
            'platform' : ['windows'],
            'audience' : 'all',
            'valid_time' : 1482308129,
            'expires_time' : 1482318129
        }
        
* 给100个用户广播消息

        {
            'msg_type' : 1,
            'msg_content' : 'hello world',
            'platform' : ['windows'],
            'audience' : 'all',
            'valid_time' : 1482308129,
            'expires_time' : 1482318129,
            'receiver_num' : 100
        }
    
* 推送给指定用户

        {
            'msg_type' : 1,
            'msg_content' : 'hello world',
            'platform' : ['windows'],
            'audience' : {
                'user_id' : ['user1', 'user2']
            },
            'valid_time' : 1482308129,
            'expires_time' : 1482318129
        }
    
* 推送给武汉和北京的用户

        {
            'msg_type' : 1,
            'msg_content' : 'hello world',
            'platform' : ['windows'],
            'audience' : {
                'tag' : ['武汉', '北京']
            },
            'valid_time' : 1482308129,
            'expires_time' : 1482318129
        }
    
* 推送给分组为武汉且为vip5的用户

        {
            'msg_type' : 1,
            'msg_content' : 'hello world',
            'platform' : ['windows'],
            'audience' : {
                'tag_and' : ['武汉', 'vip5']
            },
            'valid_time' : 1482308129,
            'expires_time' : 1482318129
        }

* 推送给具有指定属性的用户

        {
            'msg_type' : 1,
            'msg_content' : 'hello world',
            'platform' : ['windows'],
            'audience' : 'all',
            'valid_time' : 1482308129,
            'expires_time' : 1482318129,
            'extra' : {
                'vip_ge' : '5'
            }
        }
        
    指定属性必需由客户端在请求参数里自带，客户端请求中除了我们指定的参数，其它参数都默认作为额外属性，比如如下请求
    
        GET https://api.push.stnts.com/v1/fetch?apikey=xxx&&user_id=xxx&&vip_ge=5
    
    则vip\_ge=5作为额外属性，服务端在extra字段查找vip\_ge字段，若匹配到且其值为5则该消息可被获取
    
### 返回内容

    {
        'request_id' : 12345678,
        'msg_id' : 87654321
    }

## 更新消息
### 功能说明
对未过期的消息，修改其参数
### 调用地址
POST https://api.push.stnts.com/v1/update
### 调用参数
更新消息和发布消息相比，多了msg_id参数

参数名 | 类型 | 必需 | 描述 
--- | ---- | ---- | ----
msg_id | string |是 | 消息ID，由发布消息API返回
msg_type | int | 是 | 消息类型
msg_content   | string | 是 | 消息内容
platform | JSON Array | 是 | 推送平台
audience | string or JSON object | 是  | 推送目标
valid_time | int | 是 | 消息生效时间戳，采用unix时间戳
expires_time | int | 是 | 消息过期时间戳，采用unix时间戳
receiver_num | int | 否 | 推送目标数，若未指定则不限目标数，每个客户端视为一个目标
extras | JSON Object | 否 | 指定附加信息


### 返回内容

    {
        'request_id' : 12345678,
        'msg_id' : 87654321
    }

## 删除消息
### 功能说明
删除未过期的消息
### 调用地址
POST https://api.push.stnts.com/v1/remove
### 调用参数
参数名 | 类型 | 必需 | 描述 
--- | ----  | ---- | ----
msg_id | string |是 | 消息ID，由发布消息API返回

### 返回内容

    {
        'request_id' : 12345678,
        'msg_id' : 87654321
    }
    
## 统计消息
### 功能说明
统计消息状态
### 调用地址
GET https://api.push.stnts.com/v1/stat
### 调用参数
参数名 | 类型 | 必需 | 描述 
--- | ----  | ---- | ----
msg_id | string |是 | 消息ID

### 返回内容
除了返回推送消息时指定的一些参数外，还会返回msg\_id和current\_receiver\_num两个字段

参数名 | 类型 | 必需 | 描述 
--- | ----  | ---- | ----
msg_id | string |是 | 消息ID
platform | JSON Array | 是 | 推送平台
audience | string or JSON object | 是  | 推送目标
valid_time | int | 是 | 消息生效时间戳，采用unix时间戳
expires_time | int | 是 | 消息过期时间戳，采用unix时间戳
receiver_num | int | 否 | 推送目标数
current\_receiver\_num | int | 是 | 当前消息到达数
extras | JSON Object | 否 | 指定附加信息

## 获取消息
### 功能说明
客户端获取消息
### 调用地址
GET https://api.push.stnts.com/v1/fetch
### 调用参数
除了客户端公共参数外，还需指定如下字段

参数名 | 类型 | 必需 | 描述 
--- | ----  | ---- | ----
extra | string |否 | 指定附加信息

### 返回参数
参数名 | 类型 | 必需 | 描述 
--- | ----  | ---- | ----
msg_id | string | 是 | 消息ID
msg_type | int | 是 | 消息类型
msg_content | string | 是 | 消息内容
