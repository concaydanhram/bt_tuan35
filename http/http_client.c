#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUF_SIZE 4096

int main() {

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock,
                (struct sockaddr*)&server_addr,
                sizeof(server_addr)) < 0) {

        perror("connect");
        exit(1);
    }

    char *request =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    send(sock, request, strlen(request), 0);

    char buf[BUF_SIZE];

    int ret = recv(sock, buf, sizeof(buf) - 1, 0);

    if (ret > 0) {
        buf[ret] = 0;

        printf("Response from server:\n");
        printf("%s\n", buf);
    }

    close(sock);

    return 0;
}