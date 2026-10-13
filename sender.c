#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 9000

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pi-ip>\n", argv[0]);
        return 1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT)
    };

    if (inet_pton(AF_INET, argv[1], &server.sin_addr) != 1) {
        fprintf(stderr, "Invalid Pi IP address\n");
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    for (int i = 0; i < 10; i++) {
        char message[128];
        int len = snprintf(message, sizeof(message),
                           "Hello from Jetson! Packet %d\n", i);

        size_t sent = 0;
        while (sent < (size_t)len) {
            ssize_t n = send(sock, message + sent,
                             (size_t)len - sent, 0);
            if (n <= 0) {
                perror("send");
                close(sock);
                return 1;
            }
            sent += (size_t)n;
        }

        printf("Sent: %s", message);
        sleep(1);
    }

    close(sock);
    return 0;
}
