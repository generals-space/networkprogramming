package main

import (
	"log"
	"net"
	"time"
)

func listen(server string, conn *net.UDPConn) {
	buf := make([]byte, 1024)
	for {
		len, client, err := conn.ReadFromUDP(buf)
		if err != nil {
			panic(err)
		}
		log.Printf("Server %s got: %s from %+v\n", server, buf[:len], client)
	}
}

/*
 * 本程序展示了两种向UDP服务器发送信息的方法,
 * 1. 常规的, 由客户端Dial再发送数据
 * 2. 服务器端也可以直接向另一个服务器发送数据, 因为不需要Dial, 直接向着一个合法的UDP地址写数据就可以了
 */
func main() {
	// 创建TCP服务器的方式, 不能用于创建UDP服务器, Listen()的第一个参数为tcp或unix, 不能是udp
	// serverConn2, err := net.Listen("udp", ":1379")

	// 创建UDP服务器的方式
	// :0表示随机生成一个端口, udpAddr1也没有确定的值.
	// 在服务端监听成功后返回的对象有一个LocalAddr()方法,
	// 能够得到这个端口的具体值, TCP里不好这么用.
	udpAddr1, err := net.ResolveUDPAddr("udp", ":0")
	log.Println("server 1 udp addr: ", udpAddr1)
	if err != nil {
		panic(err)
	}

	serverConn1, err := net.ListenUDP("udp", udpAddr1)
	serverAddr1 := serverConn1.LocalAddr().String() // 这个对象里包含了此次监听的端口值
	log.Println("server 1 server addr: ", serverAddr1)
	go listen(serverAddr1, serverConn1)

	udpAddr2, err := net.ResolveUDPAddr("udp", "127.0.0.1:7777")
	if err != nil {
		panic(err)
	}
	serverConn2, err := net.ListenUDP("udp", udpAddr2)
	serverAddr2 := serverConn2.LocalAddr().String() // 这个对象里包含了此次监听的端口值
	go listen(serverAddr2, serverConn2)

	//////////////////////////////////////////////////////////////////////////////

	// 第一种发包的形式, 常规方法, 客户端Dial再发送
	client, err := net.Dial("udp", serverAddr2)
	if err != nil {
		panic(err)
	}

	msg1 := []byte("hello server2, I am client")
	_, err = client.Write(msg1)
	if err != nil {
		panic(err)
	}

	// 第二种发包的形式, 非主流, 服务端对服务端发送...
	// 注意: server2对server1发送在本程序内是不可以的,
	// 因为udpAddr1的端口值并不确定, 无法路由, serverAddr1可以但是类型和WriteToUDP所需参数类型不匹配.
	msg2 := []byte("hello server2, I am server1")
	_, err = serverConn1.WriteToUDP(msg2, udpAddr2)
	if err != nil {
		panic(err)
	}
	time.Sleep(time.Second * 60)
}

/*
 * 执行结果:
2018/09/04 16:35:38 server 1 udp addr:  :0
2018/09/04 16:35:38 server 1 server addr:  [::]:55496
2018/09/04 16:35:38 Server 127.0.0.1:7777 got: hello server2, I am client from 127.0.0.1:55396
2018/09/04 16:35:38 Server 127.0.0.1:7777 got: hello server2, I am server1 from 127.0.0.1:55496
 */
