#include "array_util.h"
#include "qsort.h"
#include "pqsort.h"

#include <iostream>
#include <chrono>

using int_type = short;

template <typename T, typename Iter>
concept SorterType = QSortIter<Iter> && requires(T t, Iter begin, Iter end) { t.sort(begin, end); };

template <QSortIter Iter, SorterType<Iter> Sorter>
int64_t run_experiment(Iter ibegin, Iter iend, Sorter sorter) {
  std::chrono::steady_clock::time_point begin_time, end_time;
  begin_time = std::chrono::steady_clock::now();
  sorter.sort(ibegin, iend);
  end_time = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(end_time - begin_time).count();
}

int main() {
  auto first_arr = ArrayUtil<int_type>::generate_array(1'000'000'00);
  auto second_arr = first_arr;
  QSorter<std::vector<int_type>::iterator> qsorter;
  PQSorter<std::vector<int_type>::iterator, 4> pqsorter;
  // for (auto& e : arr) {
  //   std::cout << e << ' ';
  // }
  // std::cout << std::endl;
  std::cout << std::boolalpha;
  std::cout << "Array is sorted before: " << ArrayUtil<int_type>::is_sorted(first_arr) << std::endl;
  std::cout << "Time difference (single thread) = " << run_experiment(first_arr.begin(), first_arr.end(), qsorter)
            << "[µs]" << std::endl;
  std::cout << "Array is sorted after: " << ArrayUtil<int_type>::is_sorted(first_arr) << std::endl;

  std::cout << "Array is sorted before: " << ArrayUtil<int_type>::is_sorted(second_arr) << std::endl;
  std::cout << "Time difference (multiple threads) = " << run_experiment(second_arr.begin(), second_arr.end(), pqsorter)
            << "[µs]" << std::endl;
  std::cout << "Array is sorted after: " << ArrayUtil<int_type>::is_sorted(second_arr) << std::endl;
  // for (auto& e : arr) {
  //   std::cout << e << ' ';
  // }
  // std::cout << std::endl;
  return 0;
}
