
..................rpm包安装命令........................................................

rpm -i banana_cloud_loadserver_1.001.x86_64.rpm



..................服务器启动命令...................................................   

loadserver -d -t tcp (后台启动不加"-d")




..................服务器配置(PATH:/etc/private_cloud/loadserver/loadserver.conf)........................
localip		     "127.0.0.1"	     //本地监听IP地址
localport            5800                    //本地监听端口     
shouldrestart        yes		     //是否自动重启
addrserverip        "addr.bananacloud.com"   //寻址服务器域名
addrserverport       5900	             //寻址服务器监听端口
shouldlog            yes		     //是否支持日志打印	
ScreenLoggingEnabled yes				
pidfilepath          "/var/run/LoadServer.pid"         
logdir               "/var/private_cloud/loadserver/"  //日志路径
logname              "error"                           //日志名称
bserveripconf        "/etc/private_cloud/loadserver/serverip.conf"   //业务服务器默认配置
rollinday            1                                               //日志回滚间隔(天)
maxlogbytes          10649600       #100M                            //最大日志
Verbosity            4                                               //调试级别

DataBaseUserName     "loadserver"                                    //数据库用户名（需要权限：查询，插入，更新，删除）
DataBasePassWord     "adv"                                           //数据库密码
DataBaseServerIP     "127.0.0.1"                                     //数据库IP
DataBaseName         "loadserver"                                    //数据库名


...............数据库安装脚本(PATH: ./mysql-sript/initsql.sql)........................

   
   
