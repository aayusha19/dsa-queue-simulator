#include "queue.h"

// Create an empty queue
Queue* createQueue() {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    q->front = q->rear = NULL;
    q->size = 0;
    return q;
}

// Enqueue a vehicle into the queue
void enqueue(Queue* q, int id, int lane) {
    Vehicle* newVehicle = (Vehicle*)malloc(sizeof(Vehicle));
    newVehicle->id = id;
    newVehicle->lane = lane;
    newVehicle->next = NULL;

    if (q->rear == NULL) {
        q->front = q->rear = newVehicle;
    } else {
        q->rear->next = newVehicle;
        q->rear = newVehicle;
    }
    q->size++;
}

// Dequeue a vehicle from the queue
Vehicle* dequeue(Queue* q) {
    if (q->front == NULL) return NULL;

    Vehicle* temp = q->front;
    q->front = q->front->next;

    if (q->front == NULL) q->rear = NULL;
    q->size--;

    return temp;
}

// Check if the queue is empty
int isQueueEmpty(Queue* q) {
    return q->front == NULL;
}

// Display the queue (for debugging)
void displayQueue(Queue* q) {
    Vehicle* temp = q->front;
    while (temp) {
        printf("Vehicle %d in lane %d -> ", temp->id, temp->lane);
        temp = temp->next;
    }
    printf("NULL\n");
}
