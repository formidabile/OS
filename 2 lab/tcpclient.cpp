#define _CRT_SECURE_NO_WARNINGS

#include <sys/types.h> 
#include <sys/socket.h> 
#include <netdb.h> 
#include <errno.h> 
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>


#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <time.h> 
#include <malloc.h>


char message[500][1000000] = { 0 };
int len[500];


int sock_err(const char* function, int s)
{
	int err;
	err = errno;
	fprintf(stderr, "%s: socket error: %d\n", function, err);
	return -1;
}

unsigned int get_host_ipn(const char* name)
{
	struct addrinfo* addr = 0;
	unsigned int ip4addr = 0;

	if (0 == getaddrinfo(name, 0, 0, &addr))
	{
		struct addrinfo* cur = addr;
		while (cur)
		{
			if (cur->ai_family == AF_INET)
			{
				ip4addr = ((struct sockaddr_in*)cur->ai_addr)->sin_addr.s_addr;
				break;
			}
			cur = cur->ai_next;
		}
		freeaddrinfo(addr);
	}
	return ip4addr;
}

int parse_file(char* fname)
{
	FILE* f = fopen(fname, "r");
	char str[350000] = { 0 };
	int num;
	for (num = 0; fgets(str, 350000, f); num++)
	{
		
		int num1 = htonl(num);
		message[num][0] = num1 >> 24;
		message[num][1] = (num1 >> 16) & 0xFF;
		message[num][2] = (num1 >> 8) & 0xFF;
		message[num][3] = num1 & 0xFF;
		message[num][4] = (int)(str[0] - '0') * 10 + (int)str[1] - '0';
		message[num][5] = (int)(str[3] - '0') * 10 + (int)str[4] - '0';
		unsigned short year = (int)(str[6] - '0') * 1000 + (int)(str[7] - '0') * 100 + (int)(str[8] - '0') * 10 + (int)str[9] - '0';
		year = htons(year);
		message[num][7] = (year >> 8) & 0xff;
		message[num][6] = year & 0xff;


		message[num][8] = (int)(str[11] - '0') * 10 + (int)str[12] - '0';
		message[num][9] = (int)(str[14] - '0') * 10 + (int)str[15] - '0';
		year = (int)(str[17] - '0') * 1000 + (int)(str[18] - '0') * 100 + (int)(str[19] - '0') * 10 + (int)str[20] - '0';
		year = htons(year);
		message[num][11] = (year >> 8) & 0xff;
		message[num][10] = year & 0xff;

		message[num][12] = (int)(str[22] - '0') * 10 + (int)str[23] - '0';
		message[num][13] = (int)(str[25] - '0') * 10 + (int)str[26] - '0';
		message[num][14] = (int)(str[28] - '0') * 10 + (int)str[29] - '0';

		int count;
		for (count = 31; str[count]; count++)
			message[num][count - 16] = str[count];

		message[num][count - 16] = '\0';
		len[num] = count - 15;
	}
	fclose(f);
	return num;
}


int main(int argc, char** argv)
{
	char* ip = strtok(argv[1], ":");
	int port = atoi(strtok(NULL, " "));
	struct sockaddr_in addr;
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
	{
		int i = sock_err("socket", s);
		return i;
	}
	printf("Socket created successfully.\n");
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = get_host_ipn(ip);


	for (int j = 0; j < 10; j++)
	{
		int con = connect(s, (struct sockaddr*)&addr, sizeof(addr));
		if (con < 0)
		{
			printf("Waiting\n");
			if (j == 9) {
				printf("Connection is unavailable...\n");
				return -1;
			}
		}
		else
		{
			printf("Connect\n");
			if (send(s, "put", 3, 0) == -1)
				return -1;
			else
				break;
		}
		

		sleep(10);
	}

	int string_number = parse_file(argv[2]);
	char ok[2] = {'o', 'k'};
	for (int i = 0; i <= string_number; i++)
	{
		send(s, message[i], len[i], 0);
		recv(s, ok, 2, 0);
	}
	return 0;
}
