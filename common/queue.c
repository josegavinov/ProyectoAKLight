#include <stdlib.h>
#include <string.h>
#include "queue.h"

void queue_init(MessageQueue *q) {
    q->head = NULL;
    q->tail = NULL;
    pthread_mutex_init(&q->mutex, NULL);
}

void queue_push(MessageQueue *q, Message *msg) {
    QueueNode *node = (QueueNode *)malloc(sizeof(QueueNode));
    if (!node) return;

    node->msg = *msg;
    node->next = NULL;

    pthread_mutex_lock(&q->mutex);

    if (q->tail == NULL) {
        q->head = node;
        q->tail = node;
    } else {
        q->tail->next = node;
        q->tail = node;
    }

    pthread_mutex_unlock(&q->mutex);
}

int queue_pop(MessageQueue *q, Message *out) {
    pthread_mutex_lock(&q->mutex);

    if (q->head == NULL) {
        pthread_mutex_unlock(&q->mutex);
        return 0;
    }

    QueueNode *node = q->head;
    *out = node->msg;

    q->head = node->next;
    if (q->head == NULL)
        q->tail = NULL;

    pthread_mutex_unlock(&q->mutex);

    free(node);
    return 1;
}

void queue_destroy(MessageQueue *q) {
    Message tmp;
    while (queue_pop(q, &tmp));
    pthread_mutex_destroy(&q->mutex);
}