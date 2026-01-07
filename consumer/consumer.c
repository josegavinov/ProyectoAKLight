#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "protocol.h"

#define BROKER_HOST "broker"
#define BROKER_PORT 1883
#define TOPIC "sistema/memoria/uso"

int main() {
    int sock;
    struct sockaddr_in broker_addr;
    struct hostent *server;
    char buffer[BUFFER_SIZE];

    printf("Subscriber iniciando...\n");
    fflush(stdout);

    // Resolver DNS
    server = gethostbyname(BROKER_HOST);
    if (!server) {
        fprintf(stderr, "No se pudo resolver broker\n");
        exit(1);
    }

    // Crear socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    // Configurar dirección
    memset(&broker_addr, 0, sizeof(broker_addr));
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(BROKER_PORT);
    memcpy(&broker_addr.sin_addr.s_addr,
           server->h_addr,
           server->h_length);

    // Conectar
    if (connect(sock, (struct sockaddr *)&broker_addr, sizeof(broker_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Subscriber conectado al broker\n");
    fflush(stdout);

    // Suscribirse
    snprintf(buffer, BUFFER_SIZE, "SUBSCRIBE %s\n", TOPIC);
    write(sock, buffer, strlen(buffer));

    printf("Suscrito al tópico: %s\n", TOPIC);
    fflush(stdout);

    // Escuchar mensajes
    while (1) {
        int bytes = read(sock, buffer, BUFFER_SIZE - 1);
        if (bytes <= 0) break;

        buffer[bytes] = '\0';
        printf(">> %s", buffer);
        fflush(stdout);
    }

    close(sock);
    return 0;
}
