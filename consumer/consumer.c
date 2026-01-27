#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "protocol.h"

#define BROKER_HOST "127.0.0.1"
#define BROKER_PORT 1883

int main() {
    int sock;
    struct sockaddr_in broker_addr;
    struct hostent *server;
    char buffer[BUFFER_SIZE];

    printf("Consumer AKLight iniciando...\n");
    fflush(stdout);

    /* Resolver DNS */
    server = gethostbyname(BROKER_HOST);
    if (!server) {
    fprintf(stderr, "No se pudo resolver host: %s\n", BROKER_HOST);
    exit(1);
    }

    /* Crear socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    /* Configurar dirección */
    memset(&broker_addr, 0, sizeof(broker_addr));
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(BROKER_PORT);
    memcpy(&broker_addr.sin_addr.s_addr,
           server->h_addr,
           server->h_length);

    /* Conectar */
    if (connect(sock, (struct sockaddr *)&broker_addr, sizeof(broker_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Consumer conectado al broker\n");
    fflush(stdout);

    /* ================= SUSCRIPCIONES ================= */

    /* Métricas (no persistente) */
    snprintf(buffer, BUFFER_SIZE, "SUBSCRIBE stress/# NP\n");
    write(sock, buffer, strlen(buffer));
    printf("Suscrito a stress/# (NP)\n");

    /* Alertas (persistente) */
    snprintf(buffer, BUFFER_SIZE, "SUBSCRIBE alertas/# P\n");
    write(sock, buffer, strlen(buffer));
    printf("Suscrito a alertas/# (P)\n");

    fflush(stdout);

    /* ================= RECEPCIÓN ================= */

    while (1) {
        int bytes = read(sock, buffer, BUFFER_SIZE - 1);
        if (bytes <= 0)
            break;

        buffer[bytes] = '\0';

        if (strstr(buffer, "alertas/")) {
            printf("🚨 ALERTA >> %s", buffer);
        } else {
            printf("📊 METRICA >> %s", buffer);
        }

        fflush(stdout);
    }

    close(sock);
    return 0;
}