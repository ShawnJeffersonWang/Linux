#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

const int num_philosophers = 5;
std::mutex forks[num_philosophers];

void philosopher(int id) {
    int left = id;
    int right = (id + 1) % num_philosophers;

    while (true) {
        // 拿起左边的筷子
        forks[left].lock();
        std::cout << "Philosopher " << id << " picks up fork " << left
                  << std::endl;

        // 尝试拿起右边的筷子，如果失败则放下左边的筷子
        if (forks[right].try_lock()) {
            std::cout << "Philosopher " << id << " picks up fork " << right
                      << std::endl;

            // 取到两只筷子之后开始进餐
            std::cout << "Philosopher " << id << " is eating" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));

            // 放下右边的筷子
            forks[right].unlock();
            std::cout << "Philosopher " << id << " puts down fork " << right
                      << std::endl;
        }

        // 放下左边的筷子
        forks[left].unlock();
        std::cout << "Philosopher " << id << " puts down fork " << left
                  << std::endl;

        // 思考一段时间
        std::cout << "Philosopher " << id << " is thinking" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }
}

int main() {
    std::thread philosophers[num_philosophers];
    for (int i = 0; i < num_philosophers; i++) {
        philosophers[i] = std::thread(philosopher, i);
    }

    for (int i = 0; i < num_philosophers; i++) {
        philosophers[i].join();
    }

    return 0;
}

/*另一种解决哲学家问题的方案是使用资源分级算法（Hierarchical Resource Allocation
Algorithm）。这种算法将哲学家分为两组，一组是偶数号哲学家，另一组是奇数号哲学家。每组哲学家共享一组筷子，而且偶数号哲学家必须先获得它们所在组的筷子才能获得奇数号哲学家组的筷子，反之亦然。

下面是一个使用资源分级算法解决哲学家就餐问题的例子代码：*/
const int num_philosophers = 5;
std::mutex forks[num_philosophers];

void philosopher(int id) {
    while (true) {
        std::cout << "Philosopher " << id << " is thinking" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));

        if (id % 2 == 0) {
            // 偶数号哲学家先请求偶数号筷子，再请求奇数号筷子
            forks[id].lock();
            forks[(id + 1) % num_philosophers].lock();
        } else {
            // 奇数号哲学家先请求奇数号筷子，再请求偶数号筷子
            forks[(id + 1) % num_philosophers].lock();
            forks[id].lock();
        }

        std::cout << "Philosopher " << id << " picks up forks " << id << " and "
                  << (id + 1) % num_philosophers << std::endl;
        std::cout << "Philosopher " << id << " is eating" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));

        forks[id].unlock();
        forks[(id + 1) % num_philosophers].unlock();

        std::cout << "Philosopher " << id << " finishes eating" << std::endl;
    }
}

int main() {
    std::thread philosophers[num_philosophers];
    for (int i = 0; i < num_philosophers; i++) {
        philosophers[i] = std::thread(philosopher, i);
    }

    for (int i = 0; i < num_philosophers; i++) {
        philosophers[i].join();
    }

    return 0;
}
/*在这个例子中，偶数号哲学家和奇数号哲学家被分为两组，它们共享一组筷子。当一个哲学家需要进餐时，它会先请求它所在组的筷子，然后再请求另一组的筷子。这种算法保证了哲学家之间的互斥，因为偶数号哲学家只会请求偶数号筷子，而奇数号哲学家只会请求奇数号筷子，因此不可能出现多个哲学家同时请求同一根筷子的情况。同时，这种算法也不会导致死锁，因为每个哲学家只需要同时获得两根不同的筷子才能进餐，且不需要等待其他哲学家释放它们所需要的筷子。

总的来说，资源分级算法是一种有效的解决哲学家问题的方案，它通过将哲学家分为两组，共享一组筷子，并按照一定的顺序请求筷子来保证哲学家之间的互斥和协作。*/

/*另一种解决哲学家问题的方案是使用 Chandy/Misra
算法。这种算法是基于消息传递的，每个哲学家都有一个状态，表示它是否正在进餐或者等待进餐，以及它需要哪些筷子。当一个哲学家需要进餐时，它会向它的左右两个哲学家发送一个请求消息，请求它们能否将对应的筷子传递给它。当一个哲学家收到一个请求消息时，它会检查自己是否正在使用对应的筷子，如果没有，它会将筷子传递给请求者，并将自己的状态设置为等待状态。当一个哲学家完成进餐并放下筷子时，它会向它的左右两个哲学家发送一个释放消息，通知它们可以将对应的筷子传递给下一个等待者。

下面是一个使用 Chandy/Misra 算法解决哲学家就餐问题的例子代码：*/

const int num_philosophers = 5;
std::mutex forks[num_philosophers];

enum class philosopher_state { thinking, waiting, eating };

philosopher_state states[num_philosophers];

void request_forks(int id) {
    // 向左右两个哲学家发送请求消息
    int left = id;
    int right = (id + 1) % num_philosophers;
    states[id] = philosopher_state::waiting;
    forks[left].lock();
    forks[right].lock();
}

void release_forks(int id) {
    // 放下左右两边的筷子，并通知左右两个哲学家
    int left = id;
    int right = (id + 1) % num_philosophers;
    forks[left].unlock();
    forks[right].unlock();
    states[id] = philosopher_state::thinking;
}

void philosopher(int id) {
    while (true) {
        std::cout << "Philosopher " << id << " is thinking" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));

        request_forks(id);  // 请求左右两边的筷子

        std::cout << "Philosopher " << id << " picks up forks " << id << " and "
                  << (id + 1) % num_philosophers << std::endl;
        std::cout << "Philosopher " << id << " is eating" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));

        release_forks(id);  // 放下左右两边的筷子

        std::cout << "Philosopher " << id << " finishes eating" << std::endl;
    }
}

int main() {
    std::thread philosophers[num_philosophers];
    for (int i = 0; i < num_philosophers; i++) {
        philosophers[i] = std::thread(philosopher, i);
        states[i] = philosopher_state::thinking;
    }

    for (int i = 0; i < num_philosophers; i++) {
        philosophers[i].join();
    }

    return 0;
}

/*在这个例子中，每个哲学家都有一个状态，表示它是否正在进餐或者等待进餐，以及它需要哪些筷子。当一个哲学家需要进餐时，它会向它的左右两个哲学家发送一个请求消息，请求它们能否将对应的筷子传递给它。当一个哲学家收到一个请求消息时，它会检查自己是否正在使用对应的筷子，如果没有，它会将筷子传递给请求者，并将自己的状态设置为等待状态。当一个哲学家完成进餐并放下筷子时，它会向它的左右两个哲学家发送一个释放消息，通知它们可以将对应的筷子传递给下一个等待者。这种算法也不会导致死锁，因为每个哲学家只有在拥有左右两边的筷子时才能进餐，而当一个哲学家请求不到它需要的筷子时，它会一直等待直到可以获得所有需要的筷子。同时，当一个哲学家释放它的筷子时，它会通知它的左右两个哲学家，让它们可以获得对应的筷子。

总的来说，Chandy/Misra
算法是一种有效的解决哲学家问题的方案，它使用消息传递和状态同步来保证哲学家之间的互斥和协作。*/

/*这段代码实现了哲学家就餐问题的一个经典解法，也称为资源分级算法。在这个算法中，每个哲学家都需要拿起它左边和右边的筷子才能进餐，当一个哲学家拿起了一只筷子但是另一只筷子被其他哲学家占用时，它需要放下这只筷子并等待。

这种算法的一个问题是可能会出现死锁，即所有哲学家都拿起了它们左边的筷子而右边的筷子都被占用了，从而所有哲学家都无法进餐或放下筷子。为了避免这种情况，我们可以引入一个额外的资源，例如一个服务生，它可以控制所有筷子的使用，从而避免死锁。下面是一个使用服务生解决哲学家就餐问题的例子代码：*/
const int num_philosophers = 5;
std::mutex forks[num_philosophers];
std::mutex waiter;

void philosopher(int id) {
    int left = id;
    int right = (id + 1) % num_philosophers;

    while (true) {
        // 等待服务生允许取筷子
        waiter.lock();

        // 拿起左边的筷子
        forks[left].lock();
        std::cout << "Philosopher " << id << " picks up fork " << left
                  << std::endl;

        // 尝试拿起右边的筷子，如果失败则放下左边的筷子
        if (forks[right].try_lock()) {
            std::cout << "Philosopher " << id << " picks up fork " << right
                      << std::endl;

            // 取到两只筷子之后开始进餐
            std::cout << "Philosopher " << id << " is eating" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));

            // 放下右边的筷子
            forks[right].unlock();
            std::cout << "Philosopher " << id << " puts down fork " << right
                      << std::endl;
        }

        // 放下左边的筷子
        forks[left].unlock();
        std::cout << "Philosopher " << id << " puts down fork " << left
                  << std::endl;

        // 放下服务生允许取筷子
        waiter.unlock();

        // 思考一段时间
        std::cout << "Philosopher " << id << " is thinking" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }
}

int main() {
    std::thread philosophers[num_philosophers];
    for (int i = 0; i < num_philosophers; i++) {
        philosophers[i] = std::thread(philosopher, i);
    }

    for (int i = 0; i < num_philosophers; i++) {
        philosophers[i].join();
    }

    return 0;
}

/*在这个例子中，我们引入了一个额外的互斥量
 * waiter，它用于控制哲学家拿起筷子的时机。当一个哲学家想要拿起筷子时，它需要先等待服务生允许取筷子，然后再执行和原来一样的拿起筷子、进餐、放下筷子等操作。当哲学家放下筷子后，它需要释放
 * waiter
 * 互斥量，表示它已经完成了一次就餐。这样做可以避免死锁，因为服务生可以控制哲学家拿起筷子的时机，从而避免所有哲学家都拿起了它们左边的筷子而右边的筷子都被占用的情况。*/