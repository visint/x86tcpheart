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
import "database/sql"
import _ "github.com/mattn/go-sqlite3"

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
	if lengthV1 == 1 {
		v3 := strings.TrimSpace(addrVe[0])
		if v3 == "show" {
			showSql()
		} else if v3 == "delete" {
			deleteSql()
		}

	} else if len(addrVe) >= 3 {

		v3 := strings.TrimSpace(addrVe[0])

		v1 := strings.TrimSpace(addrVe[1])

		v2 := strings.TrimSpace(addrVe[2])

		addrVe = addrVe[3:]

		if v3 == "edit" {
			editSql(v1, v2)
		} else {

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
				} else if v2 == "shellexe" {
					if lengthV1 == 3 {
						return
					}
					result = CommandStats{
						Name:         v1,
						Version:      "1.1.1",
						Serialnumber: "123456",
						Keyname:      "shellexe",
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

				logger.Println("shuchu shuju ", string(val))

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

				logger.Println("shuchu shuju ", string(val))

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
	var zhi = make([]byte, 4096)
	var index = 0
	for {
		var zhishou = make([]byte, 9128)
		var trueMac = "000000000000"
		recLength, err3 := conTcp.Read(zhishou)
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
		if recLength == 1440 && index == 0 {
			index = 1
			for mm := 0; mm < 1440; mm++ {
				zhi[mm] = zhishou[mm]
			}
			continue
		}
		if recLength == 1440 && index == 1 {
			index = 2
			for mm := 0; mm < 1440; mm++ {
				zhi[1440+mm] = zhishou[mm]
			}
			continue
		}
		for mm := 0; mm < recLength; mm++ {
			zhi[1440*index+mm] = zhishou[mm]
		}

		//取设备mac 标识字符作为唯一key
		logger.Println("read : ", recLength, " "+string(zhi))

		js, err := simplejson.NewJson(zhi)
		if err != nil {
			logger.Println("decode json error")

			//return
			//		log.Fatalln(err)
		} else {
			name := js.Get("name").MustString()
			logger.Println("recv name " + name)
			if name == "inform" {

				trueMac = js.Get("serialnumber").MustString()
				cn, e1 := kvvalue[trueMac]
				if !e1 {
					fmt.Println("jiangyibo insert ", trueMac)
					insertKv(trueMac, conTcp)
				} else {
					if cn == conTcp {
						//fmt.Println("jiangyibo update ", trueMac)
					} else {
						fmt.Println("jiangyibo update socket ", trueMac)
						insertKv(trueMac, conTcp)
					}
				}
				updateSql(trueMac)

				if err != nil {
					log.Fatalln(err)
				}

				//				fmt.Printf("Total:%s\n", trueMac)

				conTcp.Write(val)

			} else if name == "get" {

				conTcp.Write(val)
			} else if name == "getResponse" {
				pack := js.Get("packet")
				encodeString := pack.Get("data").MustString()
				logger.Println("jyb test:" + encodeString)
				decodeBytes, err := base64.StdEncoding.DecodeString(encodeString)
				if err != nil {
					log.Fatalln(err)
				} else {
					fmt.Println(string(decodeBytes))
					logger.Println(string(decodeBytes))
				}
			} else if name == "setResponse" {
				pack := js.Get("packet")
				encodeString := pack.Get("data").MustString()
				logger.Println("jyb test:" + encodeString)
				decodeBytes, err := base64.StdEncoding.DecodeString(encodeString)
				if err != nil {
					//log.Fatalln(err)
					fmt.Println("can't decode-base64")
				} else {
					logger.Println(string(decodeBytes))
				}
			}
		}
		for mm := 0; mm < 4096; mm++ {
			zhi[mm] = 0
		}
		index = 0
	}

}

func qidongServer() {
	c1, _ := net.ResolveTCPAddr("tcp", ":9095")
	d1, _ := net.ListenTCP("tcp", c1)
	for {
		f1, err := d1.Accept()
		if err == nil {
			fmt.Println("ok!", f1.RemoteAddr().String())
			//			f1.SetDeadline(time.Second * 5)
			go mac_type_ok(f1)
			//insertKv("1", f1)
		}
	}

}

var inputReader *bufio.Reader
var input string
var err error
var file *os.File

var logger *log.Logger

func checkErr(err error) {
	if err != nil {
		panic(err)
	}
}
func findSql(devname string) bool {

	db, err := sql.Open("sqlite3", "tcpReport.db")
	checkErr(err)
	//查询数据
	_, err = db.Query("SELECT * FROM devinfo where devname='00E04C0D9F8E'")
	checkErr(err)
	db.Close()
	return true
}

func updateSql(devname string) {
	db, err := sql.Open("sqlite3", "tcpReport.db")
	checkErr(err)

	rows, err := db.Query("SELECT * FROM devinfo where devname=" + "'" + devname + "'")
	checkErr(err)
	var index int
	index = 0
	/*	if rows.Next() {
			index = 1
		}else{
			index = 0
		}
	*/

	for rows.Next() {
		index = 1
	}

	if index > 0 {
		//更新数据
		stmt, err := db.Prepare("update devinfo set state=1 where devname=?")
		checkErr(err)

		res, err := stmt.Exec(devname)
		checkErr(err)

		_, err = res.RowsAffected()
		checkErr(err)

		//		fmt.Println(affect)

	} else {

		//插入数据
		stmt, err := db.Prepare("INSERT INTO devinfo(devname,state, departname, created) values(?,?,?,?)")
		checkErr(err)

		res, err := stmt.Exec(devname, 1, "派联", "2012-12-09")
		checkErr(err)

		_, err = res.LastInsertId()
		checkErr(err)

		//		fmt.Println(id)
	}

	db.Close()
}

func editSql(uid string, departname string) {
	db, err := sql.Open("sqlite3", "tcpReport.db")
	checkErr(err)

	//更新数据
	stmt, err := db.Prepare("update devinfo set departname=? where uid=?")
	checkErr(err)

	res, err := stmt.Exec(departname, uid)
	checkErr(err)

	affect, err := res.RowsAffected()
	checkErr(err)

	fmt.Println(affect)
	db.Close()
}

func deleteSql() {
	db, err := sql.Open("sqlite3", "tcpReport.db")
	checkErr(err)

	stmt, err := db.Prepare("delete from devinfo")
	checkErr(err)

	res, err := stmt.Exec()
	checkErr(err)

	affect, err := res.RowsAffected()
	checkErr(err)

	fmt.Println(affect)
	db.Close()
}

func showSql() {
	db, err := sql.Open("sqlite3", "tcpReport.db")
	checkErr(err)
	//查询数据
	rows, err := db.Query("SELECT uid,devname,state,departname FROM devinfo")
	checkErr(err)

	for rows.Next() {
		var uid int
		var devname string
		var department string
		var state int
		//       var created string
		err = rows.Scan(&uid, &devname, &state, &department)
		checkErr(err)
		fmt.Println(uid, " "+devname, " "+department+" ", state)
		//   fmt.Println(created)
	}

	db.Close()

	//删除数据
	/*
	 stmt, err = db.Prepare("delete from devinfo where uid=?")
	 checkErr(err)

	 res, err = stmt.Exec(2)
	 checkErr(err)

	 affect, err := res.RowsAffected()
	 checkErr(err)

	 fmt.Println(affect)
	*/
}

func main() {
	db, err := sql.Open("sqlite3", "tcpReport.db")
	checkErr(err)

	db.Exec(`CREATE TABLE  IF NOT EXISTS 'devinfo'(
		'uid' INTEGER PRIMARY KEY AUTOINCREMENT,
		'devname' VARCHAR(64) NULL,
		'state' INTEGER NULL,
		'departname' VARCHAR(64) NULL,
		'created' DATE NULL
	)`)
	fmt.Println("jiangyibo ok")

	//更新数据
	stmt, err := db.Prepare("update devinfo set state=0")
	checkErr(err)

	res, err := stmt.Exec()
	checkErr(err)

	affect, err := res.RowsAffected()
	checkErr(err)

	fmt.Println("update all data state to 0 总数: ", affect)
	db.Close()
	//	updateSql("jiangyibo")
	//	showSql()

	file, err = os.Create("test.log")
	if err != nil {
		log.Fatalln("fail to create test.log file!")
	}
	logger = log.New(file, "", log.LstdFlags|log.Llongfile)
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
