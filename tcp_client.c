#include "etcp.h"

/* tcp_client - set up for a TCP client */
SOCKET tcp_client(char *hname, char *sname)
{
	struct sockaddr_in peer;
	SOCKET s;

	set_address(hname, sname, &peer, "tcp");
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (!isvalidsock(s))
	{
		error(0, 0, "socket call failed");
		return -1;
	}
	if (connect(s, (struct sockaddr *)&peer, sizeof(peer)))
        {
		printf("connect failed %d\n",errno);
		close(s);
                // error(0, errno, "connect failed");
                return -1;
        }

	return s;
}
SOCKET tcp_connect(SOCKET s)
{
        struct sockaddr_in peer;


        if (connect(s, (struct sockaddr *)&peer, sizeof(peer)))
        {
                printf("connect failed %d\n",errno);
                // error(0, errno, "connect failed");
                return -1;
        }
        return s;
}

