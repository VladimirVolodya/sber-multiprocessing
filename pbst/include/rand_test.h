#ifndef RANDOM_LIB_H
#define RANDOM_LIB_H

#include <concepts>
#include <random>
#include <functional>
#include <utility>
#include <cassert>
#include <chrono>
#include <iostream>

namespace vsmr_rand {

template <typename T>
concept has_size = requires(T t) {
  { t.size() } -> std::same_as<size_t>;
};

template <typename T>
concept has_const_iterator = requires(T t) { typename T::const_iterator; };

template <typename T>
concept has_cbegin = requires(T t) {
  { t.cbegin() } -> std::forward_iterator;
};

template <typename T>
concept has_iterator = requires(T t) { typename T::iterator; };

template <typename T>
concept has_begin = requires(T t) {
  { t.begin() } -> std::forward_iterator;
};

template <typename T>
concept const_container = has_size<T> && has_const_iterator<T> && has_cbegin<T>;

template <typename T>
concept container = has_size<T> && has_iterator<T> && has_begin<T>;

template <typename T>
concept forward_iterator_over_integral = std::forward_iterator<T> && std::integral<typename T::value_type>;

template <std::integral T, std::integral... Args>
T gcd(T num, Args... other_nums) {
  if (!sizeof...(other_nums)) {
    return num;
  }
  return std::gcd(num, gcd(other_nums...));
}

template <forward_iterator_over_integral BeginIter, forward_iterator_over_integral EndIter>
auto gcd(BeginIter begin, EndIter end) {
  auto val = *begin++;
  if (begin == end) {
    return val;
  }
  return std::gcd(val, gcd(begin, end));
}

template <std::integral T>
class RandomIntegralGenerator {
 public:
  RandomIntegralGenerator()
      : gen_(std::random_device()()), distr_(std::numeric_limits<T>::min(), std::numeric_limits<T>::max()) {
  }
  T rand() {
    return distr_(gen_);
  }

 private:
  std::mt19937 gen_;
  std::uniform_int_distribution<T> distr_;
};

template <std::floating_point T>
class RandomFloatingGenerator {
 public:
  RandomFloatingGenerator()
      : gen_(std::random_device()()), distr_(std::numeric_limits<T>::min(), std::numeric_limits<T>::max()) {
  }
  T rand() {
    return distr_(gen_);
  }

 private:
  std::mt19937 gen_;
  std::uniform_real_distribution<T> distr_;
};

template <std::integral T>
T rand() {
  thread_local RandomIntegralGenerator<T> gen;
  return gen.rand();
}

template <std::floating_point T>
T rand() {
  thread_local RandomFloatingGenerator<T> gen;
  return gen.rand();
}

template <const_container Container>
Container::const_iterator rand_citer(const Container& cont) {
  return std::next(cont.cbegin(), rand<size_t>() % cont.size());
}

template <container Container>
Container::iterator rand_iter(Container& cont) {
  return std::next(cont.begin(), rand<size_t>() % cont.size());
}

// template <const_container Container>
// Container::const_iterator rand_citer(const Container& cont, RandomIntegralGenerator<size_t>& gen) {
//   return std::next(cont.cbegin(), gen.rand() % cont.size());
// }

// template <container Container>
// Container::iterator rand_iter(Container& cont, RandomIntegralGenerator<size_t>& gen) {
//   return std::next(cont.begin(), gen.rand() % cont.size());
// }

class IConditionHandler {
 public:
  virtual void reset() {
  }
  virtual bool check() = 0;
  virtual ~IConditionHandler() = default;
};

class Repeats : public IConditionHandler {
 public:
  Repeats(size_t n_repeats) : n_repeats_(n_repeats), cur_(0ull) {
  }
  virtual void reset() override {
    cur_ = 0ull;
  }
  virtual bool check() override {
    return cur_++ < n_repeats_;
  }
  ~Repeats() = default;

 private:
  size_t n_repeats_;
  size_t cur_;
};

class DurationSeconds : public IConditionHandler {
 public:
  DurationSeconds(size_t seconds) : milliseconds_(seconds * 1'000ull) {
  }
  void reset() override {
    start = std::chrono::steady_clock::now();
  }
  bool check() override {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() <
           milliseconds_;
  }
  ~DurationSeconds() = default;

 private:
  size_t milliseconds_;
  std::chrono::steady_clock::time_point start;
};

class RandTestArgParser {
 private:
  struct RandTestOutcome {
    std::function<bool()> checker;
    size_t prob_mul;
  };

  template <typename... Args>
  static auto parse_rand_test_args(size_t prob_mul, std::function<bool()>&& checker, Args... args) {
    if constexpr (!sizeof...(args)) {
      return std::vector<RandTestOutcome>(1, RandTestOutcome{std::move(checker), prob_mul});
    } else {
      auto res = parse_rand_test_args(std::forward<Args>(args)...);
      res.push_back({std::move(checker), prob_mul});
      return res;
    }
  }

  template <typename... Args>
  static auto parse_rand_test_args(size_t prob_mul, const std::function<bool()>& checker, Args... args) {
    auto checker_copy = checker;
    return parse_rand_test_args(prob_mul, std::move(checker_copy), std::forward<Args>(args)...);
  }

 public:
  template <typename... Args>
  friend void rand_test(IConditionHandler* condition_handler, std::function<bool()> common_checker, Args... args);
};

template <typename... Args>
void rand_test(IConditionHandler* condition_handler, std::function<bool()> common_checker, Args... args) {
  auto all_outcomes = RandTestArgParser::parse_rand_test_args(std::forward<Args>(args)...);
  size_t total_outcomes = 0;
  for (const auto& o : all_outcomes) {
    total_outcomes += o.prob_mul;
  }

  condition_handler->reset();
  while (condition_handler->check()) {
    auto outcome_idx = rand<size_t>() % total_outcomes;
    for (const auto& outcome : all_outcomes) {
      if (outcome_idx < outcome.prob_mul) {
        assert(outcome.checker());
        break;
      }
      outcome_idx -= outcome.prob_mul;
    }
    assert(common_checker());
  }
}

}  // namespace vsmr_rand

#endif  // RANDOM_LIB_H
