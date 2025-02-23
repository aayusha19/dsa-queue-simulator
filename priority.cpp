#include <queue>
#include "car.cpp"

// Custom comparator for priority queue
struct CompareSpeed {
    bool operator()(SmallCar const& c1, SmallCar const& c2) {
        return c1.getSpeed() < c2.getSpeed(); // Higher speed gets priority
    }
};

// Usage: std::priority_queue<SmallCar, std::vector<SmallCar>, CompareSpeed> carQueue;
