#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

const int num_philosophers = 5;
std::mutex forks[num_philosophers];

void philosophers(int id) {
    int left = id;
    int right = (id + 1) % num_philosophers;

    while (true) {
        forks[left].lock();
        std::cout << "Philosophers " << id << " picks up fork " << left
                  << std::endl;

        if (forks[right].try_lock()) {
            std::cout << "Philosophers " << id << " picks up fork " << right
                      << std::endl;

            std::cout << "Philosopher " << id << " is eatind " << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            forks[right].unlock();

            std::cout << "Philosophers " << id << " puts down fork " << right
                      << std::endl;
        }

        forks[left].unlock();
        std::cout << "Philosophers " << id << " puts down fork " << left
                  << std::endl;

        std::cout << "Philosophers " << id << " is thinking " << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }
}

int main() {
    std::thread philosophers[num_philosophers];
    for (int i = 0; i < num_philosophers; i++) {
        philosophers[i] = std::thread(philosophers, i);
    }

    for (int i = 0; i < num_philosophers; i++) {
        philosophers[i].join();
    }

    return 0;
}
