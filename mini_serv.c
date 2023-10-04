#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/select.h>
#include <signal.h>
#include <stdlib.h>

#define MAX_CLIENTS 128
#define BUFFER_SIZE 3

char msg[55000];

typedef struct s_id
{
    int id;
    int fd;
    int msg;
}   t_id;

void fatal(const char* s)
{
    write(2, s, strlen(s));
}

int handler(int sig)
{
    static int ex;
    if (sig == SIGINT)
        ex = 42;
    return (ex);
}

void close_clients(t_id *id)
{
    int i = 0;
    while (i < MAX_CLIENTS)
    {
        if (id[i].fd >= 0)
            close(id[i].fd);
        i++;
    }
}

void send_all(int except, t_id *id, fd_set write_fd)
{
    int i = 0;
    while (i < MAX_CLIENTS)
    {
        if (id[i].fd >= 0 && id[i].fd != except)
        {
            if (FD_ISSET(id[i].fd, &write_fd))
            {
                send(id[i].fd, msg, strlen(msg), 0);
            }
        }
        i++;
    }
}

void init_id(t_id *id)
{
    int i = 0;
    while (i < MAX_CLIENTS)
    {
        id[i].fd = -1;
        id[i].msg = -1;
        id[i].id = -1;
        i++;
    }
}

int get_id(int fd, t_id *id)
{
    int i = 0;
    while (i < MAX_CLIENTS)
    {
        if (fd == id[i].fd)
            return (id[i].id);
        i++;
    }
    return (-1);
}

void reset_client(int fd, t_id *id)
{
    int i = 0;
    while (i < MAX_CLIENTS)
    {
        if (fd == id[i].fd)
        {
            id[i].fd = -1;
            id[i].msg = -1;
            id[i].id = -1;
            break ;
        }
        i++;
    }
}

int get_max(int sockfd, t_id *id)
{
    int i = 0;
    int max_fd = sockfd;
    while (i < MAX_CLIENTS)
    {
        if (max_fd < id[i].fd)
            max_fd = id[i].fd;
        i++;
    }
    return (max_fd);
}

int get_msg_flag(int fd, t_id* id)
{
    int i = 0;
    while (i < MAX_CLIENTS)
    {
        if (fd == id[i].fd)
            return (id[i].msg);
        i++;
    }
    return (0);
}

void set_id_flag(int fd, t_id* id, int status)
{
    int i = 0;
    while (i < MAX_CLIENTS)
    {
        if (fd == id[i].fd)
        {
            id[i].msg = status;
            break ;
        }
        i++;
    }
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        fatal("wrong args!\n");
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
		printf("Socket successfully created..\n"); 
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
		printf("Socket successfully binded..\n");
	if (listen(sockfd, MAX_CLIENTS) != 0)
    {
		fatal("cannot listen\n"); 
		exit(1); 
	}
	signal(SIGINT, (void*)handler);
    fd_set read_fd, write_fd, curr_fd;
    FD_ZERO(&curr_fd);
    FD_SET(sockfd, &curr_fd);
    int max_fd = sockfd;
    t_id id[MAX_CLIENTS];
    init_id(id);
    int g_id = 0;
 
    while (1)
    {
        read_fd = write_fd = curr_fd;
        if (handler(0) == 42)
        {
            fatal("shutdown server\n");
            break ;
        }
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
                        id[g_id].id = g_id;
                        id[g_id].msg = 0;
                        id[g_id++].fd = client_fd;
                        if (client_fd > max_fd)
                            max_fd = client_fd;
                        sprintf(msg, "server: client %d just arrived\n", g_id - 1);
                        send_all(client_fd, id, write_fd);
                        break ;                    
                    }
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
                        break ;
                    }
                    else
                    {
                        buffer[ret] = '\0';
                        int i = 0;
                        int j = 0;
                        char buff[ret + 2];
                        while (i < ret)
                        {
                            buff[j] = buffer[i];
                            if (buff[j] == '\n')
                            {
                                buff[j + 1] = '\0';
                                if (!get_msg_flag(fd, id))
                                    sprintf(msg, "client %d: %s", get_id(fd, id), buff);
                                else
                                    sprintf(msg, "%s", buff);
                                set_id_flag(fd, id, 0);
                                send_all(fd, id, write_fd);
                                j = -1;
                            }
                            else if (i == ret - 1)
                            {
                                buff[j + 1] = '\0';
                                if (get_msg_flag(fd, id))
                                {
                                    sprintf(msg, "%s", buff);
                                }
                                else
                                    sprintf(msg, "client %d: %s", get_id(fd, id), buff);
                                set_id_flag(fd, id, 1);
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