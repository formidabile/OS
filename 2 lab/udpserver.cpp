
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <unistd.h>


#define N 500


int sock_err(const char* function, int s)
{
	int err = errno;
	fprintf(stderr, "%s: socket error: %d\n", function, err);
	return -1;
}



int set_non_block_mode(int s)
{
	int fl = fcntl(s, F_GETFL, 0);
	return fcntl(s, F_SETFL, fl | O_NONBLOCK);
}


int flags = MSG_NOSIGNAL;


struct cLients_base
{
	int port;
	unsigned int IP;
	int messages_now;
	int deliv_mes[20];
};

int port_1, port_2;
int* S;
int* portS;
int cli_num = 0;
sockaddr_in* Sockets_Add;
struct cLients_base* clients;
char answer[81] = { 0 };

int parse_recv(char* buf, int number_client, unsigned int ip, int port)
{
	memset(answer, 0, 80);
	FILE* file = fopen("msg.txt", "a");
	char msg[350080] = { 0 };
	int messages = clients[number_client].messages_now;
	bool is_exist = false;
	int num_msg = ((buf[3] * 256 + buf[2]) * 256 + buf[1]) * 256 + buf[0];
	num_msg = ntohl(num_msg);
	for (int i = 0; i < messages; i++)
	{
		if (clients[number_client].deliv_mes[i] == num_msg)
			is_exist = true;
	}
	if (!is_exist)
	{
		clients[number_client].deliv_mes[messages] = num_msg;
		clients[number_client].messages_now++;
	}
	fprintf(file, "%u", ((ip >> 24) & 0xFF));
	fputc('.', file);
	fprintf(file, "%u", ((ip >> 16) & 0xFF));
	fputc('.', file);
	fprintf(file, "%u", ((ip >> 8) & 0xFF));
	fputc('.', file);
	fprintf(file, "%u", ((ip) & 0xFF));
	fputc(':', file);
	fprintf(file, "%d", port);
	fputc(' ', file);
	if (buf[4] < 10)
		fprintf(file, "0");
	fprintf(file, "%d.", buf[4]);
	if (buf[5] < 10)
		fprintf(file, "0");
	fprintf(file, "%d.", buf[5]);
	int year = buf[7];
	year *= 256;
	year += buf[6];
	int year_good = ntohs(year);
	fprintf(file, "%d ", year_good);
	if (buf[8] < 10)
		fprintf(file, "0");
	fprintf(file, "%d.", buf[8]);
	if (buf[9] < 10)
		fprintf(file, "0");
	fprintf(file, "%d.", buf[9]);
	year = buf[11];
	year *= 256;
	year += buf[10];
	year_good = ntohs(year);
	fprintf(file, "%d ", year_good);
	if (buf[12] < 10)
		fprintf(file, "0");
	fprintf(file, "%d:", buf[12]);
	if (buf[13] < 10)
		fprintf(file, "0");
	fprintf(file, "%d:", buf[13]);
	if (buf[14] < 10)
		fprintf(file, "0");
	fprintf(file, "%d ", buf[14]);
	int i;
	for (i = 15; buf[i]; i++)
	{
		msg[i - 15] = buf[i];
		if (buf[i])
			fprintf(file, "%c", buf[i]);
	}
	buf[i] = '\n';
	fprintf(file, "%c", buf[i]);
	fclose(file);
	unsigned int num;
	for (int j = 0; j < clients[number_client].messages_now; j++)
	{
		num = htonl(clients[number_client].deliv_mes[j]);
		answer[j * 4 + 3] = (num >> 24) & 0xFF;
		answer[j * 4 + 2] = (num >> 16) & 0xFF;
		answer[j * 4 + 1] = (num >> 8) & 0xFF;
		answer[j * 4] = num & 0xFF;
	}
	if (strcmp(msg, "stop") == 0) {
		printf("stop\n");
		return 2;
	}
	return 0;
}

int get_cli_num(unsigned int ip, int port)
{
	for (int i = 0; i < cli_num; i++)
	{
		if (clients[i].IP == ip && clients[i].port == port)
			return i;
	}
	return cli_num;
}

int main(int argc, char** argv)
{
	port_1 = atoi(argv[1]);
	port_2 = atoi(argv[1]);
	int s, i;
	int ports = atoi(argv[2]) - atoi(argv[1]) + 1;
	clients = (struct cLients_base*)malloc(N * sizeof(struct cLients_base));
	S = (int*)malloc(ports * sizeof(int));
	portS = (int*)malloc(ports * sizeof(int));
	Sockets_Add = (sockaddr_in*)malloc(ports * sizeof(struct sockaddr_in));
	struct sockaddr_in addr;
	for (int j = 0; j < ports; j++)
	{
		s = socket(AF_INET, SOCK_DGRAM, 0);
		if (s < 0)
			return sock_err("socket", s);
		set_non_block_mode(s);
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(j + port_1);
		addr.sin_addr.s_addr = htonl(INADDR_ANY);

		if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0)
			return sock_err("bind", s);


		S[j] = s;
		portS[j] = j + port_1;
		Sockets_Add[j] = addr;
	}

	struct pollfd* pfd = (struct pollfd*)malloc(ports * sizeof(struct pollfd));
	if (!pfd)
		return 0;

	for (i = 0; i < ports; i++)
	{
		pfd[i].fd = S[i];
		pfd[i].events = POLLIN | POLLOUT;
	}
	while (1)
	{
		int ev_cnt = poll(pfd, ports, 1000);
		if (ev_cnt > 0)
		{
			for (i = 0; i < ports; i++)
			{
				if (pfd[i].revents & POLLHUP)
					close(pfd[i].fd);

				if (pfd[i].revents & POLLERR)
					close(pfd[i].fd);

				if (pfd[i].revents & POLLIN)
				{
					addr = Sockets_Add[i];

					socklen_t addrlen = sizeof(addr);
					char buffer[100000] = "\0";
					int rcv = recvfrom(S[i], buffer, 100000, 0, (struct sockaddr*)&addr, &addrlen);
					if (rcv > 0)
					{

						unsigned int ip = ntohl(addr.sin_addr.s_addr);
						int number_client = get_cli_num(ip, port_1 + i);
						if (cli_num == number_client)
						{
							clients[number_client].IP = ip;
							clients[number_client].port = port_1 + i;
							clients[number_client].messages_now = 0;
							for (int t = 0; t < 20; t++)
								clients[number_client].deliv_mes[t] = -1;

							cli_num++;
						}

						int res = parse_recv(buffer, number_client, ip, port_1 + i);
						sendto(s, answer, clients[number_client].messages_now * 4, flags, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
						if (res == 2)
						{
							for (int k = 0; k < ports; k++)
								close(S[i]);

							return 0;
						}
					}
				}
			}
		}
	}
	close(s);
	return 0;
}
