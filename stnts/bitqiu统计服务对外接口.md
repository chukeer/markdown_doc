# BitQiu统计服务对外接口
对外接口均使用http GET方式提供，返回均为json格式，json数据有如下两个固定字段

    errcode: 返回错误吗，成功为0，其它为失败
    errmsg: 错误信息，成功为"ok"，错误为其它信息
    
如果返回错误，不会有其它字段

###获取节点列表
请求URL

    http://host:port/json_api.1.0/get_node_list?type=$type&status=$status

返回格式

    {
        "errcode" : errcode,
        "errmsg" : errmsg,
        "nodelist" : [node1, node2, ...]
    }
    
说明

    请求参数
        type: 节点类型，可设置为root, store, client
        status: 节点状态，可设置为online, offline, error
    返回参数
        nodelist: 根据指定参数返回的节点列表，如果未指定参数，则返回所有节点
    
    
###获取系统基本信息
请求URL

    http://host:ip/json_api.1.0/get_system_info
    
返回格式

    {
        "errcode" : errcode,
        "errmsg" : errmsg,
        "total_store_node_size" : total_store_node_size,
        "used_store_node_size" : used_store_node_size,
        "total_fs_dir_count" : total_fs_dir_count,
        "total_fs_file_count" : total_fs_file_count,
        "total_fs_store_size" : total_fs_store_size,
        "root_online_info" : [online_num, offline_num, err_num],
        "store_online_info" : [online_num, offline_num, err_num],
        "client_online_info" : [online_num, offline_num, err_num],
        
    }
    
说明

    total_store_node_size: 总存储空间
    used_store_node_size: 使用存储空间
    total_fs_dir_count: 总的目录数量
    total_fs_file_count: 总的文件数量
    total_fs_store_size: 总的文件大小
    root_online_info: root在线信息 [在线数,离线数,异常数]
    store_online_info: store在线信息 [在线数,离线数,异常数]
    client_online_info: client在线信息 [在线数，离线数，异常数]
    
###获取StoreServer基本信息
请求URL

    http://host:ip/json_api.1.0/get_store_base_info?node_id=$node_id
    http://host:ip/json_api.1.0/get_store_base_info?ip=$ip&port=$port
    
返回格式

    {
        "errcode" : errcode,
        "errmsg" : errmsg,
        "node_id" : node_id,
        "ip" : ip,
        "port" : port,
        "total_store_size" : total_store_size,
        "used_store_size" : used_store_size,
        "max_download_speed" : max_download_speed,
        "max_upload_speed" : max_upload_speed,
        "download_speed" : download_speed,
        "upload_speed" : upload_speed
    }
    
说明

    node_id: node_id
    ip: ip
    port: port 
    total_store_size: 总存储空间
    used_store_size: 使用存储空间
    max_download_speed: 最大上传速度
    max_upload_speed: 最大下载速度
    download_speed: 上传速度
    upload_speed: 下载速度
    
    
###获取RootServer基本信息
请求URL

    http://host:ip/json_api.1.0/get_root_base_info?node_id=$node_id
    http://host:ip/json_api.1.0/get_root_base_info?ip=$ip&port=$port
    
返回格式

    暂无
    
###获取StoreServer速度详细数据
请求URL
    
    http://host:ip/json_api.1.0/get_store_speed_detail_info?node_id=$node_id
    http://host:ip/json_api.1.0/get_store_speed_detail_info?ip=$ip&port=$port
    
返回格式

    {
        "errcode" : errcode,
        "errmsg" : errmsg,
        "node_id" : node_id,
        "ip" : ip,
        "port" : port,
        "upload_speed":
        {
            "last_hour" : [1, 2, 3, ..., 12],
            "last_day" : [1, 2, 3, ..., 24],
            "last_week" : [1, 2, ..., 7],
            "last_month" : [1, 2, ..., 30]
        },
        "download_speed":
        {
            "last_hour" : [1, 2, 3, ..., 12],
            "last_day" : [1, 2, 3, ..., 24],
            "last_week" : [1, 2, ..., 7],
            "last_month" : [1, 2, ..., 30]
        }
    }
    
说明

    node_id: node_id
    ip: ip
    port: port
    upload_speed: 上传速度采样数据，分别为过去1小时（采样间隔5分钟），过去1天（采样间隔一小时），过去一周（采样间隔1天），过去一个月（采样间隔1天）
    download_speed: 下载速度采样数据，同上
    
###获取Page流转信息
请求URL

    http://host:ip/json_api.1.0/get_page_forward_info?path_id=$path_id&page_index=$page_index
    
返回格式

    {
        "errcode" : errcode,
        "errmsg" : errmsg,
        "path_id" : path_id,
        "page_index" : page_index,
        "page_size": page_size,
        "replicas_num": replicas_num,
        "min_replicas_num": replicas_num,
        "finish_replicas_num": finish_replicas_num
        "page_version" : page_version,
        "forward_info": 
        [
            {
                "type" : "read or write",
                "self_node_id": self_node_id,
                "self_ip": self_ip,
                "self_port", self_port,
                "handle_info": [time_cost, flag],
                "time_info": 
                [
                    {
                        "peer_node_id": peer_node_id,
                        "peer_ip": peer_ip,
                        "peer_port": peer_port,
                        "send_info": [time_cost, flag],
                        "recv_info": [time_cost, flag],
                    },..., {}
                ]
            },..., {}
        ]
    }
    
说明

    path_id: 文件的路径
    page_index: page编号
    page_size: page大小（单位Byte）
    page_version: page版本号
    replicas_num: 副本数
    min_replicas_num: 最小副本数
    finish_replicas_num: 已完成副本数
    forward_info: forward信息，列表每个元素代表一层forward信息，或者是读取信息
        type: 操作类型，如果是read则代表读取操作，time_info只有一个元素，如果是write则代表写操作，time_info可能有多个元素
        self_node_id: 自身node_id
        self_ip: 自身ip
        self_port: 自身端口
        handle_info: [自身处理耗时, 标记］
        time_info: 每个forward（或读取）操作信息及耗时
            peer_node_id: 对方node_id
            peer_ip: 对方ip
            peer_port: 对方port
            send_info: [发送网络耗时, 标记]
            recv_info: [接收网络耗时, 标记]
            
     handle_info, send_info, recv_info中的flag表示时间的异常情况，0代表正常，1代表失败，2代表耗时超出预期
    
    