#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 9000
#define BUFFER_SIZE 1024

int main(void) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY)
    };

    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Listening on TCP port %d...\n", PORT);
    fflush(stdout);

    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("accept");
        close(server_fd);
        return 1;
    }

    printf("Jetson connected!\n");

    char buffer[BUFFER_SIZE];

    while (1) {
        ssize_t n = recv(client_fd, buffer, sizeof(buffer), 0);

        if (n == 0) {
            printf("Jetson disconnected.\n");
            break;
        }

        if (n < 0) {
            perror("recv");
            break;
        }

        printf("Received %zd bytes: ", n);
        fwrite(buffer, 1, (size_t)n, stdout);
        fflush(stdout);
    }

    close(client_fd);
    close(server_fd);
    return 0;
}
