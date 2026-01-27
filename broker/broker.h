#ifndef BROKER_H
#define BROKER_H

#include <netinet/in.h>
#include "../common/protocol.h"
#include "../common/message.h"
#include "../common/queue.h"

#define MAX_CLIENTS 10
#define MAX_TOPICS  10

typedef struct {
int socket;
char topic[MAX_TOPIC];
int persistent; // 1 = persistente, 0 = no persistente
} subscriber_t;

typedef struct {
    char name[MAX_TOPIC];
    MessageQueue queue;
} Topic;

// Variables globales (definidas en broker.c)
extern Topic topics[MAX_TOPICS];
extern int topic_count;

#endif