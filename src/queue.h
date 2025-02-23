#ifndef QUEUE_H
#define QUEUE_H

#define MAX_QUEUE_SIZE 10

typedef struct Queue {
    int arr[MAX_QUEUE_SIZE];
    int front;
    int rear;
} Queue;

void initQueue(Queue *q);
int isEmpty(Queue *q);
int isFull(Queue *q);
void enqueue(Queue *q, int vehicle);
int dequeue(Queue *q);

#endif