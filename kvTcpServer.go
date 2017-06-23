package main

import "fmt"
import "time"

import "encoding/json"
import "bufio"
import "strings"
import "os"

import "net"

type A struct {
	Sfds string
	Sftt int
}

var t1 *time.Timer

func dingshi() {
	for {
		select {
		case <-t1.C:
			t1.Reset(5 * time.Second)
			fmt.Println("ok")
		}
	}
}

var kvvalue map[string]net.Conn = make(map[string]net.Conn)

func insertKv(key string, con net.Conn) {
	_, err := kvvalue[key]
	if !err {
		//v1.Close()
		kvvalue[key] = con

	} else {
		kvvalue[key] = con
	}
}

func fazhan_ok(mtype string) {
	var zhi = make([]byte, 2048)

	mtype = strings.TrimRight(mtype, "\n")

	type CommandStats struct {
		Jsonrpc string
		Id      string
		Params  string
		Method  string
	}

	type CommandData struct {
		Etype string
		Edata CommandStats
	}

	addrVe := strings.Split(mtype, " ")

	var result CommandData

	if len(addrVe) >= 2 {

		v1 := strings.TrimSpace(addrVe[0])

		v2 := strings.TrimSpace(addrVe[1])

		addrVe = addrVe[2:]

		command := strings.Join(addrVe, " ")
		if v2 == "2" {
			result = CommandData{
				Etype: v2,
				Edata: CommandStats{
					Id:     "2",
					Params: command,
					Method: "Reds",
				},
			}
		} else if v2 == "6" {
			comExeStr := "#!/bin/sh \nsleep 100 \necho \"jiang\""
			result = CommandData{
				Etype: v2,
				Edata: CommandStats{
					Id:     "2",
					Params: comExeStr,
					Method: "Reds",
				},
			}

		} else if v2 == "3" {
			comExeStr := "ok"
			result = CommandData{
				Etype: v2,
				Edata: CommandStats{
					Id:     "2",
					Params: comExeStr,
					Method: "Reds",
				},
			}
		} else {
			fmt.Println("error no this command!")
			return
		}

		val, _ := json.Marshal(result)

		fmt.Println("shuchu shuju ", command)

		cn, err := kvvalue[v1]
		if !err {

			fmt.Println("error no this mac!")
		} else {

			_, e1 := cn.Write(val)
			if e1 == nil {
				_, e2 := cn.Read(zhi)
				if e2 == nil {
					fmt.Println(string(zhi))

				} else {
					cn.Close()
					delete(kvvalue, v1)
					fmt.Println("recv error")
				}

			} else {
				cn.Close()
				delete(kvvalue, v1)
				fmt.Println("socket close")
			}

		}
	} else {

	}

}

func mac_type_ok(conTcp net.Conn) {

	var zhi = make([]byte, 2048)

	type CommandStats struct {
		Jsonrpc string
		Id      string
		Params  string
		Method  string
	}

	type CommandData struct {
		Etype string
		Edata CommandStats
	}
	result := CommandData{
		Etype: "1",
		Edata: CommandStats{
			Id:     "1",
			Params: "1",
			Method: "Reds",
		},
	}

	val, _ := json.Marshal(result)
	conTcp.Write(val)
	conTcp.Read(zhi)
	//取设备mac 标识字符作为唯一key
	zhi = zhi[12:18]

	trueMac := fmt.Sprintf("%x", zhi)

	insertKv(trueMac, conTcp)

	fmt.Println("add tcp ok ", trueMac)
}

func qidongServer() {
	c1, _ := net.ResolveTCPAddr("tcp", ":9527")
	d1, _ := net.ListenTCP("tcp", c1)
	for {
		f1, err := d1.Accept()
		if err == nil {
			fmt.Println("ok!", f1.RemoteAddr().String())
			//			f1.SetDeadline(time.Second * 5)
			mac_type_ok(f1)
			//insertKv("1", f1)
		}
	}

}

var inputReader *bufio.Reader
var input string
var err error

func main() {
	go qidongServer()

	inputReader = bufio.NewReader(os.Stdin)
	fmt.Println("Please enter some input: ")
	for {
		input, err = inputReader.ReadString('\n') //func (b *Reader) ReadString(delim byte) (line string, err error) ,‘S’ 这个例子里使用S表示结束符，也可以用其它，如'\n'
		if err == nil {
			fmt.Println("The input was:" + input)
			go fazhan_ok(input)
		}
	}
}
