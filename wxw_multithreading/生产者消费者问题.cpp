#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>

// Single-producer , single-consumer Queue
template <class T>
class SPSCQueue {
   public:
    explicit SPSCQueue(int capacity)
        : capacity_(capacity), size_(0), front_(0), rear_(0) {
        data_.reset(new T[capacity_]);  // 将data智能指针指向这个数组
    }
    bool Push(std::unique_ptr<T> item) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_full_.wait([this]() { return size_ < capacity_; });
        data_[rear_] = std::move(*item);
        rear_ = (rear_ + 1) % capacity_;
        size_++;
        not_empty_.notify_one();
        return true;
    }
    std::unique_ptr<T> pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait([this]() { return size_ > 0; });
        std::unique_ptr<T> item(new T(std::move(data[front_])));
        front_ = (front_ + 1) % capacity_;
        size_--;
        not_full_.notify_one();
        return item;
    }
    virtual ~SPSCQueue() = 0;

   private:
    std::unique_ptr<T[]> data_;
    int capacity_;
    int size_;
    int front_;
    int rear_;
    std::mutex mutex_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
};

// Multi-producer , Multi-consumer Queue
template <class T>
class MPMCQueue {
   public:
    explicit MPMCQueue(int capacity)
        : capacity_(capacity),
          buffer_(capacity),
          read_index_(0),
          write_index_(0),
          count_(0) {}
    bool Push(std::unique_ptr<T> item) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_full_.wait([this] { count_ < capacity_; });
        buffer_[write_index_] = std::move(item);
        write_index_ = (write_index_ + 1) % capacity_;
        count_++;
        not_empty_.notify_one();
        return true;
    }
    std::unique_ptr<T> pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait([this]() { count_ > 0 });
        std::unique_ptr<T> item = std::move(buffer_[read_index_]);
        read_index_ = (read_index_ + 1) % capacity_;
        count_--;
        not_full_.notify_one;
        return item;
    }
    virtual ~MPMCQueue() = 0;

   private:
    const int capacity_;
    std::vector<std::unique_ptr<T>> buffer_;
    std::atomic<int> read_index_;
    std::atomic<int> write_index_;
    std::atomic<int> count_;
    std::mutex mutex_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
};

// #include <condition_variable>
// #include <memory>
// #include <mutex>

// template <class T>
// class SPSCQueue {
//    public:
//     explicit SPSCQueue(int capacity)
//         : capacity_(capacity), size_(0), front_(0), rear_(0) {
//         data_.reset(new T[capacity_]);
//     }

//     bool Push(std::unique_ptr<T> item) {
//         // 1.阻塞等待条件变量满足 2.释放已掌握的互斥锁在(解锁互斥量)

//         //
//         1,2两步为一个原子操作 3.当被唤醒,函数返回时,解除阻塞并重新获取互斥锁
//         std::unique_lock<std::mutex> lock(mutex_);
//         not_full_.wait(lock, [this]() { return size_ < capacity_; });  //
//         // 等待队列不满
//         data_[rear_] = std::move(*item);
//         rear_ = (rear_ + 1) % capacity_;
//         size_++;
//         not_empty_.notify_one();  // 唤醒等待队列不空的线程
//         return true;
//     }

//     std::unique_ptr<T> Pop() {
//         std::unique_lock<std::mutex> lock(mutex_);
//         not_empty_.wait(lock, [this]() { return size_ > 0; });
//         /*等待队列不空*/ std::unique_ptr<T> item(
//             new T(std::move(data_[front_])));
//         front_ = (front_ + 1) % capacity_;
//         size_--;
//         not_full_.notify_one();  // 唤醒等待队列不满的线程
//         return item;
//     }

//     virtual ~SPSCQueue() {}

//    private:
//     std::unique_ptr<T[]> data_;          // 循环数组
//     int capacity_;                       // 队列容量
//     int size_;                           // 队列大小
//     int front_;                          // 队列头指针
//     int rear_;                           // 队列尾指针
//     std::mutex mutex_;                   // 互斥锁
//     std::condition_variable not_full_;   // 队列不满条件变量
//     std::condition_variable not_empty_;  // 队列不空条件变量
// };

// #include <atomic>
// #include <condition_variable>
// #include <memory>
// #include <mutex>
// #include <vector>

// template <class T>
// class MPMCQueue {
//    public:
//     explicit MPMCQueue(int capacity)
//         : capacity_(capacity),
//           buffer_(capacity),
//           read_index_(0),
//           write_index_(0),
//           count_(0) {}

//     bool Push(std::unique_ptr<T> item) {
//         std::unique_lock<std::mutex> lock(mutex_);
//         not_full_.wait(lock, [this] { return count_ < capacity_; });

//         buffer_[write_index_] = std::move(item);
//         write_index_ = (write_index_ + 1) % capacity_;
//         ++count_;

//         not_empty_.notify_one();
//         return true;
//     }

//     std::unique_ptr<T> Pop() {
//         std::unique_lock<std::mutex> lock(mutex_);
//         not_empty_.wait(lock, [this] { return count_ > 0; });

//         std::unique_ptr<T> item = std::move(buffer_[read_index_]);
//         read_index_ = (read_index_ + 1) % capacity_;
//         --count_;

//         not_full_.notify_one();
//         return item;
//     }

//     virtual ~MPMCQueue() {}

//    private:
//     const int capacity_;
//     std::vector<std::unique_ptr<T>> buffer_;
//     std::atomic<int> read_index_;
//     std::atomic<int> write_index_;
//     std::atomic<int> count_;
//     std::mutex mutex_;
//     std::condition_variable not_full_;
//     std::condition_variable not_empty_;
// };