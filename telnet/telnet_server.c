#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define BUF_SIZE 1024

int check_login(char *user, char *pass) {
    FILE *f = fopen("users.txt", "r");
    if (!f) return 0;

    char u[50], p[50];

    while (fscanf(f, "%s %s", u, p) != EOF) {
        if (strcmp(u, user) == 0 && strcmp(p, pass) == 0) {
            fclose(f);
            return 1;
        }
    }

    fclose(f);
    return 0;
}

void handle_client(int client_fd) {
    char buf[BUF_SIZE];
    char username[50];

    // ask username
    send(client_fd, "Username:\n", 10, 0);

    int n = recv(client_fd, buf, sizeof(buf) - 1, 0);
    if (n <= 0) {
        close(client_fd);
        exit(0);
    }

    buf[n] = 0;
    buf[strcspn(buf, "\r\n")] = 0;
    strcpy(username, buf);

    // ask password
    send(client_fd, "Password:\n", 10, 0);

    n = recv(client_fd, buf, sizeof(buf) - 1, 0);
    if (n <= 0) {
        close(client_fd);
        exit(0);
    }

    buf[n] = 0;
    buf[strcspn(buf, "\r\n")] = 0;

    // check login
    if (!check_login(username, buf)) {
        send(client_fd, "Login failed!\n", 14, 0);
        close(client_fd);
        exit(0);
    }

    send(client_fd, "Login success!\n$ ", 18, 0);

    // command loop
    while (1) {
        n = recv(client_fd, buf, sizeof(buf) - 1, 0);

        if (n <= 0) {
            break;
        }

        buf[n] = 0;
        buf[strcspn(buf, "\r\n")] = 0;

        if (strcmp(buf, "exit") == 0) {
            break;
        }

        char cmd[256];

        snprintf(cmd, sizeof(cmd), "%.200s > out.txt", buf);

        system(cmd);

        FILE *f = fopen("out.txt", "r");

        if (!f) {
            send(client_fd, "Command error\n$ ", 17, 0);
            continue;
        }

        char line[BUF_SIZE];

        while (fgets(line, sizeof(line), f)) {
            send(client_fd, line, strlen(line), 0);
        }

        fclose(f);

        send(client_fd, "$ ", 2, 0);
    }

    close(client_fd);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[1]));
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    listen(listen_fd, 10);

    printf("Server listening on port %s...\n", argv[1]);

    while (1) {
        int client_fd = accept(listen_fd, NULL, NULL);

        if (client_fd < 0) {
            continue;
        }

        pid_t pid = fork();

        if (pid == 0) {
            // child process
            close(listen_fd);

            handle_client(client_fd);
        }
        else if (pid > 0) {
            // parent process
            close(client_fd);

            // avoid zombie process
            while (waitpid(-1, NULL, WNOHANG) > 0);
        }
    }

    close(listen_fd);

    return 0;
}