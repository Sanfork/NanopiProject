1.WIFI 热点名nanopi-wifiap   密码123456789
      IP地址：192.168.8.1
2.ssh 登陆地址192.168.8.1
  方法 ：  ssh root@192.168.0.1  密码fa
      或   ssh hiram@192.168.0.1 密码hi      
（hiram非root用户，需要转到root用户时 输入su – 回车后，输入root用户密码 fa）
3. UART功能硬件口为UART3口，在GPIO1插口中8pin（TXD3），10pin（RXD3），如下图
4.UART串口文件位置及名称：/dev/ttySAC3
5.透传功能UART属性：波特率：9600   数据位： 8    奇偶校验：奇校验  停止位：1
6.目前透传的WIFI的socket监听端口号为8888，目前开机已启动透传功能
7.硬件板使用说明链接：http://wiki.friendlyarm.com/wiki/index.php/NanoPi/zh

