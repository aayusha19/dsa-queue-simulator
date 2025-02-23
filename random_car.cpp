#include <cstdlib> // For rand() and srand()
#include <ctime>   // For seeding random numbers

// Generate random arrival times between 1 and 30 seconds
int getRandomTime() {
    return rand() % 30 + 1;
}
