#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <signal.h>

#define BUFFER_SIZE 4096
#define MAX_CLIENT  10

typedef struct s_data
{
    struct sockaddr_in server_add;
    int port;
    int n;
    int sock_fd;
}   t_data;

typedef struct s_client
{
    int c_id;
    int c_fd;
}   t_client;

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

static int handler(int signum)
{
    static int save = 0;
    if (signum == SIGINT)
        save = 99;

    if (save == 99)
    {
        return (1);
    }
    return (0);
}

void close_clients(t_client *clients)
{
    int i = 0;

    while (i < MAX_CLIENT)
    {
        if (clients[i].c_fd != -1)
            close(clients[i].c_fd);
        i++;
    }
}

t_client add_client(t_client client, int sock_fd, t_data data)
{
    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);

    if ((client.c_fd = accept(sock_fd, (struct sockaddr *)&clientaddr, &len)) < 0)
    {  
        close(sock_fd);
        write(2, "ERROR adding client\n", 20);
        memset(&client, 0, sizeof(client));
        return (client);
    }
    client.c_id = data.n - 1;
    //printf("client fd set to:%i\nnumber:%i\n", client.c_fd, client.c_id);
    return (client);
}

int check_last(t_data data, t_client *clients)
{
    int i = 0;
    int max_fd = data.sock_fd;
    while (i < MAX_CLIENT && i < data.n)
    {
        if (clients[i].c_fd > max_fd)
            max_fd = clients[i].c_fd;
        i++;
    }
    return (max_fd);
}

int entry_message(t_client* clients, t_data data, fd_set cpy_read)
{
    int i = 0;
    char message[32];
    sprintf(message, "server: client %d just arrived\n", clients[data.n - 1].c_id);
    while (i < data.n)
    {
        if (clients[i].c_fd != -1 && clients[i].c_id != data.n - 1)
        {
            if (send(clients[i].c_fd, message, strlen(message), 0) < 0)
            {
                close(data.sock_fd);
                write(2, "ERROR in entry message!\n", 24);
                return (1);
            }
        }
        i++;
    }
    return (0);
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return (write(2, "Error Args!\n", 12));
    signal(SIGINT,  (void *)handler);
    t_data data;
    memset(&data, 0, sizeof(t_data));
    data.port = atoi(argv[1]);
    data.sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (data.sock_fd == -1)
        return (write(2, "Error create socket!\n", 21));

    data.server_add.sin_addr.s_addr = 16777343;
	data.server_add.sin_port = ((data.port & 0xFF) << 8) | ((data.port >> 8) & 0xFF);
    data.server_add.sin_family = AF_INET;
    
    if (bind(data.sock_fd, (struct sockaddr *)&data.server_add, sizeof(data.server_add)) < 0)
    {
        close(data.sock_fd);
        return (write(2, "Error binding socket!\n", 22));
    }

    if (listen(data.sock_fd, 5) < 0)
        return (write(2, "Error listen to socket!\n", 24));

    data.n += 1;
    fd_set curr_sock;
    fd_set cpy_write;
    fd_set cpy_read;
    FD_ZERO(&curr_sock);
    FD_SET(data.sock_fd, &curr_sock);
    char buffer[BUFFER_SIZE + 1];
    char *message = NULL;
    memset(buffer, 0, BUFFER_SIZE);
    t_client clients[MAX_CLIENT + 1];
    memset(clients, -1, sizeof(clients));

    while(1)
    {
        cpy_write = cpy_read = curr_sock;
        if (handler(0))
            break ;
        if (select(check_last(data, clients) + 1, &cpy_read, &cpy_write, NULL, NULL) < 0)
        {
            perror("select:");
            continue ;
        }
        int fd = 0;
        while (fd <= check_last(data, clients) + 1)
        {
            if (FD_ISSET(fd, &cpy_read))
            {
                if (fd == data.sock_fd)
                {
                    memset(&buffer, 0, BUFFER_SIZE);
                    if (data.n - 1 < MAX_CLIENT)
                    {

                        clients[data.n - 1] = add_client(clients[data.n - 1], data.sock_fd, data);
                        if (clients[data.n - 1].c_fd == 0 && clients[data.n - 1].c_id == 0)
                        {
                            close_clients(clients);
                            printf("ERROR clients[data.n - 1].c_fd...\n");
                            close(data.sock_fd);
                            exit(1);
                        }
                        if (entry_message(clients, data, cpy_read))
                        {
                            close_clients(clients);
                            close(data.sock_fd);
                            exit(1);
                        }
                        FD_SET(clients[data.n - 1].c_fd, &curr_sock);
                        data.n += 1;
                    }
                    break ;
                }
            }
            fd++;
        }
    }
    close_clients(clients);
    close(data.sock_fd);
    return (0);
}

