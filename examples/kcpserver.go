/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 xtaci
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

package main

import (
	"fmt"

	"github.com/xtaci/kcp-go/v5"
)

const port = ":9999"

// ListenTest starts the example listener with 2+2 FEC.
func ListenTest() (*kcp.Listener, error) {
	return kcp.ListenWithOptions(port, nil, 2, 2)
}

// server accepts KCP sessions and handles each client concurrently.
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

// handle_client echoes complete messages until the session closes.
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
			return
		}
		count++
		fmt.Printf("received packet %d: %d bytes\n", count, n)
		if _, err := conn.Write(buf[:n]); err != nil {
			return
		}
	}
}

// main starts the example server.
func main() {
	server()
}
