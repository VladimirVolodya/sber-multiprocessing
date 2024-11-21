#ifndef PARALLEL_Q_SORT
#define PARALLEL_Q_SORT

#include "qsort.h"
#include "blocking_queue.h"

#include <concepts>
#include <cstddef>
#include <thread>
#include <ranges>
#include <sstream>
#include <iostream>
#include <iterator>

template <QSortIter Iter, size_t n_threads>
class PQSorter {
 private:
  struct SortTask {
    Iter ibegin;
    Iter iend;
  };

 public:
  void sort(Iter ibegin, Iter iend) {
    std::vector<std::jthread> threads;
    BlockingQueue<SortTask> task_queue;
    task_queue.push({ibegin, iend});
    run(task_queue, 3 * n_threads);
    // return;

    for (const auto& i : std::views::iota(1ul, n_threads)) {
      threads.emplace_back([&task_queue] { run(task_queue); });
    }
    run(task_queue);
    for (auto& t : threads) {
      t.join();
    }
  }

 private:
  void inner_sort(Iter ibegin, Iter iend) {
  }

  static void run(BlockingQueue<SortTask>& task_queue, size_t iter_count = std::numeric_limits<size_t>::max()) {
    SortTask task;
    QSorter<Iter> qsorter;
    for (size_t i = 0; i < iter_count && task_queue.blocking_pop(task); ++i) {
      // logging
      //   std::stringstream ss;
      //   ss << std::format("IterCount={} Thread {} got task [ ", iter_count, std::this_thread::get_id());
      //   Iter it = task.ibegin;
      //   while (it != task.iend) {
      //     ss << *it++ << " ";
      //   }
      //   ss << " ]\n";
      //   std::cout << ss.str() << std::endl;

      if (task.iend - task.ibegin < size_ub_for_parallel_) {
        qsorter.sort(task.ibegin, task.iend);
        continue;
      }
      typename std::iterator_traits<Iter>::value_type pivot = qsorter.random_pivot(task.ibegin, task.iend);
      auto [less_end, greater_begin] = qsorter.partition(task.ibegin, task.iend, pivot);
      // logging
      //   ss.str("");
      //   ss << std::format("IterCount={} Thread {} prepared tasks with pivot={}: ([", iter_count,
      //                     std::this_thread::get_id(), pivot);
      //   it = task.ibegin;
      //   while (it != less_end) {
      //     ss << *it++ << " ";
      //   }
      //   ss << "], [";
      //   it = greater_begin;
      //   while (it != task.iend) {
      //     ss << *it++ << " ";
      //   }
      //   ss << "])\n";
      //   std::cout << ss.str() << std::endl;

      task_queue.push(SortTask{std::move(task.ibegin), std::move(less_end)});
      task_queue.push(SortTask{std::move(greater_begin), std::move(task.iend)});
    }
  }
  static int64_t size_ub_for_parallel_;
};

template <QSortIter Iter, size_t n_threads>
int64_t PQSorter<Iter, n_threads>::size_ub_for_parallel_ = 1000ll;

#endif  // PARALLEL_Q_SORT
