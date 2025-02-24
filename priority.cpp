#include <queue>
#include "car.cpp"


struct CompareSpeed {
    bool operator()(SmallCar const& c1, SmallCar const& c2) {
        return c1.getSpeed() < c2.getSpeed(); // Higher speed gets priority
    }
};


