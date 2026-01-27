#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <curl/curl.h>

#include "../common/protocol.h"

#define BROKER_HOST "127.0.0.1"
#define BROKER_PORT 1883

#define ALERT_TOPIC "alertas/whatsapp"
#define STRESS_TOPIC "stress/carga"
#define CPU_THRESHOLD 85.0

/* =========================
   TWILIO WHATSAPP REAL
   ========================= */

void send_whatsapp_alert(const char *msg) {
    const char *sid   = getenv("TWILIO_ACCOUNT_SID");
    const char *token = getenv("TWILIO_AUTH_TOKEN");
    const char *from  = getenv("TWILIO_FROM");
    const char *to    = getenv("TWILIO_TO");

    if (!sid || !token || !from || !to) {
        printf("[TWILIO] Variables de entorno incompletas\n");
        return;
    }

    CURL *curl = curl_easy_init();
    if (!curl) return;

    char url[256];
    snprintf(url, sizeof(url),
             "https://api.twilio.com/2010-04-01/Accounts/%s/Messages.json",
             sid);

    /* URL-encode de TODOS los campos */
    char *esc_from = curl_easy_escape(curl, from, 0);
    char *esc_to   = curl_easy_escape(curl, to, 0);
    char *esc_body = curl_easy_escape(curl, msg, 0);

    char data[1024];
    snprintf(data, sizeof(data),
             "From=%s&To=%s&Body=%s",
             esc_from, esc_to, esc_body);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    curl_easy_setopt(curl, CURLOPT_USERNAME, sid);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, token);

    CURLcode res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code == 201) {
            printf("[TWILIO] WhatsApp enviado correctamente\n");
        } else {
            printf("[TWILIO] Falló envío (HTTP %ld)\n", http_code);
        }
    } else {
        printf("[TWILIO] Error CURL: %s\n", curl_easy_strerror(res));
    }

    curl_free(esc_from);
    curl_free(esc_to);
    curl_free(esc_body);
    curl_easy_cleanup(curl);
}

/* =========================
   MÉTRICA SIMULADA
   ========================= */

float simulate_cpu_usage() {
    return rand() % 101;
}

/* =========================
   MAIN
   ========================= */

int main() {
    int sock;
    struct sockaddr_in broker_addr;
    struct hostent *server;
    char buffer[BUFFER_SIZE];

    srand(time(NULL));

    printf("Stress test AKLight iniciando...\n");

    /* Conexión al broker */
    server = gethostbyname(BROKER_HOST);
    if (!server) {
        fprintf(stderr, "No se pudo resolver host: %s\n", BROKER_HOST);
        exit(1);
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    memset(&broker_addr, 0, sizeof(broker_addr));
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(BROKER_PORT);
    memcpy(&broker_addr.sin_addr.s_addr,
           server->h_addr,
           server->h_length);

    while (connect(sock, (struct sockaddr *)&broker_addr, sizeof(broker_addr)) < 0) {
        printf("Esperando broker...\n");
        sleep(1);
    }

    printf("Stress conectado al broker\n");

    /* Loop de estrés */
    while (1) {
        float cpu = simulate_cpu_usage();

        /* Publicar carga */
        snprintf(buffer, BUFFER_SIZE,
                 "PUBLISH %s CPU=%.2f%%\n",
                 STRESS_TOPIC, cpu);
        write(sock, buffer, strlen(buffer));

        printf("[STRESS] CPU=%.2f%%\n", cpu);

        /* Alerta crítica */
        if (cpu >= CPU_THRESHOLD) {
            char alert[256];
            snprintf(alert, sizeof(alert),
                     "🚨 ALERTA CRÍTICA AKLight: CPU %.2f%%", cpu);

            /* WhatsApp */
            send_whatsapp_alert(alert);

            /* Publicar alerta */
            snprintf(buffer, BUFFER_SIZE,
                     "PUBLISH %s %s\n",
                     ALERT_TOPIC, alert);
            write(sock, buffer, strlen(buffer));
        }

        usleep(500000); // 0.5s
    }

    close(sock);
    return 0;
}