#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "../common/protocol.h"

#define BROKER_IP "broker"
#define BROKER_PORT 1883

// Obtener uso de memoria desde /proc/meminfo
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

// Obtener uso de disco usando statvfs
float get_disk_usage() {
    struct statvfs stat;
    if (statvfs("/", &stat) != 0)
        return -1;

    float total = stat.f_blocks * stat.f_frsize;
    float free = stat.f_bfree * stat.f_frsize;

    return ((total - free) / total) * 100.0;
}

int main() {
    int sock;
    struct sockaddr_in broker_addr;
    char buffer[BUFFER_SIZE];

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

    printf("Productor conectado al broker\n");

    while (1) {
        float mem = get_memory_usage();
        float disk = get_disk_usage();

        // Publicar memoria
        snprintf(buffer, BUFFER_SIZE,
                 "PUBLISH sistema/memoria/uso %.2f%%\n", mem);
        write(sock, buffer, strlen(buffer));

        // Publicar disco
        snprintf(buffer, BUFFER_SIZE,
                 "PUBLISH sistema/disco/uso %.2f%%\n", disk);
        write(sock, buffer, strlen(buffer));

        sleep(5); // cada 5 segundos
    }

    close(sock);
    return 0;
}
