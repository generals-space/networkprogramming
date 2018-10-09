package main
import (
        "net"
		"log"
		"fmt"
)

func main(){
	var msg string
	// 相当于connect()函数
	client, err := net.Dial("tcp", "127.0.0.1:7777")
	if err != nil{
		log.Fatal(err)
	}
	log.Printf("已连接到服务器...")

	buf := make([]byte, 1204)
	length, err := client.Read(buf)
	if err != nil{
		log.Fatal(err)
	}
	log.Printf(string(buf[:length]))

	for true{
		// &, 读入指针
		fmt.Scanf("%s", &msg)

		client.Write([]byte(msg))

		if msg == "exit"{
			client.Close()
			break
		}else{
			length, err := client.Read(buf)
			if err != nil{
				log.Fatal(err)
			}
			log.Printf(string(buf[:length]))
		}
	}
}