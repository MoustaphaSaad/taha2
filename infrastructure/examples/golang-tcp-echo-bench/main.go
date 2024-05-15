package main

import (
	"flag"
	"fmt"
	"log"
	"net"
	"sync"
	"sync/atomic"
	"time"
)

var (
	targetAddr  = flag.String("a", "127.0.0.1:8080", "target echo server address")
	testMsgLen  = flag.Int("l", 128, "test message length")
	testConnNum = flag.Int("c", 200, "test connection number")
	testSeconds = flag.Int("t", 10, "test duration in seconds")
	testRampUp = flag.Int("r", 5, "test ramp in seconds")
)

func main() {
	flag.Parse()

	var (
		outNum uint64
		inNum  uint64
		stop   uint64
	)

	msg := make([]byte, *testMsgLen)
	conns := make([]net.Conn, *testConnNum)
	clientsNum := 0

	rampUpTime := float64(*testRampUp) * 1000
	waitAfterEachConn := int((rampUpTime / float64(*testConnNum)))
	log.Println(waitAfterEachConn)
	for i := 0; i < *testConnNum; i++ {
		if conn, err := net.DialTimeout("tcp", *targetAddr, time.Minute*99999); err == nil {
			log.Printf("%v/%v connections\n", i + 1, *testConnNum)
			conns[i] = conn
			clientsNum++
		} else {
			log.Println(err)
		}
		time.Sleep(time.Millisecond * time.Duration(waitAfterEachConn))
	}

	go func() {
		time.Sleep(time.Second * time.Duration(*testSeconds))
		atomic.StoreUint64(&stop, 1)
	}()

	wg := new(sync.WaitGroup)

	for i := 0; i < *testConnNum; i++ {
		wg.Add(1)
		if conns[i] == nil {
			continue
		}

		go func(conn net.Conn) {
			recv := make([]byte, len(msg))

			for {
				if _, err := conn.Write(msg); err != nil {
					log.Println(err)
					break
				}

				atomic.AddUint64(&outNum, 1)

				if atomic.LoadUint64(&stop) == 1 {
					break
				}

				if _, err := conn.Read(recv); err != nil {
					log.Println(err)
					break
				}

				atomic.AddUint64(&inNum, 1)

				if atomic.LoadUint64(&stop) == 1 {
					break
				}
			}

			wg.Done()
		}(conns[i])
	}

	wg.Wait()

	fmt.Println("Benchmarking:", *targetAddr)
	fmt.Println(clientsNum, "clients, running", *testMsgLen, "bytes,", *testSeconds, "sec.")
	fmt.Println()
	fmt.Println("Speed:", outNum/uint64(*testSeconds), "request/sec,", inNum/uint64(*testSeconds), "response/sec")
	fmt.Println("Requests:", outNum)
	fmt.Println("Responses:", inNum)
}
