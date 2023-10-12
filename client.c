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
#include <arpa/inet.h>

int main(int argc, char** argv)
{

    char buffer[4096];
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    { 
        printf("socket creation failed...\n"); 
        exit(0); 
	}
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(89);
    addr.sin_addr.s_addr = htonl(2130706433);
    if(connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Connected to the server.\n");
    // Send a message to the server
    char msg[42];
    sprintf(msg, "Hello, Server!\nHallo\n\n\n");
    int i = 0;
    int bytes = 0;
    while (i++ < 100)
    {
        bytes = send(sockfd, msg, strlen(msg), 0);
        sleep(1);
        if (bytes < 0)
            break ;
    }
    printf("Message sent: %s\nbytes = %i\n", msg, bytes);
    // Close the socket and exit
    close(sockfd);
    printf("Connection closed.\n");
    return (0);
}

