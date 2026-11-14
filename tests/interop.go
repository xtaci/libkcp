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

// Run from the kcp-go module: go run /path/to/libkcp/tests/interop.go /path/to/kcp_test
package main

import (
	"context"
	"crypto/cipher"
	"crypto/subtle"
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"net"
	"os"
	"os/exec"
	"strconv"
	"sync/atomic"
	"time"

	kcp "github.com/xtaci/kcp-go/v5"
)

// testCase describes one matching Go and C packet cipher.
type testCase struct {
	name   string                               // Scenario label.
	id     int                                  // C API algorithm identifier.
	keylen int                                  // Test key length in bytes.
	new    func([]byte) (kcp.BlockCrypt, error) // Go cipher constructor.
}

// fecCase is the sender's data/parity shard layout.
type fecCase struct {
	data, parity int // Data and parity shards per group.
}

const messageSize = 65536

// This intentionally insecure AEAD tests only the custom callback wire format.
type testAEAD struct{ key [16]byte } // Shared test-only key.

// NonceSize reports the test AEAD nonce length.
func (a *testAEAD) NonceSize() int { return 12 }

// Overhead reports the test AEAD tag length.
func (a *testAEAD) Overhead() int { return 16 }

// tag computes the intentionally insecure test-only checksum.
func (a *testAEAD) tag(nonce, data []byte) [16]byte {
	hash := uint32(2166136261)
	for _, part := range [][]byte{a.key[:], nonce, data} {
		for _, b := range part {
			hash = (hash ^ uint32(b)) * 16777619
		}
	}
	var tag [16]byte
	for i := range tag {
		tag[i] = byte(hash >> (8 * (i % 4)))
	}
	return tag
}

// appendSpace reserves output space while preserving the original prefix.
func appendSpace(dst []byte, n int) ([]byte, []byte) {
	old := len(dst)
	if cap(dst)-old >= n {
		dst = dst[:old+n]
	} else {
		b := make([]byte, old+n)
		copy(b, dst)
		dst = b
	}
	return dst, dst[old:]
}

// Seal writes the test-only ciphertext and tag.
func (a *testAEAD) Seal(dst, nonce, plaintext, additionalData []byte) []byte {
	if len(nonce) != a.NonceSize() || len(additionalData) != 0 {
		panic("invalid test AEAD input")
	}
	out, tail := appendSpace(dst, len(plaintext)+a.Overhead())
	copy(tail, plaintext)
	for i := range plaintext {
		tail[i] ^= a.key[i%len(a.key)] ^ nonce[i%len(nonce)]
	}
	tag := a.tag(nonce, tail[:len(plaintext)])
	copy(tail[len(plaintext):], tag[:])
	return out
}

// Open verifies the test tag before returning plaintext.
func (a *testAEAD) Open(dst, nonce, ciphertext, additionalData []byte) ([]byte, error) {
	if len(nonce) != a.NonceSize() || len(additionalData) != 0 || len(ciphertext) < a.Overhead() {
		return nil, errors.New("invalid test AEAD input")
	}
	size := len(ciphertext) - a.Overhead()
	tag := a.tag(nonce, ciphertext[:size])
	if subtle.ConstantTimeCompare(tag[:], ciphertext[size:]) != 1 {
		return nil, errors.New("test AEAD authentication failed")
	}
	out, tail := appendSpace(dst, size)
	copy(tail, ciphertext[:size])
	for i := range tail {
		tail[i] ^= a.key[i%len(a.key)] ^ nonce[i%len(nonce)]
	}
	return out, nil
}

// main runs the encryption, FEC and auto-tuning interoperability matrix.
func main() {
	if len(os.Args) != 2 {
		fmt.Fprintln(os.Stderr, "usage: interop <kcp_test executable>")
		os.Exit(2)
	}
	cases := []testCase{
		{"plain", -1, 16, func([]byte) (kcp.BlockCrypt, error) { return nil, nil }},
		{"aes-gcm-128", 0, 16, kcp.NewAESGCMCrypt},
		{"aes-gcm-192", 0, 24, kcp.NewAESGCMCrypt},
		{"aes-gcm-256", 0, 32, kcp.NewAESGCMCrypt},
		{"aes-128", 1, 16, kcp.NewAESBlockCrypt},
		{"aes-192", 1, 24, kcp.NewAESBlockCrypt},
		{"aes-256", 1, 32, kcp.NewAESBlockCrypt},
		{"twofish-128", 3, 16, kcp.NewTwofishBlockCrypt},
		{"twofish-192", 3, 24, kcp.NewTwofishBlockCrypt},
		{"twofish-256", 3, 32, kcp.NewTwofishBlockCrypt},
		{"3des", 4, 24, kcp.NewTripleDESBlockCrypt},
		{"cast5", 5, 16, kcp.NewCast5BlockCrypt},
		{"blowfish", 6, 16, kcp.NewBlowfishBlockCrypt},
		{"blowfish-short", 6, 3, kcp.NewBlowfishBlockCrypt},
		{"tea", 7, 16, kcp.NewTEABlockCrypt},
		{"xtea", 8, 16, kcp.NewXTEABlockCrypt},
		{"salsa20", 9, 32, kcp.NewSalsa20BlockCrypt},
		{"salsa20-short", 9, 16, kcp.NewSalsa20BlockCrypt},
		{"xor", 10, 16, kcp.NewSimpleXORBlockCrypt},
		{"none", 11, 16, kcp.NewNoneBlockCrypt},
		{"custom-aead", 12, 16, func(key []byte) (kcp.BlockCrypt, error) {
			aead := new(testAEAD)
			copy(aead.key[:], key)
			return kcp.NewAEADCrypt(aead), nil
		}},
	}
	fecCases := []fecCase{{0, 0}, {1, 1}, {2, 2}, {2, 3}, {3, 2}, {5, 3}, {10, 3}}
	total := 0
	for _, tc := range cases {
		for _, fec := range fecCases {
			losses := []int{0}
			if fec.data > 0 {
				maxLoss := fec.parity
				if maxLoss > fec.data {
					maxLoss = fec.data
				}
				losses = append(losses, maxLoss)
			}
			for _, loss := range losses {
				if err := run(tc, fec, fec, loss, loss, 1, messageSize, os.Args[1]); err != nil {
					fmt.Fprintf(os.Stderr, "%s fec=%d+%d loss=%d: %v\n", tc.name, fec.data, fec.parity, loss, err)
					os.Exit(1)
				}
				total++
				fmt.Printf("ok %s fec=%d+%d loss=%d\n", tc.name, fec.data, fec.parity, loss)
			}
		}
	}
	autoCases := []struct{ client, server fecCase }{
		{fecCase{2, 2}, fecCase{5, 3}},
		{fecCase{5, 3}, fecCase{2, 2}},
		{fecCase{0, 0}, fecCase{5, 3}},
		{fecCase{5, 3}, fecCase{0, 0}},
		{fecCase{1, 1}, fecCase{10, 3}},
	}
	for _, tc := range []testCase{cases[0], cases[1], cases[4], cases[len(cases)-1]} {
		for _, pair := range autoCases {
			for _, withLoss := range []bool{false, true} {
				clientLoss, serverLoss := 0, 0
				if withLoss {
					clientLoss = min(pair.client.data, pair.client.parity)
					serverLoss = min(pair.server.data, pair.server.parity)
				}
				if err := run(tc, pair.client, pair.server, clientLoss, serverLoss, 3, 131072, os.Args[1]); err != nil {
					fmt.Fprintf(os.Stderr, "%s autotune client=%d+%d server=%d+%d loss=%t: %v\n",
						tc.name, pair.client.data, pair.client.parity, pair.server.data, pair.server.parity, withLoss, err)
					os.Exit(1)
				}
				total++
				fmt.Printf("ok %s autotune client=%d+%d server=%d+%d loss=%t\n",
					tc.name, pair.client.data, pair.client.parity, pair.server.data, pair.server.parity, withLoss)
			}
		}
	}
	fmt.Printf("passed %d interoperability cases\n", total)
}

// run sends and verifies an echo through the requested FEC and loss settings.
func run(tc testCase, clientFEC, serverFEC fecCase, clientLoss, serverLoss int, minGroup uint32, size int, client string) error {
	key := make([]byte, tc.keylen)
	for i := range key {
		key[i] = byte(i)
	}
	crypt, err := tc.new(key)
	if err != nil {
		return err
	}
	listener, err := kcp.ListenWithOptions("127.0.0.1:0", crypt, serverFEC.data, serverFEC.parity)
	if err != nil {
		return err
	}
	defer listener.Close()
	port := listener.Addr().(*net.UDPAddr).Port
	var droppedClient, droppedServer *atomic.Int32
	if clientLoss > 0 || serverLoss > 0 {
		proxy, clientDrop, serverDrop, err := lossProxy(listener.Addr().(*net.UDPAddr), crypt,
			clientFEC, serverFEC, clientLoss, serverLoss, minGroup)
		if err != nil {
			return err
		}
		defer proxy.Close()
		droppedClient, droppedServer = clientDrop, serverDrop
		port = proxy.LocalAddr().(*net.UDPAddr).Port
	}
	serverDone := make(chan error, 1)
	clientDone := make(chan struct{})
	go func() {
		s, err := listener.AcceptKCP()
		if err != nil {
			serverDone <- err
			return
		}
		defer s.Close()
		s.SetNoDelay(1, 20, 2, 1)
		s.SetWindowSize(128, 128)
		if err = s.SetDeadline(time.Now().Add(10 * time.Second)); err == nil {
			message := make([]byte, size)
			if _, err = io.ReadFull(s, message); err == nil {
				var written int
				written, err = s.Write(message)
				if err == nil && written != len(message) {
					err = io.ErrShortWrite
				}
			}
		}
		serverDone <- err
		<-clientDone
	}()
	ctx, cancel := context.WithTimeout(context.Background(), 12*time.Second)
	defer cancel()
	cmd := exec.CommandContext(ctx, client, strconv.Itoa(port), strconv.Itoa(clientFEC.data),
		strconv.Itoa(clientFEC.parity), strconv.Itoa(tc.id), strconv.Itoa(tc.keylen), strconv.Itoa(size))
	output, err := cmd.CombinedOutput()
	close(clientDone)
	if err != nil {
		select {
		case serverErr := <-serverDone:
			return fmt.Errorf("client: %w: %s (server: %v)", err, output, serverErr)
		default:
			return fmt.Errorf("client: %w: %s (server still waiting)", err, output)
		}
	}
	if (clientLoss > 0 || serverLoss > 0) &&
		(int(droppedClient.Load()) != clientLoss || int(droppedServer.Load()) != serverLoss) {
		return fmt.Errorf("proxy selected %d client and %d server segments; expected %d and %d",
			droppedClient.Load(), droppedServer.Load(), clientLoss, serverLoss)
	}
	return <-serverDone
}

// Drop data shards from one complete FEC group in each direction, and block
// every retransmission of their selected KCP segments.
func lossProxy(target *net.UDPAddr, crypt kcp.BlockCrypt, clientFEC, serverFEC fecCase,
	clientLoss, serverLoss int, minGroup uint32) (*net.UDPConn, *atomic.Int32, *atomic.Int32, error) {
	front, err := net.ListenUDP("udp", &net.UDPAddr{IP: net.IPv4(127, 0, 0, 1)})
	if err != nil {
		return nil, nil, nil, err
	}
	back, err := net.DialUDP("udp", nil, target)
	if err != nil {
		front.Close()
		return nil, nil, nil, err
	}
	clientDrop, serverDrop := new(atomic.Int32), new(atomic.Int32)
	dropClient := dropPush(crypt, clientFEC, clientLoss, minGroup, clientDrop)
	dropServer := dropPush(crypt, serverFEC, serverLoss, minGroup, serverDrop)
	client := make(chan *net.UDPAddr, 1)
	go func() {
		defer back.Close()
		buf := make([]byte, 2048)
		var remote *net.UDPAddr
		for {
			n, addr, err := front.ReadFromUDP(buf)
			if err != nil {
				return
			}
			if remote == nil {
				remote = addr
				client <- addr
			}
			if err := dropClient(buf[:n], func(packet []byte) error {
				_, err := back.Write(packet)
				return err
			}); err != nil {
				return
			}
		}
	}()
	go func() {
		addr := <-client
		buf := make([]byte, 2048)
		for {
			n, err := back.Read(buf)
			if err != nil {
				return
			}
			if err := dropServer(buf[:n], func(packet []byte) error {
				_, err := front.WriteToUDP(packet, addr)
				return err
			}); err != nil {
				return
			}
		}
	}()
	return front, clientDrop, serverDrop, nil
}

// bufferedShard retains a datagram and its KCP data sequence numbers.
type bufferedShard struct {
	packet []byte   // Original encrypted or plaintext datagram.
	pushes []uint32 // KCP PUSH sequence numbers carried by this shard.
}

// dropPush selects one complete FEC group and blocks its chosen data and retransmissions.
func dropPush(crypt kcp.BlockCrypt, fec fecCase, loss int, minGroup uint32, dropped *atomic.Int32) func([]byte, func([]byte) error) error {
	if loss == 0 {
		return func(packet []byte, send func([]byte) error) error { return send(packet) }
	}
	targets := make(map[uint32]bool)
	seen := make(map[uint32]bool)
	var group uint32
	var pending []bufferedShard
	dataIndices := make(map[uint32]bool)
	parityIndices := make(map[uint32]bool)
	selected := false

	flush := func(send func([]byte) error, selectTargets bool) error {
		if selectTargets && len(dataIndices) == fec.data && len(parityIndices) >= loss {
			occurrences := make(map[uint32]int)
			for _, shard := range pending {
				for _, sn := range shard.pushes {
					occurrences[sn]++
				}
			}
			chosen := make(map[uint32]bool)
			for _, shard := range pending {
				for _, sn := range shard.pushes {
					if !seen[sn] && occurrences[sn] == 1 {
						chosen[sn] = true
						break
					}
				}
				if len(chosen) == loss {
					break
				}
			}
			if len(chosen) == loss {
				targets = chosen
				selected = true
				dropped.Store(int32(loss))
			}
		}
		for _, shard := range pending {
			drop := false
			for _, sn := range shard.pushes {
				seen[sn] = true
				drop = drop || targets[sn]
			}
			if !drop {
				if err := send(shard.packet); err != nil {
					return err
				}
			}
		}
		pending = nil
		clear(dataIndices)
		clear(parityIndices)
		return nil
	}

	return func(packet []byte, send func([]byte) error) error {
		data := append([]byte(nil), packet...)
		if aead, ok := crypt.(cipher.AEAD); ok {
			if len(data) < aead.NonceSize()+aead.Overhead() {
				return send(packet)
			}
			nonce := data[:aead.NonceSize()]
			var err error
			data, err = aead.Open(nil, nonce, data[aead.NonceSize():], nil)
			if err != nil {
				return send(packet)
			}
		} else if crypt != nil {
			if len(data) < 20 {
				return send(packet)
			}
			crypt.Decrypt(data, data)
			data = data[20:]
		}
		if len(data) < 8 {
			return send(packet)
		}
		seq := binary.LittleEndian.Uint32(data[:4])
		flag := binary.LittleEndian.Uint16(data[4:6])
		if flag != 0xf1 && flag != 0xf2 {
			return send(packet)
		}
		total := uint32(fec.data + fec.parity)
		packetGroup := seq / total
		index := seq % total
		var pushes []uint32
		if flag == 0xf1 {
			data = data[8:]
			for len(data) >= 24 {
				length := int(binary.LittleEndian.Uint32(data[20:24]))
				if length > len(data)-24 {
					return send(packet)
				}
				if data[4] == 81 {
					pushes = append(pushes, binary.LittleEndian.Uint32(data[12:16]))
				}
				data = data[24+length:]
			}
		}
		if len(pending) > 0 && packetGroup != group {
			if err := flush(send, false); err != nil {
				return err
			}
		}
		if !selected && len(pending) == 0 && packetGroup >= minGroup && flag == 0xf1 && index == 0 {
			group = packetGroup
			pending = make([]bufferedShard, 0, fec.data)
		}
		if !selected && pending != nil && packetGroup == group {
			indices := dataIndices
			if flag == 0xf2 {
				indices = parityIndices
			}
			if !indices[index] {
				pending = append(pending, bufferedShard{
					packet: append([]byte(nil), packet...), pushes: pushes,
				})
				indices[index] = true
			}
			if len(dataIndices) == fec.data && len(parityIndices) >= loss {
				return flush(send, true)
			}
			return nil
		}
		for _, sn := range pushes {
			seen[sn] = true
			if targets[sn] {
				return nil
			}
		}
		return send(packet)
	}
}
