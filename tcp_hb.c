/***************************************************************************

gcc tcp_hb.c tcp_client.c util.c b64.c tcpsink.c -o client -I./jsonx86/out/include/json/  -L./jsonx86/out/lib -ljson
export LD_LIBRARY_PATH=./jsonx86/out/lib/:$LD_LIBRARY_PATH
{  \"Etype\": \"1\",  \"Edata\": {    \"Jsonrpc\": \"\",    \"Id\": \"1\",    \"Params\": \"/sbin/reboot\"  }}"  
 
****************************************************************************/
#include "etcp.h"
#include "heartbeat.h"
#include "json.h"
#include "b64.h"
#include <net/if.h> 
#include <sys/socket.h>
#include <sys/ioctl.h>

static char *fc_script = "/usr/sbin/freecwmp";
static char *fc_script_set_actions = "/tmp/freecwmp_set_action_values.sh";
#define HOMEPWD "./etc/config/"
#define JSPWD  "/usr/lib/js/"
//#define JSPWD "./js/"
#define ErrorJson "{\"name\": \"errorResponse\",\"version\": \"1.0.0\",\"serialnumber\": \"112233445566\",\"error\": \"1\"}"
#define FileJson "{\"name\": \"getResponse\",\"version\": \"1.0.0\",\"serialnumber\": \"%s\",\
				\"keyname\": \"file\",\"packet\": {\"path\": \"/etc/config/\",\"filename\": \"%s\",\"data\": \"%s\"}}"
#define ConfigJson "{\"name\": \"getResponse\",\"version\": \"1.0.0\",\"serialnumber\": \"%s\",\
				\"keyname\": \"config\",\"packet\": {\"data\": \"%s\"}}"
#define CommandJson "{\"name\": \"getResponse\",\"version\": \"1.0.0\",\"serialnumber\": \"%s\",\
				\"keyname\": \"command\",\"packet\": {\"data\": \"%s\"}}"
#define SetResponse "{\"name\": \"setResponse\",\"version\": \"1.0.0\",\"serialnumber\": \"%s\",\"keyname\": \"config\",\
					\"packet\": {\"data\": \"%s\"}}"
#define FREE(x)   \
	do            \
	{             \
		free(x);  \
		x = NULL; \
	} while (0);
char deviceMac[13];

int get_mac(char *eth0)    //·µ»ØÖµÊÇÊµ¼ÊÐ´Èëchar * macµÄ×Ö·û¸öÊý£¨²»°üÀ¨'\0'£©
{
    struct ifreq ifreq;
    int sock;
    printf("jiangyibo mac get\n");
    if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
		snprintf (deviceMac, 13, "112233445566");
        return -1;
    }
    strcpy (ifreq.ifr_name, eth0);    //Currently, only get eth0

    if (ioctl (sock, SIOCGIFHWADDR, &ifreq) < 0)
    {
        snprintf (deviceMac, 13, "112233445566");
        return -1;
	}
	snprintf (deviceMac, 13, "112233445566");
    printf("jiangyibo mac get ok\n");
	return snprintf (deviceMac, 13, "%02X%02X%02X%02X%02X%02X", (unsigned char) ifreq.ifr_hwaddr.sa_data[0], (unsigned char) ifreq.ifr_hwaddr.sa_data[1],\
			  (unsigned char) ifreq.ifr_hwaddr.sa_data[2], (unsigned char) ifreq.ifr_hwaddr.sa_data[3], (unsigned char) ifreq.ifr_hwaddr.sa_data[4], \
			  (unsigned char) ifreq.ifr_hwaddr.sa_data[5]);
}
int external_get_action(char *action, char *name, char **value)
{
	//lfc_log_message(NAME, L_NOTICE, "executing get %s '%s'\n",
	//		action, name);
	int pid;
	int pfds[2];
	char *c = NULL;

	if (pipe(pfds) < 0)
		return -1;

	if ((pid = fork()) == -1)
		goto error;

	if (pid == 0)
	{
		/* child */

		const char *argv[8];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = "/root/run.sh";
		argv[i++] = "--newline";
		argv[i++] = "--value";
		argv[i++] = "get";
		argv[i++] = action;
		argv[i++] = name;
		argv[i++] = NULL;

		close(pfds[0]);
		dup2(pfds[1], 1);
		close(pfds[1]);
		execvp(argv[0], (char **)argv);
		exit(ESRCH);
	}
	else if (pid < 0)
		goto error;

	/* parent */
	close(pfds[1]);

	int status;
	while (wait(&status) != pid)
	{
		printf("waiting for child to exit");
	}

	char buffer[64];
	ssize_t rxed;
	int t;

	*value = NULL;
	while ((rxed = read(pfds[0], buffer, sizeof(buffer))) > 0)
	{

		if (*value)
			t = asprintf(&c, "%s%.*s", *value, (int)rxed, buffer);
		else
			t = asprintf(&c, "%.*s", (int)rxed, buffer);

		if (t == -1)
			goto error;

		free(*value);
		*value = strdup(c);
		free(c);
	}

	if (!(*value))
	{
		goto done;
	}

	if (!strlen(*value))
	{
		FREE(*value);
		goto done;
	}

	if (rxed < 0)
		goto error;

done:
	close(pfds[0]);
	return 0;

error:
	free(c);
	FREE(*value);
	close(pfds[0]);
	return -1;
}

int external_set_action_write(char *action, char *name, char *value)
{

	FILE *fp;

	if (access(fc_script_set_actions, R_OK | W_OK | X_OK) != -1)
	{
		fp = fopen(fc_script_set_actions, "a");
		if (!fp)
			return -1;
	}
	else
	{
		fp = fopen(fc_script_set_actions, "w");
		if (!fp)
			return -1;

		fprintf(fp, "#!/bin/sh\n");

		if (chmod(fc_script_set_actions,
				  strtol("0700", 0, 8)) < 0)
		{
			return -1;
		}
	}

	fprintf(fp, "/bin/sh %s set %s %s '%s'\n", fc_script, action, name, value);

	fclose(fp);

	return 0;
}

int external_set_action_execute()
{
	int pid = 0;
	if ((pid = fork()) == -1)
	{
		return -1;
	}

	if (pid == 0)
	{
		/* child */

		const char *argv[3];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = fc_script_set_actions;
		argv[i++] = NULL;

		execvp(argv[0], (char **)argv);
		exit(ESRCH);
	}
	else if (pid < 0)
		return -1;

	/* parent */
	int status;
	while (wait(&status) != pid)
	{
		printf("waiting for child to exit");
	}

	// TODO: add some kind of checks
/*
	if (remove(fc_script_set_actions) != 0)
		return -1;
*/
	return 0;
}

int external_download(char *url, char *size)
{
	int pid = 0;

	if ((pid = fork()) == -1)
		return -1;

	if (pid == 0)
	{
		/* child */

		const char *argv[8];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = fc_script;
		argv[i++] = "download";
		argv[i++] = "--url";
		argv[i++] = url;
		argv[i++] = "--size";
		argv[i++] = size;
		argv[i++] = NULL;

		execvp(argv[0], (char **)argv);
		exit(ESRCH);
	}
	else if (pid < 0)
		return -1;

	/* parent */
	int status;
	while (wait(&status) != pid)
	{
		printf("waiting for child to exit");
	}

	if (WIFEXITED(status) && !WEXITSTATUS(status))
		return 0;
	else
		return 1;

	return 0;
}

int commandDownload(char *url, char *md5)
{
	return 1;
}

int commandFactoryset()
{
	return 1;
}

int setShellValue(char *value)
{

	char *c = NULL;
	if (NULL == value || '\0' == value[0])
	{
		if (external_get_action("value", "text", &c))
			goto error;
	}
	else
	{
		c = strdup(value);
	}
	if (c)
	{

		FREE(c);
	}
	return 0;
error:
	return -1;
}

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

int jsonGetConfig(json_object *config)
{
	int rc = 0;
	char tempstr[2048];
	enum json_type type;
	json_object *pval = NULL;
	char *tempVal = NULL;
	json_object *obj = config;

	int index = 0;
	memset(tempstr, 0, 2048);
	char *key;
	struct json_object *val;
	struct lh_entry *entry;
	for ( entry = json_object_get_object(obj)->head; entry != NULL;)
	{
		if (entry)
		{
			key = (char *)entry->k;
			val = (struct json_object *)entry->v;
			entry = entry->next;
		}
		else
		{
			printf("mabi\n");
		}
		type = json_object_get_type(pval);
		switch (type)
		{
		case json_type_string:
			tempVal = json_object_get_string(val);
			break;
		default:
			break;
		}
		printf("\t%s:\n", key /*, json_object_to_json_string(val)*/);
		/*		if (index++ == 0)
		{
			sprintf(tempstr, "%s\"%s\":\"%s\"", tempstr, key, tempVal);
		}
		else
		{
			sprintf(tempstr, "%s,\"%s\":\"%s\"", tempstr, key, tempVal);
		}
*/
	}
}

int jsonSetConfig(SOCKET s, json_object *config)
{
	int rc = 0;
	char tempstr[2048];
	char *tempVal = NULL;
	enum json_type type;
	int index = 0;
	json_object *obj = config;
	memset(tempstr, 0, 2048);

	if (config == NULL)
	{
		printf("jyb test %s\n", config);
		return;
	}

	char *key;
	struct json_object *val;
	struct lh_entry *entry = json_object_get_object(obj)->head;
	for (; entry != NULL;)
	{
		printf("ri mabi\n");
		if (entry)
		{
			key = (char *)entry->k;
			val = (struct json_object *)entry->v;
			entry = entry->next;
		}
		else
		{
			printf("mabi\n");
			break;
		}

		printf("jiangyibo sfdsfsa mabi\n");
		type = json_object_get_type(val);
		switch (type)
		{
		case json_type_string:
			tempVal = json_object_get_string(val);
			break;
		default:
			break;
		}
		printf("jyb test %s %s\n", key, tempVal);


		if (index++ == 0)
		{
			sprintf(tempstr, "\"%s\":\"%s\"", key, tempVal);
		}
		else
		{
			sprintf(tempstr, "%s,\"%s\":\"%s\"", tempstr, key, tempVal);
		}

		printf("jiangyibo %s\n", tempstr);
	
		
		if (external_set_action_write("value", key,tempVal ))
		{
			external_set_action_execute();
		}


		if(entry==NULL)
		{
			break;
		}	
	}
    memset(tempstr,0,1024);
	sprintf(tempstr, SetResponse, deviceMac,"setok");

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

char *exeShellnoreturn(char *comm)
{
	FILE *fstream = NULL;

	int errnoT = 0;

	memset(cliBuff, 0, sizeof(cliBuff));

	if (NULL == (fstream = popen(comm, "r")))
	{
		fprintf(stderr, "execute command failed: %s", strerror(errno));
		return "error";
	}
	pclose(fstream);

	return cliBuff;
}

#define SERVER_IP  "113.106.98.92"
#define SERVER_TCP_PORT  "8880"


int main(int argc, char **argv)
{
	fd_set allfd;
	fd_set readfd;
	char *server_ip;
	char *server_port;
	msg_t msg;
	char sendmsg[4096];
	char informRes[1500];
	char tempstr[1500];
	char infomsg[1500];
	char recvmsg[4096];
	struct timeval tv;
	int length;
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
    if(argc >= 3)
	{
		if(argc==4)
		{
		get_mac(argv[3]);
		}else{
			get_mac("eth0");
		}
		server_ip = argv[1];
		server_port = argv[2];
	}else{
		get_mac("eth0");
		server_ip = SERVER_IP;
		server_port = SERVER_TCP_PORT;
	}
	INIT();
begin :
	s = tcp_client(server_ip, server_port);
	if(s<0)
	{
		sleep(10);
		goto begin;
	}
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
		{
			error(0, 0, "select failure");
			goto end;
		}
		if (rc == 0) /* timed out */
		{
			if (++heartbeats > 13)
			{
				error(0, 0, "connection dead\n");
			}
			error(0, 0, "sending heartbeat #%d\n", heartbeats);
			msg.type = htonl(MSG_HEARTBEAT);
			sprintf(sendmsg, infomsg, deviceMac, commandkey, deviceMac, uptime);
			rc = send(s, (char *)sendmsg, strlen(sendmsg), 0);
			if (rc < 0)
			{
				error(0, 0, "send failure");
				goto end;
			}
			tv.tv_sec = T2;
			continue;
		}
		printf("jiangyibo jjj\n");
		if (!FD_ISSET(s, &readfd))
		{
			error(0, 0, "select returned invalid socket\n");
			goto end;
		}
		memset(recvmsg, 0, 2000);
		rc = recv(s, (char *)recvmsg,
				  2000, 0);
		if (rc == 0)
		{
			error(0, 0, "server terminated\n");
			goto end;
		}
		if (rc < 0)
		{
			error(0, 0, "recv failure");
			goto end;
		}
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
					param_p3 = zstream_b64encode(param_p2, &length);
					printf("jiangyibo %s\n", param_p3);
					sprintf(sendmsg, CommandJson, deviceMac, param_p3);
					free(param_p3);
					rc = send(s, (char *)sendmsg, sizeof(sendmsg), 0);
				}
				else if (strcmp(command, "shellexe") == 0)
				{
					memset(sendmsg, 0, 1024);
					p1_obj = GetValByEdata(new_obj, "packet");
					param_p1 = GetValByKey(p1_obj, "shellcmd");
					exeShellnoreturn(param_p1);
					sprintf(sendmsg, CommandJson, deviceMac, "exeShellnoreturn");
					free(param_p3);
					rc = send(s, (char *)sendmsg, sizeof(sendmsg), 0);
				}				
				else if (strcmp(command, "file") == 0)
				{
					memset(sendmsg, 0, 1500);
					p1_obj = GetValByEdata(new_obj, "packet");
					param_p1 = GetValByKey(p1_obj, "shellcmd");
					printf("jyb test %s\n", param_p1);
					if (param_p1 != NULL)
					{
						if (getConfigFile(tempstr, param_p1) != 0)
						{
							length = strlen(tempstr);
							param_p3 = zstream_b64encode(tempstr, &length);

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
				char *c = NULL;

				command = GetValByEtype(new_obj, "keyname");
				printf("jiangyibo eeee 333 %s\n", command);
				if (!strcmp(command, "value"))
				{
					p1_obj = json_object_object_get(new_obj, "packet");
					jsonSetConfig(s, p1_obj);
					printf("jyb test ok\n");
			
				}
				else if (!strcmp(command, "download"))
				{
					p1_obj = GetValByEdata(new_obj, "packet");
					param_p1 = GetValByKey(p1_obj, "url");
					param_p2 = GetValByKey(p1_obj, "size");
					commandDownload(param_p1, param_p2);
				}
				else if (!strcmp(command, "factory"))
				{
					commandFactoryset();
				}
				else if (!strcmp(command, "update"))
				{
					p1_obj = GetValByEdata(new_obj, "packet");
					param_p1 = GetValByKey(p1_obj, "url");
					param_p2 = GetValByKey(p1_obj, "size");
				}
				else
				{
					rc = send(s, ErrorJson, sizeof(ErrorJson), 0);
				}
			}
			else
			{

				rc = send(s, ErrorJson, sizeof(ErrorJson), 0);
				//发送   的json 错误
			}
			json_object_put(new_obj);
		}
		/* process message */
	}
end:
	close(s);
	sleep(10);
   	goto begin;

}
