#curl参数说明
参数 | 说明 
--- | --- 
-i/--include | 输出响应包头
-I | 只获取响应包头
-x/—proxy <proxyhost[:port]> | 设置代理 
-X/—request <command> | 设置http method
-D/—dump-header <file> | 输出包头到指定文件
-H/--header <header> | 指定请求包头字段 </br>如果有多个字段，可多次使用本参数
-d/—data <data> | 发送post数据(ascii)</br> curl -d "param1=value1&param2=value2"
--data-binary <data> </br>—data-binary '@filename'|发送二进制post数据</br>如果以'@'开头则发送文件内容
-A/—user-agent <agent string> | 设置user-agent