#ifndef MESSAGE_H
#define MESSAGE_H

#include <time.h>

#define MAX_TOPIC_LEN   128
#define MAX_PAYLOAD_LEN 256

typedef struct {
    char topic[MAX_TOPIC_LEN];     // tópico multinivel
    char payload[MAX_PAYLOAD_LEN]; // valor de la métrica
    time_t timestamp;              // tiempo de llegada
} Message;

#endif