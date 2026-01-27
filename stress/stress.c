#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <curl/curl.h>

#include "../common/protocol.h"

#define BROKER_HOST "broker"
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
    srand(time(NULL));
    printf("Stress test con WhatsApp iniciando...\n");
    send_whatsapp_alert("🚨 Prueba AKLight: mensaje desde stress.c");
    return 0;
}