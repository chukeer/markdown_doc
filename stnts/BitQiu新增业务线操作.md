## BitQiu新增业务线运维操作步骤

### BitQiu客户端脚本部署环境

    10.0.0.5
    /root/btq_client

### 操作步骤

1. 在跟目录创建业务线目录，假设业务线名字为test123

        cd /root/btq_client/shell && ./mkdir_under / -a 'test123'
        
    **并记录下path_id，假设为/<1560701>**
        
2. 给新建目录设置副本数
    
        cd /root/btq_client/shell && ./set_attr '/test123' -r 4
        
3. 新增客户端node_id

    创建如下文件内容
    
        cluster_id = 100
        room_id = 101
        rack_id = 102
        host_id = 103
        service_id = 104
        node_type = 3
        node_attr = 0
        user_key = 39935760fd56ce49cda26b28e8cdba56
        AC_key_0=/<1560701>
        AC_value_0=4294967295
        
    其中`AC_key_0`为新建目录的path\_id，`AC_value_0`为4294967295，16进制0xffffffff，代表对目录由所有操作权限，node\_type为3（代表客户端），node\_attr为0，user_key可以随便配，其它字段cluster\_id,room\_id,rack\_id,host\_id,service\_id不要和其它节点重复
    
    将文件按`client_业务线_cluster_id,room_id,rack_id,host_id,service_id`的格式命令，如以上内容的文件命名为`client_test123_100,101,102,103,104`
    
    将文件上传到zookeeper的节点/BitQiu/online/rootServer/user_keys下
    
        cd /root/btq_client && ./zk_online.py put_file /BitQiu/online/rootServer/user_keys/client_test123_100,101,102,103,104 client_test123_100,101,102,103,104
        
4. 通知rootServer重新加载配置

        cd /root/btq_client && ./update_all_online_conf.sh
        
    确保所有rootServer均执行成功，对执行失败的，可以单独通知
    
        cd /root/btq_client && ./repeater_client.py -H host -p 18081 update_conf
        
5. 测试是否配置成功
    
    修改/root/btq\_client/py\_script/config.py文件的OnlineConfig类成员`self_node_id_opt`和`user_key`为之前配置的值，其中`self_node_id_opt`的格式为`cluster_id,room_id,rack_id,host_id,service_id`
    
    执行测试脚本
    
        cd /root/btq_client/test && ./cud_test.py '/test123' -c online
        
    没有打印红色错误信息即为成功
        
<hr>
        
请龙发将以上信息汇总到BitQiu运维手册里
        
        