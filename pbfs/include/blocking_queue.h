#ifndef BLOCKING_QUEUE_H
#define BLOCKING_QUEUE_H

#include <queue>
#include <condition_variable>
#include <iterator>
#include <mutex>
#include <sstream>
#include <iostream>

template <typename T>
class BlockingQueue {
 public:
  void push(const T& value);
  void push(T&& value);
  template <std::forward_iterator BeginIter, std::forward_iterator EndIter>
  void push_all(BeginIter begin, EndIter end);
  void blocking_pop(T& value);
  bool try_pop(T& value);

 private:
  void pop(T& value);
  void notify(std::unique_lock<std::mutex>& lock);
  std::queue<T> queue_;
  std::mutex mutex_;
  std::condition_variable cond_var_;
};

template <typename T>
void BlockingQueue<T>::push(const T& value) {
  T copy = value;
  push(std::move(copy));
}

template <typename T>
void BlockingQueue<T>::push(T&& value) {
  std::unique_lock lock(mutex_);
  queue_.emplace(std::move(value));
  notify(lock);
}

template <typename T>
void BlockingQueue<T>::blocking_pop(T& value) {
  std::unique_lock lock(mutex_);
  cond_var_.wait(lock, [this] { return !this->queue_.empty(); });
  pop(value);
  notify(lock);
}

template <typename T>
bool BlockingQueue<T>::try_pop(T& value) {
  std::unique_lock lock(mutex_);
  if (queue_.empty()) {
    return false;
  }
  pop(value);
  notify(lock);
}

template <typename T>
void BlockingQueue<T>::pop(T& value) {
  value = std::move(queue_.front());
  queue_.pop();
}

template <typename T>
void BlockingQueue<T>::notify(std::unique_lock<std::mutex>& lock) {
  lock.unlock();
  cond_var_.notify_all();
}

template <typename T>
template <std::forward_iterator BeginIter, std::forward_iterator EndIter>
void BlockingQueue<T>::push_all(BeginIter begin, EndIter end) {
  std::unique_lock lock(mutex_);
  while (begin != end) {
    queue_.emplace(*begin++);
  }
  notify(lock);
}

#endif  // BLOCKING_QUEUE_H