package main

import "fmt"
import "time"

import "encoding/json"
import "bufio"
import "strings"
import "os"
import "log"
import "github.com/bitly/go-simplejson"
import "encoding/base64"

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

/*
{
    "name": "get",
    "version": "1.0.0",
    "serialnumber": "%s",
    "keyname": "file",
    "packet": {
        "path": "%s",
        "name": "%s",
        "data": "%s"
    }
}
*/

func fazhan_ok(mtype string) {
	//	var zhi = make([]byte, 2048)

	mtype = strings.TrimRight(mtype, "\n")

	type CommandData struct {
		Etype string
		Edata CommandStats
	}

	addrVe := strings.Split(mtype, " ")

	var result CommandStats
	var setcomm SetCommandStats
	var val []byte

	lengthV1 := len(addrVe)

	if len(addrVe) >= 3 {

		v3 := strings.TrimSpace(addrVe[0])

		v1 := strings.TrimSpace(addrVe[1])

		v2 := strings.TrimSpace(addrVe[2])

		addrVe = addrVe[3:]

		if v1 == "get" {

			command := strings.Join(addrVe, " ")
			if v2 == "command" {
				if lengthV1 == 3 {
					return
				}
				result = CommandStats{
					Name:         v1,
					Version:      "1.1.1",
					Serialnumber: "123456",
					Keyname:      "command",
					Packet: PacketStats{
						Shellcmd: command,
						Data:     "ok",
					},
				}
			} else if v2 == "config" {
				result = CommandStats{
					Name:         v1,
					Version:      "1.1.1",
					Serialnumber: "123456",
					Keyname:      "config",
				}

			} else if v2 == "inform" {
				result = CommandStats{
					Name:         v1,
					Version:      "1.1.1",
					Serialnumber: "123456",
					Keyname:      "inform",
				}
			} else if v2 == "file" {
				if lengthV1 == 3 {
					return
				}
				result = CommandStats{
					Name:         v1,
					Version:      "1.1.1",
					Serialnumber: "123456",
					Keyname:      "file",
					Packet: PacketStats{
						Shellcmd: command,
						Data:     "ok",
					},
				}
			} else {
				fmt.Println("error no this command! " + command)
				return
			}

			val, _ = json.Marshal(result)

			fmt.Println("shuchu shuju ", string(val))

			cn, err := kvvalue[v3]
			if !err {

				fmt.Println("error no this mac!")
			} else {
				_, e1 := cn.Write(val)
				if e1 == nil {

				} else {
					cn.Close()
					delete(kvvalue, v1)
					fmt.Println("socket close")
				}
			}
		} else if v1 == "set" {
			command := strings.Join(addrVe, " ")
			setcomm = SetCommandStats{
				Name:         v1,
				Version:      "1.1.1",
				Serialnumber: "123456",
				Keyname:      "value",
				Packet: SetPacketStats{
					Wireless: command,
					Data:     "ok",
				},
			}

			val, _ := json.Marshal(setcomm)

			fmt.Println("shuchu shuju ", string(val))

			cn, err := kvvalue[v3]
			if !err {

				fmt.Println("error no this mac!")
			} else {

				_, e1 := cn.Write(val)
				if e1 == nil {

				} else {
					cn.Close()
					delete(kvvalue, v1)
					fmt.Println("socket close")
				}

			}
		}
	} else {
		fmt.Println("import data error")
	}

}

type PacketStats struct {
	Shellcmd string `json:"shellcmd"`
	Data     string `json:"date"`
}

type SetPacketStats struct {
	Wireless string `json:"wireless"`
	Data     string `json:"date"`
}

type SetCommandStats struct {
	/*	    "name": "get",
	"version": "1.0.0",
	"serialnumber": "%s",
	"keyname": "wireless",
	"packet": {
	    "type": "1"
	}
	*/
	Name         string         `json:"name"`
	Version      string         `json:"version"`
	Serialnumber string         `json:"serialnumber"`
	Keyname      string         `json:"keyname"`
	Packet       SetPacketStats `json:"packet"`
}

type CommandStats struct {
	/*	    "name": "get",
	"version": "1.0.0",
	"serialnumber": "%s",
	"keyname": "wireless",
	"packet": {
	    "type": "1"
	}
	*/
	Name         string      `json:"name"`
	Version      string      `json:"version"`
	Serialnumber string      `json:"serialnumber"`
	Keyname      string      `json:"keyname"`
	Packet       PacketStats `json:"packet"`
}

func mac_type_ok(conTcp net.Conn) {

	type CommandData struct {
		Etype string
		Edata CommandStats
	}
	result := CommandStats{
		Name:         "informResponse",
		Version:      "1.1.1",
		Serialnumber: "123456",
		Keyname:      "ok",
	}

	val, _ := json.Marshal(result)
	for {
		var zhi = make([]byte, 2048)
		var trueMac = "000000000000"
		_, err3 := conTcp.Read(zhi)
		if err3 != nil {
			cn, e1 := kvvalue[trueMac]
			if !e1 {

			} else {
				cn.Close()
				delete(kvvalue, trueMac)
				fmt.Println("socket close")
			}
			return
		}

		//取设备mac 标识字符作为唯一key
		fmt.Println("read : " + string(zhi))

		js, err := simplejson.NewJson(zhi)
		if err != nil {
			fmt.Println("decode json error")

			//return
			//		log.Fatalln(err)
		} else {
			name := js.Get("name").MustString()
			fmt.Println("recv name " + name)
			if name == "inform" {

				trueMac = js.Get("serialnumber").MustString()
				cn, e1 := kvvalue[trueMac]
				if !e1 {
					fmt.Println("jiangyibo insert 1")
					insertKv(trueMac, conTcp)
				} else {
					if cn == conTcp {
						fmt.Println("jiangyibo insert 3")
					} else {
						fmt.Println("jiangyibo insert 2")
						insertKv(trueMac, conTcp)
					}
				}

				if err != nil {
					log.Fatalln(err)
				}

				fmt.Printf("Total:%s\n", trueMac)

				conTcp.Write(val)

			} else if name == "get" {

				conTcp.Write(val)
			} else if name == "getResponse" {
				pack := js.Get("packet")
				encodeString := pack.Get("data").MustString()
				fmt.Println("jyb test:" + encodeString)
				decodeBytes, err := base64.StdEncoding.DecodeString(encodeString)
				if err != nil {
					log.Fatalln(err)
				} else {
					fmt.Println(string(decodeBytes))
				}
			} else if name == "setResponse" {
				pack := js.Get("packet")
				encodeString := pack.Get("data").MustString()
				fmt.Println("jyb test:" + encodeString)
				decodeBytes, err := base64.StdEncoding.DecodeString(encodeString)
				if err != nil {
					//log.Fatalln(err)
					fmt.Println("can't decode-base64")
				} else {
					fmt.Println(string(decodeBytes))
				}
			}
		}
	}

}

func qidongServer() {
	c1, _ := net.ResolveTCPAddr("tcp", ":8880")
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
var file *os.File

func main() {
	file, err = os.Create("test.log")
	if err != nil {
		log.Fatalln("fail to create test.log file!")
	}
	logger := log.New(file, "", log.LstdFlags|log.Llongfile)
	log.Println("1.Println log with log.LstdFlags ...")
	logger.Println("1.Println log with log.LstdFlags ...")

	logger.SetFlags(log.LstdFlags)

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
