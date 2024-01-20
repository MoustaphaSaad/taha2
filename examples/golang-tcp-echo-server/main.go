package main

import "net"

func main() {
	if lsn, err := net.Listen("tcp", "127.0.0.1:8080"); err == nil {
		for {
			conn, err := lsn.Accept()

			if err != nil {
				break
			}

			go func() {
				conn.(*net.TCPConn).ReadFrom(conn)
			}()
		}
	}
}