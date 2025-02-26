#ifndef CAR_H
#define CAR_H

#include <queue>
#include <string>

struct Car {
    int id;
    std::string lane;

    Car(int i, std::string l) : id(i), lane(l) {}
};

// Function declarations
std::string getRandomLane();
void generateCars(std::queue<Car>& leftLane, std::queue<Car>& rightLane, std::queue<Car>& straightLane, int numCars);
void processCars(std::queue<Car>& leftLane, std::queue<Car>& rightLane, std::queue<Car>& straightLane);

#endif
