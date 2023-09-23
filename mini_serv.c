#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>

#include <signal.h>

#define BUFFER_SIZE 50000
#define MAX_CLIENTS 128

char msg[40];

int handler(int sig)
{
    static int ex;
    if (sig == SIGINT)
        ex = 42;
    
    return (ex);
}

void close_clients(int *id)
{
    int i = 0;
    while (i < MAX_CLIENTS)
    {
        if (id[i] > 0)
        {
            close(id[i]);
        }
        i++;
    }
}

int fatal(const char* msg)
{
    return (write(2, msg, strlen(msg)));
}

int send_all(int client_fd, int max_fd, fd_set write_fd, int *id)
{
    int i = 0;
    while (i <= max_fd)
    {
        if (id[i] != client_fd && id[i] > 0)
        {
            if (FD_ISSET(id[i], &write_fd))
            {
                int bytes = send(id[i], msg, strlen(msg), 0);
            }
        }
        i++;
    }
    return (0);
}

int find_max_fd(int max_fd, int* id, int sockfd)
{
    int i = 0;
    max_fd = sockfd;
    while (i < MAX_CLIENTS)
    {
        if (max_fd < id[i])
            max_fd = id[i];
        i++;
    }
    return (max_fd);
}

void reset_client(int fd, int *id)
{
    int i = 0;
    while (i < MAX_CLIENTS)
    {
        if (fd == id[i])
        {
            id[i] = 0;
            break ;
        }
        i++;
    }
}

int find_client(int fd, int *id)
{
    int i = 0;
    while (i < MAX_CLIENTS)
    {
        if (fd == id[i])
            return (i);
        i++;
    }
    return (-1);
}


int main(int argc, char **argv)
{
    if (argc != 2)
        return (fatal("wrong args!\n"));
	int sockfd, connfd, len;
	struct sockaddr_in servaddr, cli; 
    
	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
    { 
        fatal("socket failed\n");
		exit(1); 
	} 
	else
		printf("Socket successfully created..\n"); 

	bzero(&servaddr, sizeof(servaddr)); 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = 16777343; //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1]));
  
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
    { 
        fatal("bind failed\n");
		exit(0); 
	} 
	else
		printf("Socket successfully binded..\n");
	if (listen(sockfd, 128) != 0)
    {
        fatal("listen failed\n");
		exit(0); 
	}
    signal(SIGINT, (void*)handler);

    fd_set read_fd, write_fd, curr_fd;
    FD_ZERO(&curr_fd);
    FD_SET(sockfd, &curr_fd);
    int id[MAX_CLIENTS];
    memset(id,0,sizeof(id));
    int max_fd = sockfd;
    int n_id = 0;

    while (1)
    {
        read_fd = write_fd = curr_fd;
        if (handler(0) == 42)
        {
            (void)fatal("shutdown server\n");
            break ;
        }
        if (select(max_fd + 1, &read_fd, &write_fd, 0, 0) < 0)
        {
            (void)fatal("select failed\n");
            continue ;
        }
        int fd = 0;
        while (fd <= max_fd)
        {
            if (FD_ISSET(fd, &read_fd))
            {
                if (fd == sockfd)
                {
                    struct sockaddr_in client;
                    socklen_t len = sizeof(client);
                    int client_fd = accept(fd, (struct sockaddr *)&client, &len);
                    //printf("client accepted [%i]\n", n_id);
                    if (client_fd >= 0)
                    {
                        FD_SET(client_fd, &curr_fd);
                        id[n_id++] = client_fd;
                        if (max_fd < client_fd)
                            max_fd = client_fd;
                        sprintf(msg, "server: client %d just arrived\n", find_client(client_fd, id));
                        send_all(client_fd, max_fd, write_fd, id);
                    }
                }
                else
                {
                    char buffer[BUFFER_SIZE];
                    int ret = recv(fd, buffer, BUFFER_SIZE - 1, 0);
                    if (ret <= 0)
                    {
                        sprintf(msg, "server: client %d just left\n", find_client(fd, id));
                        send_all(fd, max_fd, write_fd, id);
                        FD_CLR(fd, &curr_fd);
                        close(fd);
                        reset_client(fd, id);
                        max_fd = find_max_fd(max_fd, id, sockfd);
                    }
                    else
                    {
                        buffer[ret] = '\0';
                        char *buff = buffer;
                        sprintf(msg, "client %d: %s", find_client(fd, id), buff);
                        send_all(fd, max_fd, write_fd, id);
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