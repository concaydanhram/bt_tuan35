#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>

#define PORT 8080
#define WORKERS 4
#define BUF_SIZE 1024

void worker_process(int listener) {
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client = accept(listener,
                            (struct sockaddr*)&client_addr,
                            &client_len);

        if (client < 0) {
            perror("accept");
            continue;
        }

        printf("[PID %d] Client connected\n", getpid());

        char buf[BUF_SIZE];

        int ret = recv(client, buf, sizeof(buf) - 1, 0);

        if (ret > 0) {
            buf[ret] = 0;

            printf("Request:\n%s\n", buf);

            char *response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "\r\n"
                "<html>"
                "<body>"
                "<h1>Hello from Preforking Server</h1>"
                "</body>"
                "</html>";

            send(client, response, strlen(response), 0);
        }

        close(client);
    }
}

int main() {

    int listener = socket(AF_INET, SOCK_STREAM, 0);

    if (listener < 0) {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listener,
             (struct sockaddr*)&server_addr,
             sizeof(server_addr)) < 0) {

        perror("bind");
        exit(1);
    }

    if (listen(listener, 10) < 0) {
        perror("listen");
        exit(1);
    }

    printf("HTTP Server listening on port %d\n", PORT);

    // PREFORKING
    for (int i = 0; i < WORKERS; i++) {

        pid_t pid = fork();

        if (pid == 0) {
            // Child process
            worker_process(listener);
            exit(0);
        }
    }

    // Parent process waits
    while (1) {
        wait(NULL);
    }

    close(listener);

    return 0;
}