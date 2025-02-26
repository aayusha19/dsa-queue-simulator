#include <iostream>
#include <queue>
#include "car_simulation.h"

using namespace std;

int main() {
    // Initialize random number generator
    srand(time(0));  

    // Create queues for each lane
    queue<Car> leftLane;
    queue<Car> rightLane;
    queue<Car> straightLane;

    // Number of cars to generate for the simulation
    int numCars = 5;  // Adjust this number as needed
    generateCars(leftLane, rightLane, straightLane, numCars); // Generate random cars

    // Process and move the cars from the lanes
    processCars(leftLane, rightLane, straightLane);

    return 0;
}
