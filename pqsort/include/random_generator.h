#ifndef RANDOM_GEN
#define RANDOM_GEN

#include <concepts>
#include <random>
#include <limits>

template <std::integral T>
class RandomGenerator {
 public:
  RandomGenerator()
      : gen_(std::random_device()()), distrib_(std::numeric_limits<T>::min(), std::numeric_limits<T>::max()) {
  }
  T random_value() {
    return distrib_(gen_);
  }

 private:
  std::mt19937 gen_;
  std::uniform_int_distribution<T> distrib_;
};

#endif  // RANDOM_GEN
