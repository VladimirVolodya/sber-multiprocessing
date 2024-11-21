#ifndef Q_SORT
#define Q_SORT

#include <concepts>
#include <iterator>
#include <functional>

#include "random_generator.h"

template <typename T>
concept QSortIter = std::is_integral_v<typename std::iterator_traits<T>::value_type> && std::bidirectional_iterator<T>;

template <QSortIter Iter>
class QSorter {
 private:
  using value_type = typename std::iterator_traits<Iter>::value_type;

 public:
  QSorter(RandomGenerator<int64_t> gen) : gen_(gen) {
  }
  QSorter() = default;

  void sort(Iter ibegin, Iter iend) {
    if (iend - ibegin < 2) {
      return;
    }
    auto [less_end, greater_begin] = partition(ibegin, iend, random_pivot(ibegin, iend));
    sort(ibegin, less_end);
    sort(greater_begin, iend);
  }

  std::pair<Iter, Iter> partition(Iter ibegin, Iter iend, const value_type& pivot) {
    Iter less_end = ibegin;
    Iter greater_begin = iend;
    Iter cur = ibegin;
    while (greater_begin != cur) {
      if (*cur < pivot) {
        std::swap(*less_end, *cur);
        ++less_end;
        ++cur;
      } else if (*cur > pivot) {
        --greater_begin;
        std::swap(*greater_begin, *cur);
      } else {
        ++cur;
      }
    }
    return {less_end, greater_begin};
  }

  auto random_pivot(const Iter& ibegin, const Iter& iend) {
    int64_t size = iend - ibegin;
    int64_t idx = (size + gen_.random_value() % size) % size;
    return *(ibegin + idx);
  }

 private:
  RandomGenerator<int64_t> gen_;
};

#endif  // Q_SORT