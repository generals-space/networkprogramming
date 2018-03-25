# CLOSE_WAIT与TIME_WAIT

本例中只能实现`CLOSE_WAIT`, 无法实现`TIME_WAIT`.

在启动服务端和客户端建立连接后, 将服务端所在终端强制关闭. 端口状态将如下

![](https://gitimg.generals.space/cd1cc5696526e3b8ad68fee02f11a5f1.png)

其中`FIN_WAIT2`会在一段时间后自动消失, 但是如果客户端没有发送操作, 就会一直保持`CLOSE_WAIT`状态, 程序依然活得好好的.

只有当客户端尝试向服务端发送信息, 客户端才会重新刷新socket状态, 此时已经不存在连接, 一般会get到一个`Broken Pipe`的错误.

![](https://gitimg.generals.space/69b476f9f8ad86fdf1afc8f3937d7e91.png)

看来, 客户端如果意外断开, 服务端将收到空字符串...

对于`TIME_WAIT`状态, 本例无法重现. 目前还不清楚`TIME_WAIT`产生的条件.