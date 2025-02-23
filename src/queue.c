#include <stdio.h>
#include "queue.h"

void initQueue(Queue *q) {
    q->front = -1;
    q->rear = -1;
}

int isEmpty(Queue *q) {
    return (q->front == -1);
}

int isFull(Queue *q) {
    return (q->rear == MAX_QUEUE_SIZE - 1);
}

void enqueue(Queue *q, int vehicle) {
    if (isFull(q)) {
        printf("Queue is full! Cannot enqueue vehicle %d\n", vehicle);
        return;
    }
    if (q->front == -1) {
        q->front = 0;
    }
    q->rear++;
    q->arr[q->rear] = vehicle;
    printf("Enqueued vehicle %d\n", vehicle);
}

int dequeue(Queue *q) {
    if (isEmpty(q)) {
        printf("Queue is empty! Cannot dequeue vehicle\n");
        return -1;  // Return an error value
    }
    int vehicle = q->arr[q->front];
    q->front++;
    if (q->front > q->rear) {  // Reset the queue if it's empty after dequeue
        q->front = q->rear = -1;
    }
    return vehicle;
}
