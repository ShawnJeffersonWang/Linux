#include <gtest/gtest.h>
#include <iostream>
namespace Shawn {
template <class T>
// list存储的节点
// 可以根据你的需要添加适当的成员函数与成员变量
struct node {
    node<T>* prev_;
    node<T>* next_;
    T data_;
    // 构造函数
    node(const T& data) : data_(data), prev_(nullptr), next_(nullptr) {}
    node() : prev_(nullptr), next_(nullptr) {}
    // 析构函数
    ~node() {}
};

template <class T>
// 迭代器类,(类比指针)
struct iterator {
    typedef node<T> node_;
    typedef iterator<T> iterator_;
    node_* data_;
    iterator(node_* data) : data_(data) {}
    ~iterator() {}
    // 迭代到下一个节点
    //++it
    iterator_& operator++() {
        data_ = data_->next_;
        return *this;
    }
    // 迭代到前一个节点
    //--it
    iterator_& operator--() {
        data_ = data_->prev_;
        return *this;
    }
    // it++
    iterator operator++(int) {
        iterator it = *this;
        data_ = data_->next_;
        return it;
    }
    // it--
    iterator operator--(int) {
        iterator it = *this;
        data_ = data_->prev_;
        return it;
    }
    // 获得迭代器的值
    T& operator*() { return data_->data_; }
    // 获得迭代器对应的指针
    T* operator->() { return &(data_->data_); }
    // 重载==
    bool operator==(const iterator_& t) { return data_ == t.data_; }
    // 重载！=
    bool operator!=(const iterator_& t) { return data_ != t.data_; }

    //**可以添加需要的成员变量与成员函数**
};

template <class T>
class list {
   public:
    typedef node<T> node_;
    typedef iterator<T> Iterator;

   private:
    // 可以添加你需要的成员变量
    node_* head_;  // 头节点,哨兵（不存储有效数据）
    size_t size_;  // 存储有效节点的数量

   public:
    // 构造函数
    list() : head_(new node_<T>()), size_(0) {
        head_->prev_ = head_->next_ = head_;
    }
    // 拷贝构造，要求实现深拷贝
    list(const list<T>& lt) : head_(new node_<T>()), size_(0) {
        head_->prev_ = head_->next_ = head_;
        node_* cur = lt.head_->next_;
        while (cur != lt.head_) {
            push_back(cur->data_);
            cur = cur->next_;
        }
    }
    template <class inputIterator>
    // 迭代器构造
    list(inputIterator begin, inputIterator end)
        : head_(new node_<T>()), size_(0) {
        head_->prev_ = head_->next_ = head_;
        for (auto it = begin; it != end; ++it) {
            push_back(*it);
        }
    }

    ~list() {
        clear();
        delete head_;
    }
    // 拷贝赋值
    list<T>& operator=(const list<T>& lt) {
        if (this != &lt) {
            list<T> tmp(lt);
            swap(tmp);
        }
        return *this;
    }
    // 清空list中的数据
    void clear() {
        node_* cur = head_->next_;
        while (cur != head_) {
            node_* tmp = cur;
            cur = cur->next_;
            delete tmp;
        }
        head_->prev_ = head_->next_ = head_;
        size_ = 0;
    }
    // 返回容器中存储的有效节点个数
    size_t size() const { return size_; }
    // 判断是否为空
    bool empty() const { return size_ == 0; }
    // 尾插
    void push_back(const T& data) { insert(end(), data); }
    // 头插
    void push_front(const T& data) { insert(begin(), data); }
    // 尾删
    void pop_back() { erase(--end()); }
    // 头删
    void pop_front() { erase(begin()); }
    // 默认新数据添加到pos迭代器的后面,根据back的方向决定插入在pos的前面还是后面
    void insert(Iterator pos, const T& data, bool back = true) {
        node_* cur = pos.data_;
        node_* prev = cur->prev_;
        node_* next = cur;
        if (back) {
            next = cur->next_;
        }
        node_* tmp = new node_<T>(data);
        tmp->prev_ = prev;
        tmp->next_ = next;
        prev->next_ = tmp;
        next->prev_ = tmp;
        ++size_;
    }
    // 删除pos位置的元素
    void erase(Iterator pos) {
        node_* cur = pos.data_;
        node_* prev = cur->prev_;
        node_* next = cur->next_;
        prev->next_ = next;
        next->prev_ = prev;
        delete cur;
        --size_;
    }

    // 获得list第一个有效节点的迭代器
    Iterator begin() const { return Iterator(head_->next_); }

    // 获得list最后一个节点的下一个位置，可以理解为nullptr
    Iterator end() const { return Iterator(head_); }
    // 查找data对应的迭代器
    Iterator find(const T& data) const {
        node_* cur = head_->next_;
        while (cur != head_) {
            if (cur->data_ == data) {
                return Iterator(cur);
            }
            cur = cur->next_;
        }
        return end();
    }
    // 获得第一个有效节点
    node_ front() const { return *(begin().data_); }
    // 获得最后一个有效节点
    node_ back() const { return *(--end()).data_; }

   private:
    // 可以添加你需要的成员函数
    void swap(list<T>& lt) {
        std::swap(head_, lt.head_);
        std::swap(size_, lt.size_);
    }
};
};  // namespace Shawn