#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/statvfs.h>
#include "protocol.h"

#define BROKER_HOST "broker"
#define BROKER_PORT 1883

// ========================================
// MÉTRICAS DEL SISTEMA
// ========================================

float get_memory_usage() {
    FILE *file = fopen("/proc/meminfo", "r");
    if (!file) return -1;

    char line[256];
    float total = 0, free = 0;

    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "MemTotal: %f kB", &total) == 1);
        if (sscanf(line, "MemAvailable: %f kB", &free) == 1);
    }

    fclose(file);
    return ((total - free) / total) * 100.0;
}

float get_disk_usage() {
    struct statvfs stat;
    if (statvfs("/", &stat) != 0)
        return -1;

    float total = stat.f_blocks * stat.f_frsize;
    float free = stat.f_bfree * stat.f_frsize;

    return ((total - free) / total) * 100.0;
}

// ========================================
// ESPERA ACTIVA PARA DNS DEL BROKER
// ========================================

struct hostent* wait_for_broker(const char* host) {
    struct hostent* server = NULL;

    while (server == NULL) {
        server = gethostbyname(host);
        if (server == NULL) {
            printf("Esperando resolución DNS del broker...\n");
            fflush(stdout);
            sleep(1);
        }
    }

    return server;
}

// ========================================
// FUNCIÓN PRINCIPAL
// ========================================

int main() {
    int sock;
    struct sockaddr_in broker_addr;
    struct hostent *server;
    char buffer[BUFFER_SIZE];

    printf("Productor iniciando...\n");
    fflush(stdout);

    // Esperar a que el broker esté disponible en DNS
    server = wait_for_broker(BROKER_HOST);

    printf("Broker resuelto por DNS\n");
    fflush(stdout);

    // Crear socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    // Configurar dirección del broker
    memset(&broker_addr, 0, sizeof(broker_addr));
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(BROKER_PORT);
    memcpy(&broker_addr.sin_addr.s_addr,
           server->h_addr,
           server->h_length);

    // Intentar conectar (reintento controlado)
    while (connect(sock, (struct sockaddr *)&broker_addr, sizeof(broker_addr)) < 0) {
        printf("Esperando que el broker acepte conexiones...\n");
        fflush(stdout);
        sleep(1);
    }

    printf("Productor conectado al broker\n");
    fflush(stdout);

    // Loop de publicación
    while (1) {
        float mem = get_memory_usage();
        float disk = get_disk_usage();

        snprintf(buffer, BUFFER_SIZE,
                 "PUBLISH sistema/memoria/uso %.2f%%\n", mem);
        write(sock, buffer, strlen(buffer));

        snprintf(buffer, BUFFER_SIZE,
                 "PUBLISH sistema/disco/uso %.2f%%\n", disk);
        write(sock, buffer, strlen(buffer));

        sleep(5);
    }

    close(sock);
    return 0;
}
