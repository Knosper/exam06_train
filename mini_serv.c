#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#define BUFFER_SIZE 4096

typedef struct s_client
{
    int c_name;
    int c_fd;
    struct sockaddr_in c_addr;
    socklen_t c_len;
}   t_client;

static int handler(int signum)
{
    static int _case;

    if (signum == SIGINT)
        _case = 42;
    
    if (_case == 42)
        return (42);
    return (0);
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    signal(SIGINT, (void *)handler);

    int port = atoi(argv[1]);
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1)
    {
        perror("Error creating socket");
        exit(1);
    }
    int opt = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("setsockopt");
        close(sock_fd);
        exit(1);
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Error binding socket");
        close(sock_fd);
        exit(1);
    }

    if (listen(sock_fd, 5) == -1)
    {
        perror("Error listening to socket");
        close(sock_fd);
        exit(1);
    }

    printf("Server is listening on port %d\n", port);
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd == -1)
    {
        perror("Error accepting client connection");
    }
    char buff[BUFFER_SIZE];
    printf("Accepted a connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    int i = 0;
    int send_bytes;
    int stop = 0;
    while (stop == 0)
    {
        stop = handler(0);
        int bytes = recv(client_fd, buff, BUFFER_SIZE, 0);
        if (bytes > 0)
        {
            buff[bytes] = '\0';
            printf("Received %d bytes\n", bytes);
            printf("Received message:\n%s\n", buff);
            send_bytes = send(client_fd, buff, bytes, 0);
            if (send_bytes < 0)
            {
                perror("Send error!");
            }
        }
        i++;
        if (i % 22 == 0)
        {
            printf("bytes == %i\n", bytes);
        }
        if (bytes <= 0)
        {
            continue ;
        }
        close(client_fd);
        client_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == -1)
        {
            perror("Error accepting client connection");
        }
    }
    if (client_fd)
        close(client_fd);
    close(sock_fd);
    return (0);
}
