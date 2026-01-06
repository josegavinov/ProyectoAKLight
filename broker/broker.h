#ifndef BROKER_H
#define BROKER_H

#include <netinet/in.h>

#define MAX_CLIENTS 10
#define PORT 1883

typedef struct {
    int socket;
    char topic[128];
} subscriber_t;

#endif
