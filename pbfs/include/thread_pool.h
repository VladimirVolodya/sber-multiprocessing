#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "blocking_queue.h"

#include <thread>
#include <vector>
#include <functional>
#include <barrier>

class ThreadPool {
 public:
  ThreadPool(size_t n_threads);
  void schedule(const std::function<void()>& task);
  void schedule(std::function<void()>&& task);
  template <std::random_access_iterator BeginIter, std::random_access_iterator EndIter>
  void schedule_all(BeginIter begin, EndIter end);
  template <std::random_access_iterator BeginIter, std::random_access_iterator EndIter>
  void schedule_all(BeginIter begin, EndIter end, int64_t batch_size);
  template <std::random_access_iterator BeginIter, std::random_access_iterator EndIter>
  void execute_sync(BeginIter begin, EndIter end);
  void finish_all();
  size_t get_parallelism() const;
  auto get_tids() const;

 private:
  void run();
  size_t n_threads_;
  std::vector<std::thread> threads_;
  BlockingQueue<std::function<bool()>> task_queue_;
  std::barrier<> cur_tasks_completed_;
};

ThreadPool::ThreadPool(size_t n_threads) : n_threads_(n_threads), cur_tasks_completed_(n_threads + 1) {
  threads_.reserve(n_threads);
  for (size_t i = 0; i < n_threads; ++i) {
    threads_.emplace_back([this] { this->run(); });
  }
}

void ThreadPool::schedule(const std::function<void()>& task) {
  task_queue_.push([&task] {
    task();
    return false;
  });
}

template <std::random_access_iterator BeginIter, std::random_access_iterator EndIter>
void ThreadPool::schedule_all(BeginIter begin, EndIter end) {
  schedule_all(begin, end, this->get_parallelism() * 3);
}

template <std::random_access_iterator BeginIter, std::random_access_iterator EndIter>
void ThreadPool::schedule_all(BeginIter begin, EndIter end, int64_t batch_size) {
  std::vector<std::function<bool()>> tasks;
  EndIter cur_end;

  tasks.reserve(std::min(batch_size, end - begin));
  while (begin != end) {
    cur_end = (begin + batch_size) > end ? end : begin + batch_size;
    while (begin != cur_end) {
      tasks.emplace_back([begin] {
        (*begin)();
        return false;
      });
      ++begin;
    }
    task_queue_.push_all(std::move_iterator(tasks.begin()), std::move_iterator(tasks.cend()));
    tasks.clear();
  }
}

template <std::random_access_iterator BeginIter, std::random_access_iterator EndIter>
void ThreadPool::execute_sync(BeginIter begin, EndIter end) {
  static int i = -1;
  ++i;
  std::vector<std::function<void()>> complete_tasks(threads_.size(),
                                                    [this] { this->cur_tasks_completed_.arrive_and_wait(); });
  schedule_all(begin, end);
  schedule_all(complete_tasks.begin(), complete_tasks.cend());
  cur_tasks_completed_.arrive_and_wait();
}

size_t ThreadPool::get_parallelism() const {
  return threads_.size();
}

void ThreadPool::run() {
  std::function<bool()> task;
  for (;;) {
    task_queue_.blocking_pop(task);
    if (task()) {
      return;
    }
  }
}

void ThreadPool::finish_all() {
  std::vector<std::function<bool()>> finish_tasks(threads_.size(), [] { return true; });
  task_queue_.push_all(finish_tasks.begin(), finish_tasks.cend());
  for (auto& t : threads_) {
    t.join();
  }
}

auto ThreadPool::get_tids() const {
  std::vector<std::thread::id> tids;
  tids.reserve(n_threads_);
  for (const auto& t : threads_) {
    tids.emplace_back(t.get_id());
  }
  return tids;
}

#endif  // THREADPOOL_H