#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include "broker.h"
#include "../common/protocol.h"

subscriber_t subscribers[MAX_CLIENTS];
int subscriber_count = 0;

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr;
    fd_set read_fds;
    int max_fd;

    char buffer[BUFFER_SIZE];

    // 1. Crear socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }

    // 2. Configurar dirección
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 3. Bind
    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    // 4. Listen
    listen(server_fd, 5);

    printf("Broker escuchando en el puerto %d...\n", PORT);

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        max_fd = server_fd;

        // Agregar clientes suscritos al select
        for (int i = 0; i < subscriber_count; i++) {
            FD_SET(subscribers[i].socket, &read_fds);
            if (subscribers[i].socket > max_fd)
                max_fd = subscribers[i].socket;
        }

        select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        // Nueva conexión
        if (FD_ISSET(server_fd, &read_fds)) {
            client_fd = accept(server_fd, NULL, NULL);
            printf("Nuevo cliente conectado: %d\n", client_fd);
        }

        // Mensajes de clientes
        for (int i = 0; i < subscriber_count; i++) {
            int sd = subscribers[i].socket;

            if (FD_ISSET(sd, &read_fds)) {
                int bytes = read(sd, buffer, BUFFER_SIZE);
                if (bytes <= 0) {
                    close(sd);
                    subscribers[i] = subscribers[--subscriber_count];
                    continue;
                }

                buffer[bytes] = '\0';

                // SUBSCRIBE
                if (strncmp(buffer, CMD_SUBSCRIBE, strlen(CMD_SUBSCRIBE)) == 0) {
                    sscanf(buffer, "SUBSCRIBE %s", subscribers[subscriber_count].topic);
                    subscribers[subscriber_count].socket = sd;
                    subscriber_count++;

                    printf("Cliente %d suscrito a %s\n", sd, subscribers[subscriber_count-1].topic);
                }

                // PUBLISH
                if (strncmp(buffer, CMD_PUBLISH, strlen(CMD_PUBLISH)) == 0) {
                    char topic[MAX_TOPIC];
                    char msg[MAX_MESSAGE];

                    sscanf(buffer, "PUBLISH %s %[^\n]", topic, msg);

                    for (int j = 0; j < subscriber_count; j++) {
                        if (strcmp(subscribers[j].topic, topic) == 0) {
                            char out[BUFFER_SIZE];
                            snprintf(out, BUFFER_SIZE, "MESSAGE %s %s\n", topic, msg);
                            write(subscribers[j].socket, out, strlen(out));
                        }
                    }
                }
            }
        }
    }

    return 0;
}
