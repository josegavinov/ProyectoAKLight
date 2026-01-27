#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include "message.h"

typedef struct QueueNode {
    Message msg;
    struct QueueNode *next;
} QueueNode;

typedef struct {
    QueueNode *head;
    QueueNode *tail;
    pthread_mutex_t mutex;
} MessageQueue;

// Inicializa la cola
void queue_init(MessageQueue *q);

// Inserta un mensaje (enqueue)
void queue_push(MessageQueue *q, Message *msg);

// Extrae un mensaje (dequeue)
// Retorna 1 si hay mensaje, 0 si está vacía
int queue_pop(MessageQueue *q, Message *out);

// Libera memoria
void queue_destroy(MessageQueue *q);

#endif