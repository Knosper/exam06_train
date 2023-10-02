#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#define MAX_CLIENTS     128
#define BUFFER_SIZE     42

char msg[BUFFER_SIZE + BUFFER_SIZE];

int handler(int sig)
{
    static int ex;
    if (sig == SIGINT)
        ex = 42;
    return (ex);
}

void close_clients(int* id)
{
    int i = 0;
    while (i < MAX_CLIENTS)
    {
        if (id[i] >= 0)
            close(id[i]);
        i++;
    }
}

int get_id(int fd, int* id)
{
    int i = 0;
    while (i < MAX_CLIENTS)
    {
        if (id[i] == fd)
            return (i);
        i++;
    }
    return (-1);
}

void send_all(int except, int *id, fd_set write_fd)
{
    int i = 0;
    while (i < MAX_CLIENTS)
    {
        if (id[i] >= 0 && id[i] != except)
        {
            if (FD_ISSET(id[i], &write_fd))
            {
                send(id[i], msg, strlen(msg), 0);
            }
        }
        i++;
    }
}

void reset_id(int fd, int *id)
{
    int i = 0;
    while (i < MAX_CLIENTS)
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
    int max_fd = sockfd;
    int i = 0;
    while (i < MAX_CLIENTS)
    {
        if (max_fd < id[i])
            max_fd = id[i];
        i++;
    }
    return (max_fd);
}

int main(int argc, char** argv)
{
	int sockfd, connfd, len;
	struct sockaddr_in servaddr, cli; 

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		printf("socket creation failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully created..\n"); 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1])); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		printf("socket bind failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully binded..\n");
	if (listen(sockfd, 10) != 0) {
		printf("cannot listen\n"); 
		exit(0); 
	}
    signal(SIGINT, (void*)handler);
    int id[MAX_CLIENTS];
    int id_flag[MAX_CLIENTS];
    memset(id_flag, 0, sizeof(id));
    memset(id, -1, sizeof(id));
    int max_fd = sockfd;
    fd_set read_fd, write_fd, curr_fd;
    FD_ZERO(&curr_fd);
    FD_SET(sockfd, &curr_fd);
    int g_id = 0;
    while (1)
    {
        read_fd = write_fd = curr_fd;
        if (handler(0) == 42)
        {
            break ;
        }
        if (select(max_fd + 1, &read_fd, &write_fd, 0, 0) < 0)
        {
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
                    int len = sizeof(client);
                    int client_fd = accept(fd, (struct sockaddr*)&client, &len);
                    if (client_fd >= 0)
                    {
                        FD_SET(client_fd, &curr_fd);
                        id[g_id++] = client_fd;
                        if (client_fd > max_fd)
                            max_fd = client_fd;
                        sprintf(msg, "server: client %d just arrived\n", get_id(client_fd, id));
                        send_all(client_fd, id, write_fd);
                    }
                }
                else
                {
                    char buffer[BUFFER_SIZE + 1];
                    int i = 0;
                    int j = 0;
                    int ret = recv(fd, buffer, BUFFER_SIZE, 0);
                    if (ret <= 0)
                    {
                        sprintf(msg, "server: client %d just left\n", get_id(fd, id));
                        send_all(fd, id, write_fd);
                        FD_CLR(fd, &curr_fd);
                        close(fd);
                        reset_id(fd, id);
                        max_fd = get_max(sockfd, id);
                        break;
                    }
                    else
                    {
                        char str[BUFFER_SIZE + 2];
                        while (i < ret)
                        {
                            str[j] = buffer[i];
                            if (str[j] == '\n')
                            {
                                str[j + 1] = '\0';
                                if (id_flag[get_id(fd, id)])
                                {
                                    sprintf(msg, "%s", str);
                                }
                                else
                                {
                                    sprintf(msg, "client %d: %s", get_id(fd, id), str);
                                }
                                id_flag[get_id(fd, id)] = 0;
                                send_all(fd, id, write_fd);
                                j = -1;
                            }
                            else if (i == ret - 1)
                            {
                                str[j + 1] = '\0';
                                if (id_flag[get_id(fd, id)])
                                {
                                    sprintf(msg, "%s", str);
                                }
                                else
                                    sprintf(msg, "client %d: %s", get_id(fd, id), str);
                                id_flag[get_id(fd, id)] = 1;
                                send_all(fd, id, write_fd);
                                break ;
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


    close_clients(id);
    close(sockfd);
    return (0);
}