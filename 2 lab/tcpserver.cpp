#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h> 
#include <winsock2.h> 
#include <iostream>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

char ip_port_info[21];
int numEvents = 0;
int cs[100];
int sock;


// функция, которая считывает информацию, 
// полученную от клиента
int parse_recv(int s, int here)
{
	FILE* file = fopen("D:\\программирование\\tcpserver\\msg.txt", "a");
	char buffer[1040] = { 0 };
	char msg[350080] = { 0 };
	int ret;
	char put[4];
	if (here == 0) {
		ret = recv(s, put, 3, 0);
		put[ret] = '\0';

		// если получаем сообщение put, начинаем парсить клиентский файл
		if (strcmp(put, "put") == 0)
			printf("get PUT\n");
		else
			return 2;
	}

	int num = recv(s, buffer, 15, 0);
	if (num == 0 || num == -1)
		return 2;
	if (num == 15)
	{
		fprintf(file, "%s ", ip_port_info);
		if (buffer[4] < 10)
			fprintf(file, "0");
		fprintf(file, "%d.", buffer[4]);
		if (buffer[5] < 10)
			fprintf(file, "0");
		fprintf(file, "%d.", buffer[5]);
		int year = buffer[7];
		year *= 256;
		year += buffer[6];
		int year_good = ntohs(year);
		fprintf(file, "%d ", year_good);
		if (buffer[8] < 10)
			fprintf(file, "0");
		fprintf(file, "%d.", buffer[8]);
		if (buffer[9] < 10)
			fprintf(file, "0");
		fprintf(file, "%d.", buffer[9]);
		year = buffer[11];
		year *= 256;
		year += buffer[10];
		year_good = ntohs(year);
		fprintf(file, "%d ", year_good);
		if (buffer[12] < 10)
			fprintf(file, "0");
		printf("\n %d \n", buffer[12]);
		fprintf(file, "%d:", buffer[12]);
		if (buffer[13] < 10)
			fprintf(file, "0");
		fprintf(file, "%d:", buffer[13]);
		if (buffer[14] < 10)
			fprintf(file, "0");
		fprintf(file, "%d ", buffer[14]);


		printf("()");
		while (true)
		{
			printf("!");
			int rec = recv(s, buffer, 1, 0);
			if (rec == 1)
			{
				strcat(msg, buffer);
				if (buffer[0] != 0)
					fprintf(file, "%c", buffer[0]);
				printf("%c \n", buffer[0]);
			}
			if (buffer[0] == '\0')
				break;

		}

		buffer[0] = '\n';
		fprintf(file, "%c", buffer[0]);
		printf(" ");
		send(s, "ok", 2, 0);
		printf(".");

		if (!strncmp(msg, "stop", 4)) {
			printf("stop\n");
			for (int i = 0; i < numEvents; i++)
				closesocket(cs[i]);
			closesocket(sock);
			printf("Client disconnected\n");
			WSACleanup();
			return 0;
		}
		return 1;
	}
	else
		printf("%d", num);
	return 2;
}


int sock_err(const char* function, int s)
{
	fprintf(stderr, "%s: socket error: %d\n", function, WSAGetLastError());
	return -1;
}


int main(int argc, char* argv[])
{
	int port = atoi(argv[1]);
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2, 2), &wsa_data);
	struct sockaddr_in addr;
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0)
		return sock_err("socket", sock);

	// ставим неблокирующий режим
	unsigned long mode = 1;
	ioctlsocket(sock, FIONBIO, &mode);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[1]));
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		return sock_err("bind", sock);
	if (listen(sock, 10) < 0)
		return sock_err("listen", sock);


	fd_set rfd;
	FD_ZERO(&rfd);
	FD_SET(sock, &rfd);
	int i;
	int addrlen;
	int maxfd = sock; // максимальный дескриптор сокета
	int ready;

	// инициализация массива сокетов
	for (i = 0; i < 100; i++)
		cs[i] = -1;

	int fail = 1;
	while (fail) {
		ready = select(maxfd + 1, &rfd, NULL, NULL, NULL); // выбор клиента функцией select
		if (ready > 0) {

			// есть события
			if (FD_ISSET(sock, &rfd)) {
				// Есть события на прослушивающем сокете, можно вызвать accept, принять 
				// подключение и добавить сокет подключившегося клиента в массив cs
				addrlen = sizeof(addr);
				int cli = accept(sock, (struct sockaddr*)&addr, &addrlen);
				if (cli < 0)
					sock_err("accept", cli);
				for (i = 0; i < 100; i++)
					if (cs[i] < 0) {
						cs[i] = cli;
						numEvents++;
						break;
					}
				FD_SET(cli, &rfd);
				if (cli > maxfd)
					maxfd = cli;

				// вывод информации о клиенте
				int ip = ntohl(addr.sin_addr.s_addr);
				sprintf(ip_port_info, "%u.%u.%u.%u:", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, (ip) & 0xFF);
				char portStr[5] = { 0 };
				_itoa(port, portStr, 10);
				strcat(ip_port_info, portStr);
				strcat(ip_port_info, "\0");
				
			}

			for (i = 0; i < numEvents; i++) {
				if (FD_ISSET(cs[i], &rfd)) {
					// Сокет cs[i] доступен для чтения.
					// Можно считать информацию, полученную от клиента,
					// и записать ее в файл
					int here = -1;
					while (fail > 0 && fail < 3)
					{
						here++;
						printf("(");
						fail = parse_recv(cs[i], here);
						printf(")");
					}
				}
			}
		}
		else {
			// Произошел таймаут
			if (errno == EINTR)
				continue;
		}
	}
	return 0;
}