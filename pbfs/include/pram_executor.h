#ifndef PRAM_EXECUTOR_H
#define PRAM_EXECUTOR_H

#include "thread_pool.h"

class PRAMExecutor : private ThreadPool {
 public:
  enum BatchingStrategy { SEQUENTIAL, JUMPING };
  PRAMExecutor(size_t n_threads);
  template <std::integral idx_type>
  void pfor(idx_type from, idx_type to, std::function<void(idx_type)> loop_body);
  template <std::integral idx_type>
  void pfor(idx_type from, idx_type to, std::function<void(idx_type)> loop_body, idx_type batch_size);
  template <std::random_access_iterator BeginIter, std::random_access_iterator EndIter>
  auto pfilter(BeginIter begin, EndIter end,
               std::function<bool(const typename std::iterator_traits<BeginIter>::value_type&)> predicate);
  void finish_all();
  auto get_tids() const;
};

PRAMExecutor::PRAMExecutor(size_t n_threads) : ThreadPool(n_threads) {
}

template <std::integral idx_type>
void PRAMExecutor::pfor(idx_type from, idx_type to, std::function<void(idx_type)> loop_body) {
  pfor(from, to, loop_body,
       (to - from + static_cast<idx_type>(this->get_parallelism()) - 1) /
           static_cast<idx_type>(this->get_parallelism()));
}

template <std::integral idx_type>
void PRAMExecutor::pfor(idx_type from, idx_type to, std::function<void(idx_type)> loop_body, idx_type batch_size) {
  std::vector<std::function<void()>> tasks;
  const idx_type range = to - from;
  const size_t n_batches = (range + batch_size - 1) / batch_size;
  const size_t n_full_batches = n_batches - (batch_size - range % batch_size) % batch_size;
  tasks.reserve(n_batches);

  idx_type lb = 0;
  for (idx_type batch_id = 0; batch_id < n_batches; ++batch_id) {
    idx_type ub = lb + batch_size - (batch_id >= n_full_batches);

    tasks.emplace_back([lb, ub, &loop_body]() {
      for (idx_type i = lb; i < ub; ++i) {
        loop_body(i);
      }
    });

    lb = ub;
  }

  execute_sync(tasks.begin(), tasks.cend());
}

template <std::random_access_iterator BeginIter, std::random_access_iterator EndIter>
auto PRAMExecutor::pfilter(BeginIter begin, EndIter end,
                           std::function<bool(const typename std::iterator_traits<BeginIter>::value_type&)> predicate) {
  using value_type = typename std::iterator_traits<BeginIter>::value_type;
  std::vector<value_type> result;
  std::vector<std::vector<value_type>> subresults;
  std::vector<std::function<void()>> tasks;

  size_t size = end - begin;
  subresults.reserve(get_parallelism());

  for (size_t tid = 0; tid < get_parallelism(); ++tid) {
    pfor(0ll, end - begin, [tid, begin, &predicate, &subresults](int64_t idx) {
      if (predicate(*(begin + idx))) {
        subresults[tid].emplace_bach(*begin);
      }
    });
  }

  for (auto& sr : subresults) {
    result.insert(result.end(), std::move_iterator(sr.begin()), sr.cend());
  }

  return result;
}

void PRAMExecutor::finish_all() {
  ThreadPool::finish_all();
}

auto PRAMExecutor::get_tids() const {
  return ThreadPool::get_tids();
}

#endif  // PRAM_EXECUTOR_H
