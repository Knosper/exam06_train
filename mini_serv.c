#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/select.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_CLIENTS 128
#define BUFFER_SIZE 3

char msg[5500];

int handler(int sig)
{
	static int ex;
	if (sig == SIGINT)
		ex = 42;
	return (ex);
}

void fatal(const char* s)
{
	write(2, s, strlen(s));
}

void close_clients(int* id)
{
	int i = 0;
	while(i < MAX_CLIENTS)
	{
		if (id[i] > -1)
			close(id[i]);
		i++;
	}
}

void send_all(int except, int* id, fd_set write_fd)
{
	int i = 0;
	while(i < MAX_CLIENTS)
	{
		if (id[i] != except && id[i] > -1)
		{
			if (FD_ISSET(id[i], &write_fd))
				send(id[i], msg, strlen(msg), 0);
		}
		i++;
	}
}

void reset_client(int fd, int* id)
{
	int i = 0;
	while(i < MAX_CLIENTS)
	{
		if (fd == id[i])
		{
			id[i] = -1;
			break ;
		}
		i++;
	}
}

int get_max(int sockfd, int* id)
{
	int i = 0;
	int max_fd = sockfd;
	while(i < MAX_CLIENTS)
	{
		if (max_fd < id[i])
			max_fd = id[i];
		i++;
	}
	return (max_fd);
}

int get_id(int fd, int* id)
{
	int i = 0;
	while(i < MAX_CLIENTS)
	{
		if (fd == id[i])
			return (i);
		i++;
	}
	return (-1);
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		fatal("wrong nb of args!\n");
		exit(1);
	}
	int sockfd, connfd, len;
	struct sockaddr_in servaddr, cli; 

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
	{ 
		fatal("socket creation failed...\n"); 
		exit(1); 
	} 
	else
		fatal("Socket successfully created..\n"); 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1])); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
	{ 
		fatal("socket bind failed...\n"); 
		exit(1); 
	} 
	else
		fatal("Socket successfully binded..\n");
	if (listen(sockfd, MAX_CLIENTS) != 0)
	{
		fatal("cannot listen\n"); 
		exit(1); 
	}
	signal(SIGINT, (void*)handler);

	int id[MAX_CLIENTS];
	int id_f[MAX_CLIENTS];
	memset(id, -1, sizeof(id));
	memset(id_f, 0, sizeof(id_f));
	int max_fd = sockfd;
	int g_id = 0;
	fd_set write_fd, read_fd, curr_fd;
	FD_ZERO(&curr_fd);
	FD_SET(sockfd, &curr_fd);

	while (1)
	{
		if (handler(0) == 42)
		{
			fatal("shutdown server!\n");
			break ;
		}
		read_fd = write_fd = curr_fd;
		if (select(max_fd + 1, &read_fd, &write_fd, 0, 0) < 0)
		{
			fatal("select failed, continue...\n");
			continue ;
		}
		int fd = 0;
		while (fd <= max_fd)
		{
			if (FD_ISSET(fd, &read_fd))
			{
				if (sockfd == fd)
				{
					struct sockaddr_in client;
					socklen_t len = sizeof(client);
					int client_fd = accept(fd, (struct sockaddr*)&client, &len);
					if (client_fd >= 0)
					{
						FD_SET(client_fd, &curr_fd);
						fcntl(client_fd, F_SETFL, O_NONBLOCK);
						id[g_id] = client_fd;
						id_f[g_id] = 0;
						if (max_fd < client_fd)
							max_fd = client_fd;
						sprintf(msg, "server: client %d just arrived\n", g_id);
						send_all(client_fd, id, write_fd);
						g_id++;
					}
					break ;
				}
				else
				{
					char buffer[BUFFER_SIZE + 1];
					int ret = recv(fd, buffer, BUFFER_SIZE, 0);
					if (ret <= 0)
					{

						sprintf(msg, "server: client %d just left\n", get_id(fd, id));
						send_all(fd, id, write_fd);
						FD_CLR(fd, &curr_fd);
						close(fd);
						reset_client(fd, id);
						max_fd = get_max(sockfd, id);
						id_f[get_id(fd, id)] = 0;
					}
					else
					{
						int i = 0;
						int j = 0;
						char buff[ret + 2];
						buffer[ret] = '\0';
						while (i < ret)
						{
							buff[j] = buffer[i];
							if (buff[j] == '\n')
							{
								buff[j + 1] = '\0';
								if (id_f[get_id(fd, id)])
									sprintf(msg, "%s", buff);
								else
									sprintf(msg, "client %d: %s", get_id(fd, id), buff);
								id_f[get_id(fd, id)] = 0;
								send_all(fd, id, write_fd);
							}
							else if (i == ret - 1)
							{
								buff[j + 1] = '\0';
								if (id_f[get_id(fd, id)])
									sprintf(msg, "%s", buff);
								else
									sprintf(msg, "client %d: %s", get_id(fd, id), buff);
								id_f[get_id(fd, id)] = 1;
								send_all(fd, id, write_fd);
							}
							j++;
							i++;
						}
					}
				}
			}
			fd++;
		}
	}
	close(sockfd);
	close_clients(id);
	return (0);
}