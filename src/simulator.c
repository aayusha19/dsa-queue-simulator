#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "queue.h"

#define MAX_VEHICLES 5

// Traffic light states
typedef enum { RED, GREEN, YELLOW } TrafficLightState;
TrafficLightState trafficLight = RED;

// Function to simulate traffic light cycling
void cycleTrafficLight() {
    switch (trafficLight) {
        case RED:
            printf("Traffic Light: RED\n");
            trafficLight = GREEN;
            break;
        case GREEN:
            printf("Traffic Light: GREEN\n");
            trafficLight = YELLOW;
            break;
        case YELLOW:
            printf("Traffic Light: YELLOW\n");
            trafficLight = RED;
            break;
    }
}

// Function to process vehicles at the traffic junction
void processVehicles(Queue* q) {
    while (!isEmpty(q)) {
        int vehicle = dequeue(q);
        printf("Processing vehicle: %d\n", vehicle);
    }
}

int main() {
    Queue q;
    initQueue(&q);

    // Generate some traffic
    for (int i = 1; i <= MAX_VEHICLES; i++) {
        enqueue(&q, i);
        printf("Enqueued vehicle %d\n", i);
    }

    // Start the simulation
    printf("Traffic Simulator Started!\n");

    // Simulate traffic light cycles and process vehicles
    for (int i = 0; i < 10; i++) {
        cycleTrafficLight();
        if (trafficLight == GREEN) {
            processVehicles(&q);
        }
        sleep(1); // wait for 1 second before next cycle
    }

    return 0;
}
