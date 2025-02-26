#include <iostream>
#include <queue>
#include <cstdlib>  // For rand()
#include <ctime>    // For time()

using namespace std;

// Car structure
struct Car {
    int id;
    string lane; // Left, Right, or Straight

    Car(int i, string l) : id(i), lane(l) {}
};

// Function to generate random lane
string getRandomLane() {
    int laneChoice = rand() % 3; // 0: Left, 1: Right, 2: Straight
    if (laneChoice == 0) return "Left";
    if (laneChoice == 1) return "Right";
    return "Straight";
}

// Function to generate cars and add them to the respective lanes
void generateCars(queue<Car> &leftLane, queue<Car> &rightLane, queue<Car> &straightLane, int numCars) {
    for (int i = 1; i <= numCars; i++) {
        string lane = getRandomLane();
        Car newCar(i, lane);

        if (lane == "Left") leftLane.push(newCar);
        else if (lane == "Right") rightLane.push(newCar);
        else straightLane.push(newCar);

        cout << "🚗 Car " << i << " added to " << lane << " lane.\n";
    }
}

// Function to process and move cars
void processCars(queue<Car> &leftLane, queue<Car> &rightLane, queue<Car> &straightLane) {
    cout << "\n🚦 Processing Cars at the Junction 🚦\n";

    while (!leftLane.empty() || !rightLane.empty() || !straightLane.empty()) {
        if (!leftLane.empty()) {
            Car c = leftLane.front();
            leftLane.pop();
            cout << "🚗 Car " << c.id << " from LEFT lane is moving.\n";
        }

        if (!rightLane.empty()) {
            Car c = rightLane.front();
            rightLane.pop();
            cout << "🚕 Car " << c.id << " from RIGHT lane is moving.\n";
        }

        if (!straightLane.empty()) {
            Car c = straightLane.front();
            straightLane.pop();
            cout << "🚙 Car " << c.id << " from STRAIGHT lane is moving.\n";
        }

        cout << "-----------------------------------\n";
    }
}

int main() {
    srand(time(0)); // Seed random number generator

    queue<Car> leftLane;
    queue<Car> rightLane;
    queue<Car> straightLane;

    int numCars = 5; // Change this to add more cars
    generateCars(leftLane, rightLane, straightLane, numCars);

    // Process and move the cars
    processCars(leftLane, rightLane, straightLane);

    return 0;
}
