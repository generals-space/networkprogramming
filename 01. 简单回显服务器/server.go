package main
import (
	"net"
	"log"
)

func main(){
	var data string
	var msg string
	listen_fd, err := net.Listen("tcp", "0.0.0.0:7777")

	log.Printf("开始监听...")

	if err != nil {
		log.Fatal(err)
	}

	conn_fd, err := listen_fd.Accept()
	if err != nil{
		log.Fatal(err)
	}
	addr := conn_fd.RemoteAddr().String()
	log.Printf("收到来自 %s 的连接", addr)
	conn_fd.Write([]byte("欢迎!"))

	for true{
		buf := make([]byte, 1024)
		length, err := conn_fd.Read(buf)
		if err != nil{
		    log.Fatal(err)
		}
		data = string(buf[:length])
		log.Printf("收到来自 %s 的信息: %s", addr, data)
		if data == "exit" {
		    log.Printf("客户端 %s 断开连接...", addr)
		    conn_fd.Close()
		    break
		}

		msg = "Hello, " + data
		conn_fd.Write([]byte(msg))
	}
	log.Printf("停止监听...")
	listen_fd.Close()
}
