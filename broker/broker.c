#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include "broker.h"
#include "protocol.h"

#define BROKER_PORT 1883

subscriber_t subscribers[MAX_CLIENTS];
int subscriber_count = 0;

int find_client_index(int socket) {
    for (int i = 0; i < subscriber_count; i++) {
        if (subscribers[i].socket == socket)
            return i;
    }
    return -1;
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr;
    fd_set read_fds;
    int max_fd;
    char buffer[BUFFER_SIZE];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(BROKER_PORT);

    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_fd, 5);

    printf("Broker escuchando en el puerto %d...\n", BROKER_PORT);
    fflush(stdout);

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        max_fd = server_fd;

        for (int i = 0; i < subscriber_count; i++) {
            FD_SET(subscribers[i].socket, &read_fds);
            if (subscribers[i].socket > max_fd)
                max_fd = subscribers[i].socket;
        }

        select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        // Nueva conexión
        if (FD_ISSET(server_fd, &read_fds)) {
            client_fd = accept(server_fd, NULL, NULL);

            subscribers[subscriber_count].socket = client_fd;
            strcpy(subscribers[subscriber_count].topic, "");
            subscriber_count++;

            printf("Nuevo cliente conectado: %d\n", client_fd);
        }

        // Mensajes
        for (int i = 0; i < subscriber_count; i++) {
            int sd = subscribers[i].socket;

            if (FD_ISSET(sd, &read_fds)) {
                int bytes = read(sd, buffer, BUFFER_SIZE - 1);

                if (bytes <= 0) {
                    close(sd);
                    subscribers[i] = subscribers[--subscriber_count];
                    continue;
                }

                buffer[bytes] = '\0';

                int idx = find_client_index(sd);
                if (idx < 0) continue;

                // SUBSCRIBE
                if (strncmp(buffer, CMD_SUBSCRIBE, strlen(CMD_SUBSCRIBE)) == 0) {
                    sscanf(buffer, "SUBSCRIBE %s", subscribers[idx].topic);
                    printf("Cliente %d suscrito a %s\n",
                           sd, subscribers[idx].topic);
                }

                // PUBLISH
                if (strncmp(buffer, CMD_PUBLISH, strlen(CMD_PUBLISH)) == 0) {
                    char topic[MAX_TOPIC];
                    char msg[MAX_MESSAGE];

                    sscanf(buffer, "PUBLISH %s %[^\n]", topic, msg);

                    for (int j = 0; j < subscriber_count; j++) {
                        if (strcmp(subscribers[j].topic, topic) == 0) {
                            char out[BUFFER_SIZE];
                            snprintf(out, BUFFER_SIZE,
                                     "MESSAGE %s %s\n", topic, msg);
                            write(subscribers[j].socket, out, strlen(out));
                        }
                    }
                }
            }
        }
    }
}
