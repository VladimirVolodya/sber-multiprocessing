#ifndef ARRAY_UTIL
#define ARRAY_UTIL

#include <concepts>
#include <random>
#include <limits>

template <std::integral T>
class ArrayUtil {
 public:
  static std::vector<T> generate_array(size_t size) {
    std::vector<T> result(size);
    for (auto& e : result) {
      e = rand_value();
    }
    return result;
  }
  static bool is_sorted(const std::vector<T>& array) {
    for (size_t i = 0; i + 1 < array.size(); ++i) {
      if (array[i] > array[i + 1]) {
        return false;
      }
    }
    return true;
  }

 private:
  static inline T rand_value() {
    return distrib(gen);
  }

  static std::mt19937 gen;
  static std::uniform_int_distribution<T> distrib;
};

template <std::integral T>
std::mt19937 ArrayUtil<T>::gen = std::mt19937(std::random_device()());

template <std::integral T>
std::uniform_int_distribution<T> ArrayUtil<T>::distrib =
    std::uniform_int_distribution<T>(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
// template <std::integral T>
// std::uniform_int_distribution<T> ArrayUtil<T>::distrib = std::uniform_int_distribution<T>(0, 20);

#endif  // ARRAY_UTIL
