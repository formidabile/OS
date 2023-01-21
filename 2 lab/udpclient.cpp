#define _CRT_SECURE_NO_WARNINGS

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h> 
#include <winsock2.h> 
#include <conio.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib") 


#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <time.h> 
#include <malloc.h>


char message[500][1000000] = { 0 };
int len[500];
int is_sent[500] = { 0 };
int is_deliv[500] = { 0 };
int count = 0;


int init()
{
	WSADATA wsa_data;
	return (0 == WSAStartup(MAKEWORD(2, 2), &wsa_data));
}

int sock_err(const char* function, int s)
{
	fprintf(stderr, "%s: socket error: %d\n", function, errno);
	return -1;
}
void send_request(int s, struct sockaddr_in* addr, char serv_mess[], int length)
{
	int res = sendto(s, serv_mess, length, 0, (struct sockaddr*)addr, sizeof(struct sockaddr_in));
	if (res <= 0)
		sock_err("sendto", s);
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

		int c;
		for (c = 31; str[c]; c++)
			message[num][c - 16] = str[c];
		
		message[num][c - 16] = '\0';
		len[num] = c - 15;
	}

	fclose(f);

	return num;
}


int main(int argc, char** argv)
{
	char* ip = strtok(argv[1], ":");
	int port = atoi(strtok(NULL, " "));
	struct sockaddr_in addr;
	unsigned char num[801] = { 0 };
	init();
	int s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
	{
		int i = sock_err("socket", s);
		return i;
	}
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = get_host_ipn(ip);
	struct timeval tv = { 0, 100000 };
	int res;
	count = parse_file(argv[2]);
	int deliv = 0;


	while (true)
	{
		printf("%d ", deliv);
		if (deliv == count + 1 || deliv == 20)
			break;
		for (int i = 0; i < 20 && i < count + 1; i++)
		{
			if (is_deliv[i] == 0)
				sendto(s, (char*)message[i], len[i], 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
			memset(num, 0, 800);
			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(s, &fds);
			res = select(s + 1, &fds, 0, 0, &tv);
			if (res > 0)
			{
				struct sockaddr_in addr;
				unsigned int addrlen = sizeof(addr);
				int received = 0;
				received = recvfrom(s, (char*)num, 800, 0, (struct sockaddr*)&addr, (int*)&addrlen);
				num[received] = '\0';
				printf("%d!", received);
				if (received <= 0)
					continue;
				for (int h = 0; h <= received - 4; h += 4)
				{
					int k = num[h + 3] * 256 * 256 * 256 + num[h + 2] * 256 * 256 + num[h + 1] * 256 + num[h];
					printf("(%d)", k);
					if (is_deliv[k] == 0)
					{
						deliv++;
						is_deliv[k] = 1;
					}
				}
			}
		}
		Sleep(1000);
	}
	closesocket(s);
	return 0;
}
