#include "queue.h"
#include <stdio.h>

void generateTraffic(Queue *q) {
    
    enqueue(q, 1);
    enqueue(q, 2);
    enqueue(q, 3);
    enqueue(q, 4);
    printf("Traffic generated!\n");
}
