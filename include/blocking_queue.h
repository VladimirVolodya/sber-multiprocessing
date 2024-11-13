#ifndef BLOCKING_STACK
#define BLOCKING_STACK

#include <queue>
#include <mutex>

template <typename T>
class BlockingQueue {
 public:
  bool blocking_pop(T& value) {
    std::unique_lock lk(m_);
    if (queue_.empty()) {
      return false;
    }
    value = std::move(queue_.front());
    queue_.pop();
    return true;
  }
  void push(const T& value) {
    T copy = value;
    push(std::move(copy));
  }
  void push(T&& value) {
    std::unique_lock lk(m_);
    queue_.push(std::move(value));
  }

 private:
  inline const T& pop() {
    T value = std::move(queue_.front());
    queue_.pop();
    return value;
  }
  std::queue<T> queue_;
  std::mutex m_;
};

#endif  // BLOCKING_STACK
