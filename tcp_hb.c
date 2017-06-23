/***************************************************************************

gcc tcp_hb.c tcp_client.c util.c b64.c -o client -I./jsonx86/out/include/json/  -L./jsonx86/out/lib -ljson
export LD_LIBRARY_PATH=./jsonx86/out/lib/:$LD_LIBRARY_PATH
{  \"Etype\": \"1\",  \"Edata\": {    \"Jsonrpc\": \"\",    \"Id\": \"1\",    \"Params\": \"/sbin/reboot\"  }}"  
 
****************************************************************************/
#include "etcp.h"
#include "heartbeat.h"
#include "json.h"
#include "b64.h"

#define HOMEPWD "./etc/config/"
//#define JSPWD  "/usr/lib/js/"
#define JSPWD "./js/"
#define ErrorJson "{\"name\": \"errorResponse\",\"version\": \"1.0.0\",\"serialnumber\": \"112233445566\",\"error\": \"1\"}"
#define FileJson "{\"name\": \"getResponse\",\"version\": \"1.0.0\",\"serialnumber\": \"%s\",\
				\"keyname\": \"file\",\"packet\": {\"path\": \"/etc/config/\",\"filename\": \"%s\",\"data\": \"%s\"}}"
#define ConfigJson "{\"name\": \"getResponse\",\"version\": \"1.0.0\",\"serialnumber\": \"%s\",\
				\"keyname\": \"config\",\"packet\": {\"data\": \"%s\"}}"
#define CommandJson "{\"name\": \"getResponse\",\"version\": \"1.0.0\",\"serialnumber\": \"%s\",\
				\"keyname\": \"command\",\"packet\": {\"data\": \"%s\"}}"
#define SetResponse "{\"name\": \"setResponse\",\"version\": \"1.0.0\",\"serialnumber\": \"%s\",\"keyname\": \"config\",\
					\"packet\": { \"type\": \"ok\" }}"

char deviceMac[] = "112233445566";

char *GetValByEtype(json_object *jobj, const char *sname)
{
	json_object *pval = NULL;
	enum json_type type;
	pval = json_object_object_get(jobj, sname);
	if (NULL != pval)
	{
		type = json_object_get_type(pval);
		switch (type)
		{
		case json_type_string:
			return json_object_get_string(pval);

		default:
			return NULL;
		}
	}
	return NULL;
}

json_object *GetValByEdata(json_object *jobj, const char *sname)
{
	json_object *pval = NULL;
	enum json_type type;
	pval = json_object_object_get(jobj, sname);
	if (NULL != pval)
	{
		type = json_object_get_type(pval);
		switch (type)
		{
		case json_type_object:
			return pval;

		case json_type_array:
			return pval;
		default:
			return NULL;
		}
	}
	return NULL;
}

char *GetValByKey(json_object *jobj, const char *sname)
{
	json_object *pval = NULL;
	enum json_type type;
	pval = json_object_object_get(jobj, sname);
	if (NULL != pval)
	{
		type = json_object_get_type(pval);
		switch (type)
		{
		case json_type_string:
			return json_object_get_string(pval);

		case json_type_object:
			return json_object_to_json_string(pval);

		default:
			return NULL;
		}
	}
	return NULL;
}
int getConfigFile(char *msg, char *filename)
{
	char temp[64];
	sprintf(temp, "%s%s", HOMEPWD, filename);
	FILE *pFile = fopen(temp, "r"); //

	if (pFile == NULL)
	{
		return 0;
	}

	fseek(pFile, 0, SEEK_END); //把指针移动到文件的结尾 ，获取文件长度
	int len = ftell(pFile);	//获取文件长度

	rewind(pFile);			   //把指针移动到文件开头 因为我们一开始把指针移动到结尾，如果不移动回来 会出错
	fread(msg, 1, len, pFile); //读文件
	msg[len] = 0;			   //把读到的文件最后一位 写为0 要不然系统会一直寻找到0后才结束

	fclose(pFile); // 关闭文件
	return len;
}

void getFileData(char *msg, char *filename)
{
	char temp[64];
	sprintf(temp, "%s%s", JSPWD, filename);
	FILE *pFile = fopen(temp, "r"); //获取文件的指针

	fseek(pFile, 0, SEEK_END); //把指针移动到文件的结尾 ，获取文件长度
	int len = ftell(pFile);	//获取文件长度

	rewind(pFile);			   //把指针移动到文件开头 因为我们一开始把指针移动到结尾，如果不移动回来 会出错
	fread(msg, 1, len, pFile); //读文件
	msg[len] = 0;			   //把读到的文件最后一位 写为0 要不然系统会一直寻找到0后才结束

	fclose(pFile); // 关闭文件
}

int jsonSetConfig(SOCKET s, json_object *config)
{
	int rc = 0;
	char tempstr[1024];

	json_object *pval = NULL;
	enum json_type type;
	json_object_object_foreach(config, key, val)
	{

		printf("\t%s:\n", key/*, json_object_to_json_string(val)*/);
		if (!strcmp(key, "ping_result"))
		{
			sprintf(tempstr, SetResponse, deviceMac);
			rc = send(s, tempstr, sizeof(tempstr), 0);
			return rc;
		}
	}

	sprintf(tempstr, SetResponse, deviceMac);

	rc = send(s, tempstr, sizeof(tempstr), 0);

	return rc;
}

char cliBuff[4096];

char *exeShell(char *comm)
{
	FILE *fstream = NULL;

	int errnoT = 0;

	memset(cliBuff, 0, sizeof(cliBuff));

	if (NULL == (fstream = popen(comm, "r")))
	{
		fprintf(stderr, "execute command failed: %s", strerror(errno));
		return "error";
	}
	if (NULL != fread(cliBuff, 1, sizeof(cliBuff), fstream))
	{
		printf("exeShell zhi\n");
	}
	else
	{
		pclose(fstream);
		return "error";
	}

	pclose(fstream);

	return cliBuff;
}



int main(int argc, char **argv)
{
	fd_set allfd;
	fd_set readfd;
	msg_t msg;
	char sendmsg[1500];
	char informRes[1500];
	char tempstr[1500];
	char infomsg[1500];
	char recvmsg[1500];
	struct timeval tv;
	int  length;
	SOCKET s;
	int rc;
	int heartbeats = 0;
	int cnt = sizeof(msg);
	int commandkey = 0;
	int uptime = 0;
	int workmode = 0;
	json_object *pobj, *p1_obj, *p2_obj, *p3_obj = NULL;

	char *param_p1, *param_p2, *param_p3, *param_p4, *param_p5 = NULL;

	int param_int;

	char *typeE, *name, *command;

	char *dataE;

	int typeInt;

	int datalength;

	json_object *new_obj;

	int i;

	getFileData(infomsg, "inform.json");
	getFileData(informRes, "informResponse.json");
//	getFileData(tempstr, "recv.json");

#if 1

	INIT();
	s = tcp_client(argv[1], argv[2]);
	FD_ZERO(&allfd);
	FD_SET(s, &allfd);
	tv.tv_sec = T1;
	tv.tv_usec = 0;
	sprintf(sendmsg, infomsg, deviceMac, commandkey, deviceMac, uptime);
	rc = send(s, (char *)sendmsg, sizeof(sendmsg), 0);

	for (;;)
	{
		readfd = allfd;
		rc = select(s + 1, &readfd, NULL, NULL, &tv);
		if (rc < 0)
			error(1, errno, "select failure");
		if (rc == 0) /* timed out */
		{
			if (++heartbeats > 13)
				error(1, 0, "connection dead\n");
			error(0, 0, "sending heartbeat #%d\n", heartbeats);
			msg.type = htonl(MSG_HEARTBEAT);
			sprintf(sendmsg, infomsg, deviceMac, commandkey, deviceMac, uptime);
			rc = send(s, (char *)sendmsg, strlen(sendmsg), 0);
			if (rc < 0)
				error(1, errno, "send failure");
			tv.tv_sec = T2;
			continue;
		}
		printf("jiangyibo jjj\n");
		if (!FD_ISSET(s, &readfd))
			error(1, 0, "select returned invalid socket\n");
		memset(recvmsg,0,2000);
		rc = recv(s, (char *)recvmsg,
				  2000, 0);
		if (rc == 0)
			error(1, 0, "server terminated\n");
		if (rc < 0)
			error(1, errno, "recv failure");
		heartbeats = 0;
		tv.tv_sec = T1;

		if (rc > 2000)
			continue;
		printf("jiangyibo 888%s\n", recvmsg);
		new_obj = json_tokener_parse(recvmsg);
		
		if (is_error(new_obj))
		{
			// rc = send(s, ErrorJson, sizeof(ErrorJson), 0);
		}
		else
		{
			name = GetValByEtype(new_obj, "name");

			//typeE = GetValByEtype(new_obj, "params");
			printf("jiangyibo name %s\n", name);
			if (name == NULL)
			{
				rc = send(s, ErrorJson, sizeof(ErrorJson), 0);
				//发送  的json 错误
			}
			else if (!strcmp(name, "informResponse"))
			{

				//发送  的定时上报报文
			}
			else if (!strcmp(name, "get"))
			{
				command = GetValByEtype(new_obj, "keyname");
				if (command == NULL)
				{
				}
				else if (strcmp(command, "wireless") == 0)
				{
					/*
			p1_obj = GetValByEdata(new_obj, "params");
			param_p1 = GetValByKey(p1_obj, "type");
			printf("jiangyibo jjj %s\n", param_p1);
			*/
				}
				else if (strcmp(command, "config") == 0)
				{
					memset(sendmsg, 0, 1024);
					getFileData(tempstr, "config.json");
					sprintf(sendmsg, ConfigJson, deviceMac, tempstr);
					rc = send(s, (char *)sendmsg, sizeof(sendmsg), 0);
				}
				else if (strcmp(command, "inform") == 0)
				{
					memset(sendmsg, 0, 1500);
					sprintf(sendmsg, informRes, deviceMac, "informResponse", deviceMac, uptime);
					rc = send(s, (char *)sendmsg, sizeof(sendmsg), 0);
				}
				else if (strcmp(command, "command") == 0)
				{
					memset(sendmsg, 0, 1024);
					p1_obj = GetValByEdata(new_obj, "packet");
					param_p1 = GetValByKey(p1_obj, "shellcmd");
					param_p2 = exeShell(param_p1);
					length = strlen(param_p2);
					param_p3 = zstream_b64encode(param_p2,&length);
					printf("jiangyibo %s\n",param_p3);
					sprintf(sendmsg, CommandJson, deviceMac, param_p3);
					free(param_p3);
					rc = send(s, (char *)sendmsg, sizeof(sendmsg), 0);
				}
				else if (strcmp(command, "file") == 0)
				{
					memset(sendmsg, 0, 1500);
					p1_obj = GetValByEdata(new_obj, "packet");
					param_p1 = GetValByKey(p1_obj, "shellcmd");
					printf("jyb test %s\n",param_p1);
					if (param_p1 != NULL)
					{
						if (getConfigFile(tempstr, param_p1) != 0)
						{
							length = strlen(tempstr);
					        param_p3 = zstream_b64encode(tempstr,&length);

							memset(sendmsg, 0, 1500);
							sprintf(sendmsg, FileJson, deviceMac, param_p1, param_p3);
							free(param_p3);
							rc = send(s, (char *)sendmsg, sizeof(sendmsg), 0);
						}
						else
						{
							memset(sendmsg, 0, 1500);
							rc = send(s, ErrorJson, sizeof(ErrorJson), 0);
						}
					}
				}
				else
				{
					 memset(sendmsg, 0, 1500);
					rc = send(s, ErrorJson, sizeof(ErrorJson), 0);
				}
			}
			else if (!strcmp(name, "set"))
			{
				p1_obj = GetValByEdata(new_obj, "packet");
				param_p1 = GetValByKey(p1_obj, "keyname");
				if (p1_obj != NULL)
				{
					jsonSetConfig(s, p1_obj);
				}
				rc = send(s, ErrorJson, sizeof(ErrorJson), 0);
			}
			else
			{
				memset(sendmsg, 0, 1500);
				rc = send(s, ErrorJson, sizeof(ErrorJson), 0);
				//发送   的json 错误
			}
			json_object_put(new_obj);
		}
		/* process message */
	}
#endif
}
