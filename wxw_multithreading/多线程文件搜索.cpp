#include <algorithm>
#include <condition_variable>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>
struct SearchConfig {
    std::string root_path;  // 要搜索的根目录
    std::string file_type;  // 要搜索的文件类型，如 ".txt"、".cpp" 等
    int max_concurrency;    // 最大并发数
    int max_depth;          // 最大搜索深度
    bool skip_hidden;       // 是否跳过隐藏文件或目录
    std::vector<std::string> skip_paths;  // 要跳过的目录或文件的路径
};

// 用于存储搜索结果的数据结构
struct SearchResult {
    std::vector<std::string> file_paths;  // 符合条件的文件路径
    std::mutex mutex;  // 用于保护 file_paths 的互斥量
};

// 递归搜索目录下的文件
void search_files(const std::string& path,
                  const std::string& file_type,
                  SearchResult& result,
                  int max_depth,
                  bool skip_hidden,
                  const std::vector<std::string>& skip_paths) {
    std::filesystem::directory_iterator end_itr;  // 迭代器的末尾
    for (std::filesystem::directory_iterator itr(path); itr != end_itr; ++itr) {
        if (max_depth == 0) {
            break;
        }
        if (std::filesystem::is_directory(itr->status())) {
            // 检查是否需要跳过该目录
            if (skip_hidden && itr->path().filename().string()[0] == '.') {
                continue;
            }
            if (std::find(skip_paths.begin(), skip_paths.end(),
                          itr->path().string()) != skip_paths.end()) {
                continue;
            }
            // 递归搜索子目录
            search_files(itr->path().string(), file_type, result, max_depth - 1,
                         skip_hidden, skip_paths);
        } else if (std::filesystem::is_regular_file(itr->status())) {
            // 检查文件类型是否符合要求
            if (itr->path().extension() == file_type) {
                std::lock_guard<std::mutex> lock(result.mutex);
                result.file_paths.push_back(itr->path().string());
            }
        }
    }
}

// 多线程搜索文件
void search_files_multithreaded(const SearchConfig& config,
                                SearchResult& result) {
    std::queue<std::string> dirs_to_search;  // 待搜索的目录队列
    dirs_to_search.push(config.root_path);

    std::vector<std::thread> threads;  // 存储所有线程的 vector
    std::mutex mutex;            // 用于保护 dirs_to_search 的互斥量
    std::condition_variable cv;  // 用于线程间的通信

    // 创建多个线程
    for (int i = 0; i < config.max_concurrency; ++i) {
        threads.emplace_back([&]() {
            while (true) {
                std::unique_lock<std::mutex> lock(mutex);
                cv.wait(lock, [&]() { return !dirs_to_search.empty(); });

                std::string dir_to_search = dirs_to_search.front();
                dirs_to_search.pop();
                lock.unlock();

                // 搜索该目录下的文件
                search_files(dir_to_search, config.file_type, result,
                             config.max_depth, config.skip_hidden,
                             config.skip_paths);

                // 通知主线程已完成搜索
                cv.notify_one();
            }
        });
    }

    // 主线程等待所有线程完成搜索
    while (!dirs_to_search.empty()) {
        std::unique_lock<std::mutex> lock(mutex);
        cv.notify_all();
        cv.wait(lock, [&]() { return dirs_to_search.empty(); });
    }

    // 通知所有线程结束
    cv.notify_all();

    // 等待所有线程结束
    for (auto& t : threads) {
        t.join();
    }
}

// 输出搜索结果
void print_search_result(const SearchResult& result) {
    std::vector<std::string> file_paths = result.file_paths;
    std::sort(file_paths.begin(), file_paths.end());
    file_paths.erase(std::unique(file_paths.begin(), file_paths.end()),
                     file_paths.end());  // 去重
    std::cout << "Search result: " << std::endl;
    for (const auto& path : file_paths) {
        std::cout << path << std::endl;
    }
}

int main() {
    SearchConfig config = {"/path/to/search",
                           ".txt",
                           4,
                           3,
                           true,
                           {"/path/to/search/skip1", "/path/to/search/skip2"}};
    SearchResult result;
    search_files_multithreaded(config, result);
    print_search_result(result);
    return 0;
}