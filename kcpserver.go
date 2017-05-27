package main

import (
	"fmt"
	"crypto/sha1"

	"github.com/xtaci/kcp-go"
	"golang.org/x/crypto/pbkdf2"
	"time"
)

const port = ":9999"
var (
	// VERSION is injected by buildflags
	VERSION = "SELFBUILD"
	// SALT is use for pbkdf2 key expansion
	SALT = "kcp-go"
)

func ListenTest() (*kcp.Listener, error) {
	var block kcp.BlockCrypt
	pass := pbkdf2.Key([]byte("it's a secrect"), []byte(SALT), 4096, 32, sha1.New)
	fmt.Println("key",pass)
	block, _ = kcp.NewAESBlockCrypt([]byte("1234567890123456789012345678901234567890"))
	return kcp.ListenWithOptions(port, block, 2, 2)
}

func server() {
	l, err := ListenTest()
	if err != nil {
		panic(err)
	}
	l.SetDSCP(46)
	for {
		s, err := l.AcceptKCP()
		if err != nil {
			panic(err)
		}

		go handle_client(s)
	}
}
func handle_client(conn *kcp.UDPSession) {
	conn.SetWindowSize(1024, 1024)
	conn.SetNoDelay(1, 20, 2, 1)
	conn.SetStreamMode(false)
	fmt.Println("new client", conn.RemoteAddr())
	buf := make([]byte, 65536)
	count := 0
	for {
		n, err := conn.Read(buf)
		if err != nil {
			panic(err)
		}
		count++
		
		//fmt.Println("received :" ,time.Now().Format("2006-01-02 15:04:05"),n)
		fmt.Println("received:", string(buf[:n]))
		conn.Write(buf[:n])
	}
}

func main() {
	fmt.Println("start :" ,time.Now().Format("2006-01-02 15:04:05"))
	server()
}
