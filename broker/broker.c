#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include "broker.h"
#include "protocol.h"
#include "../common/message.h"
#include "../common/queue.h"



#define BROKER_PORT 1883

Topic topics[MAX_TOPICS];
int topic_count = 0;
subscriber_t subscribers[MAX_CLIENTS];
int subscriber_count = 0;

int find_client_index(int socket) {
    for (int i = 0; i < subscriber_count; i++) {
        if (subscribers[i].socket == socket)
            return i;
    }
    return -1;
}
Topic* get_or_create_topic(const char *name) {
    for (int i = 0; i < topic_count; i++) {
        if (strcmp(topics[i].name, name) == 0)
            return &topics[i];
    }

    if (topic_count >= MAX_TOPICS)
        return NULL;

    strcpy(topics[topic_count].name, name);
    queue_init(&topics[topic_count].queue);
    topic_count++;

    return &topics[topic_count - 1];
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
                    char topic_name[MAX_TOPIC];
                    char mode[4] = "NP";  // no persistente por defecto

                    sscanf(buffer, "SUBSCRIBE %s %s", topic_name, mode);

                    strcpy(subscribers[idx].topic, topic_name);
                    subscribers[idx].persistent = (strcmp(mode, "P") == 0);

                    printf("Cliente %d suscrito a %s (%s)\n",
                        sd, topic_name,
                        subscribers[idx].persistent ? "persistente" : "no persistente");

                    // Si es persistente, enviar mensajes históricos
                    if (subscribers[idx].persistent) {
                        for (int t = 0; t < topic_count; t++) {
                            if (strcmp(topics[t].name, topic_name) == 0) {
                                Message msg;
                                MessageQueue *q = &topics[t].queue;

                                // Copia segura de la cola
                                pthread_mutex_lock(&q->mutex);
                                QueueNode *cur = q->head;
                                while (cur) {
                                    char out[BUFFER_SIZE];
                                    snprintf(out, BUFFER_SIZE,
                                            "MESSAGE %s %s\n",
                                            cur->msg.topic,
                                            cur->msg.payload);
                                    write(sd, out, strlen(out));
                                    cur = cur->next;
                                }
                                pthread_mutex_unlock(&q->mutex);
                            }
                        }
                    }
                }

                // PUBLISH
                if (strncmp(buffer, CMD_PUBLISH, strlen(CMD_PUBLISH)) == 0) {
                    char topic_name[MAX_TOPIC];
                    char payload[MAX_MESSAGE];

                    sscanf(buffer, "PUBLISH %s %[^\n]", topic_name, payload);

                    Topic *topic = get_or_create_topic(topic_name);
                    if (!topic) continue;

                    Message msg;
                    strcpy(msg.topic, topic_name);
                    strcpy(msg.payload, payload);
                    msg.timestamp = time(NULL);

                    // Guardar en la cola del tópico
                    queue_push(&topic->queue, &msg);

                    // Reenviar a subscribers
                    for (int j = 0; j < subscriber_count; j++) {
                        if (strcmp(subscribers[j].topic, topic_name) == 0) {
                            char out[BUFFER_SIZE];
                            snprintf(out, BUFFER_SIZE,
                                    "MESSAGE %s %s\n", topic_name, payload);
                            write(subscribers[j].socket, out, strlen(out));
                        }
                    }
                }
            }
        }
    }
}
