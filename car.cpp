#include <iostream>
#include <queue>

class SmallCar {
private:
    int carID;
    int arrivalTime;  // Time when the car reaches the junction

public:
    // Constructor
    SmallCar(int id, int time) : carID(id), arrivalTime(time) {}

    // Getter functions
    int getID() const { return carID; }
    int getArrivalTime() const { return arrivalTime; }

    // Display function
    void display() const {
        std::cout << "Small Car ID: " << carID 
                  << ", Arrival Time: " << arrivalTime << " sec\n";
    }

    private:
    int speed; // in km/h
public:
    SmallCar(int id, int time, int spd) : carID(id), arrivalTime(time), speed(spd) {}

    int getSpeed() const { return speed; }

};


int main() {
    std::queue<SmallCar> carQueue;

    // Adding cars to the queue
    carQueue.push(SmallCar(1, 5));
    carQueue.push(SmallCar(2, 10));
    carQueue.push(SmallCar(3, 15));

    // Processing cars in the queue
    while (!carQueue.empty()) {
        SmallCar car = carQueue.front();
        car.display();
        carQueue.pop();  // Remove the processed car
    }

    return 0;
}
