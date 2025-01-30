#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>
#include <stdlib.h>

typedef struct Vehicle {
    int id;
    int lane;
    struct Vehicle* next;
} Vehicle;

typedef struct Queue {
    Vehicle* front;
    Vehicle* rear;
    int size;
} Queue;

Queue* createQueue();
void enqueue(Queue* q, int id, int lane);
Vehicle* dequeue(Queue* q);
int isQueueEmpty(Queue* q);
void displayQueue(Queue* q);

#endif
