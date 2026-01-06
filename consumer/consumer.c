#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "../common/protocol.h"

#define BROKER_IP "127.0.0.1"
#define BROKER_PORT 1883

int main() {
    int sock;
    struct sockaddr_in broker_addr;
    char buffer[BUFFER_SIZE];
    char topic[MAX_TOPIC];

    // 1. Crear socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    // 2. Configurar broker
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(BROKER_PORT);
    inet_pton(AF_INET, BROKER_IP, &broker_addr.sin_addr);

    // 3. Conectarse al broker
    if (connect(sock, (struct sockaddr *)&broker_addr, sizeof(broker_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Consumidor conectado al broker\n");

    // 4. Elegir tópico
    printf("Ingrese el tópico a suscribirse: ");
    scanf("%s", topic);

    // 5. Enviar SUBSCRIBE
    snprintf(buffer, BUFFER_SIZE, "SUBSCRIBE %s\n", topic);
    write(sock, buffer, strlen(buffer));

    printf("Suscrito a %s\n\n", topic);

    // 6. Recibir mensajes
    while (1) {
        int bytes = read(sock, buffer, BUFFER_SIZE);
        if (bytes <= 0) {
            printf("Conexión cerrada por el broker\n");
            break;
        }

        buffer[bytes] = '\0';
        printf(">> %s", buffer);
    }

    close(sock);
    return 0;
}
