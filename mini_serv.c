#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#define BUFFER_SIZE 4096 * 42
#define MAX_CLIENT  65536

int clients[MAX_CLIENT];
int id[MAX_CLIENT];
int currmsg[MAX_CLIENT];
char buffer[BUFFER_SIZE];
char string[BUFFER_SIZE];
char msg[BUFFER_SIZE + 42];

void entry_message(int except_fd, int max_fd, fd_set *cpy_write)
{
    for (int i = 0; i <= max_fd; i++)
    {
        if (FD_ISSET(i, cpy_write) && except_fd != i)
        {
            send(i, msg, strlen(msg), 0);
        }
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return (write(2, "Error Args!\n", 12));

    int sock_fd;
    struct sockaddr_in server_add;

    memset(&server_add, 0, sizeof(server_add));
    int port = atoi(argv[1]);
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1)
        return (write(2, "Error create socket!\n", 21));

    server_add.sin_addr.s_addr = htonl(INADDR_ANY);
    server_add.sin_port = htons(port);
    server_add.sin_family = AF_INET;

    if (bind(sock_fd, (struct sockaddr *)&server_add, sizeof(server_add)) < 0)
    {
        close(sock_fd);
        return (write(2, "Error binding socket!\n", 22));
    }

    if (listen(sock_fd, 5) < 0)
        return (write(2, "Error listen to socket!\n", 24));

    fd_set curr_sock, cpy_read, cpy_write;
    FD_ZERO(&curr_sock);
    FD_SET(sock_fd, &curr_sock);

    memset(id, 0, sizeof(id));
    int max_fd = sock_fd;
    int g_id = 0;

    while (1)
    {
        cpy_read = cpy_write = curr_sock;
        if (select(max_fd + 1, &cpy_read, &cpy_write, NULL, NULL) < 0)
        {
            perror("select");
            continue;
        }
        for (int fd = 0; fd <= max_fd; fd++)
        {
            if (FD_ISSET(fd, &cpy_read))
            {
                if (fd == sock_fd)
                {
                    struct sockaddr_in clientaddr;
                    socklen_t len = sizeof(clientaddr);
                    int client_fd = accept(fd, (struct sockaddr *)&clientaddr, &len);
                    if (client_fd < 0)
                        continue;
                    FD_SET(client_fd, &curr_sock);
                    id[client_fd] = g_id++;
                    if (max_fd < client_fd)
                        max_fd = client_fd;
                    sprintf(msg, "server: client %d just arrived\n", id[client_fd]);
                    entry_message(client_fd, max_fd, &cpy_write);
                }
                else
                {
                    int ret = recv(fd, buffer, BUFFER_SIZE, 0);
                    if (ret <= 0)
                    {
                        sprintf(msg, "server: client %d just left\n", id[fd]);
                        entry_message(fd, max_fd, &cpy_write);
                        FD_CLR(fd, &curr_sock);
                        close(fd);
                    }
                    else
                    {
                        for (int i = 0, j = 0; i < ret; i++, j++)
                        {
                            string[j] = buffer[i];
                            if (string[j] == '\n')
                            {
                                string[j + 1] = '\0';
                                if (currmsg[fd])
                                    sprintf(msg, "%s", string);
                                else
                                    sprintf(msg, "client %d: %s", id[fd], string);
                                currmsg[fd] = 0;
                                entry_message(fd, max_fd, &cpy_write);
                                j = -1;
                            }
                            else if (i == (ret - 1))
                            {
                                string[j + 1] = '\0';
                                if (currmsg[fd])
                                    sprintf(msg, "%s", string);
                                else
                                    sprintf(msg, "client %d: %s", id[fd], string);
                                currmsg[fd] = 1;
                                entry_message(fd, max_fd, &cpy_write);
                            }
                        }
                    }
                }
            }
        }
    }
    return (0);
}
