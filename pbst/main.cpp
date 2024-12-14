#include "pbst.h"
#include "rand_test.h"

#include <iostream>
#include <thread>
#include <barrier>
#include <unordered_set>
#include <queue>
#include <optional>
#include <stack>

enum OperationType { INSERT, REMOVE, CONTAINS };

template <std::integral Key>
struct Operation {
  OperationType operation_type;
  Key key;
  uint64_t time;
  bool contains_res;
};

template <std::integral Key>
struct TestHistory {
  std::vector<Key> init_values;
  std::vector<std::vector<Operation<Key>>> operations;
};

template <std::integral Key>
bool insert(PBST<Key>& pbst, const std::vector<Key>& values, std::vector<Operation<Key>>& thread_ops) {
  auto key = *vsmr_rand::rand_citer(values);
  uint64_t time;
  pbst.Insert(key, time);
  thread_ops.push_back({OperationType::INSERT, key, time, false});
#ifdef DEBUG
  std::ostringstream os;
  os << "thread #" << std::this_thread::get_id() << " ";
  os << "inserts ";
  os << key << " (time: " << time << ")" << std::endl;
  std::cout << os.str();
#endif
  return true;
}

template <std::integral Key>
bool remove(PBST<Key>& pbst, const std::vector<Key>& values, std::vector<Operation<Key>>& thread_ops) {
  auto key = *vsmr_rand::rand_citer(values);
  uint64_t time;
  pbst.Remove(key, time);
  thread_ops.push_back({OperationType::REMOVE, key, time, false});
#ifdef DEBUG
  std::ostringstream os;
  os << "thread #" << std::this_thread::get_id() << " ";
  os << "removes ";
  os << key << " (time: " << time << ")" << std::endl;
  std::cout << os.str();
#endif
  return true;
}

template <std::integral Key>
bool contains(PBST<Key>& pbst, const std::vector<Key>& values, std::vector<Operation<Key>>& thread_ops) {
  auto key = *vsmr_rand::rand_citer(values);
  uint64_t time;
  auto res = pbst.Contains(key, time);
  thread_ops.push_back({OperationType::CONTAINS, key, time, res});
#ifdef DEBUG
  std::ostringstream os;
  os << "thread #" << std::this_thread::get_id() << " ";
  os << "contains ";
  os << key << ": " << res << " (time: " << time << ")" << std::endl;
  std::cout << os.str();
#endif
  return true;
}

template <std::integral Key>
auto prepopulate(PBST<Key>& pbst, const std::vector<Key>& all_vals) {
  std::vector<Key> inserted;
  inserted.reserve(all_vals.size() * 2 / 3);
  for (const auto& v : all_vals) {
    if (vsmr_rand::rand<int>() % 2) {
      inserted.emplace_back(v);
    }
  }

  std::stack<std::pair<size_t, size_t>> s;
  s.emplace(0, inserted.size());
  while (!s.empty()) {
    auto [l, r] = s.top();
    s.pop();
    auto m = l + (r - l) / 2;
    pbst.Insert(inserted[m]);
    if (m != l) {
      s.emplace(l, m);
      s.emplace(m + 1, r);
    }
  }

  return inserted;
}

template <std::integral Key, Key values_lb, Key values_ub>
TestHistory<Key> test(PBST<Key>& pbst, size_t parallelism, size_t x, size_t duration_seconds) {
  std::vector<std::vector<Operation<Key>>> test_results(parallelism);
  std::vector<std::jthread> threads;
  std::vector<Key> values;

  for (Key i = values_lb; i < values_ub; ++i) {
    values.emplace_back(i);
  }

  auto inserted = prepopulate(pbst, values);

  for (size_t i = 0; i < parallelism; ++i) {
    threads.emplace_back([&pbst, &values, &test_results, x, i, duration_seconds]() {
      vsmr_rand::DurationSeconds condition(duration_seconds);
      vsmr_rand::rand_test(
          &condition, []() { return true; }, x,
          [&pbst, &values, &test_results, i]() { return insert(pbst, values, test_results[i]); }, x,
          [&pbst, &values, &test_results, i]() { return remove(pbst, values, test_results[i]); }, 10 - 2 * x,
          [&pbst, &values, &test_results, i]() { return contains(pbst, values, test_results[i]); });
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  return {inserted, test_results};
}

template <std::integral Key>
bool operator<(const std::pair<size_t, Operation<Key>>& lhs, const std::pair<size_t, Operation<Key>>& rhs) {
  return lhs.second.time > rhs.second.time;
}

template <std::integral Key>
bool operator<(const std::pair<size_t, uint64_t>& lhs, const std::pair<size_t, uint64_t>& rhs) {
  return lhs.second > rhs.second;
}

template <std::integral Key>
bool history_is_linearizable(const TestHistory<Key>& history) {
  std::unordered_set<Key> commited_set;
  std::unordered_map<Key, uint64_t> superposition_key_time_ub;
  std::vector<std::optional<Operation<Key>>> concurrent_ops(history.operations.size());

  std::priority_queue<std::pair<size_t, Operation<Key>>> starts;
  std::priority_queue<std::pair<size_t, uint64_t>> finishes;

  for (size_t tid = 0; tid < history.operations.size(); ++tid) {
    if (history.operations[tid].empty()) {
      continue;
    }

    starts.emplace(tid, history.operations[tid].front());
    for (size_t i = 1; i < history.operations[tid].size(); ++i) {
      starts.emplace(tid, history.operations[tid][i]);
      finishes.emplace(tid, history.operations[tid][i].time);
    }
    finishes.emplace(tid, std::numeric_limits<uint64_t>::max());
  }
  for (const auto& v : history.init_values) {
    commited_set.emplace(v);
  }

  while (!starts.empty()) {
    if (starts.top().second.time < finishes.top().second) {
      auto [tid, op] = starts.top();
      starts.pop();
      concurrent_ops[tid].emplace(op);
      start_operation(commited_set, superposition_key_time_ub, concurrent_ops, op);
    } else {
      auto [tid, finish_time] = finishes.top();
      finishes.pop();
      if (concurrent_ops[tid]) {
        finish_operation(commited_set, superposition_key_time_ub, concurrent_ops, concurrent_ops[tid].value(),
                         finish_time);
      }
    }
  }

  return true;
}

template <std::integral Key>
void start_operation(std::unordered_set<Key>& commited_set,
                     std::unordered_map<Key, uint64_t>& superposition_key_time_ub,
                     std::vector<std::optional<Operation<Key>>>& concurrent_ops, Operation<Key> op) {
  if (op.operation_type == OperationType::INSERT) {
    if (count_ops(concurrent_ops, OperationType::REMOVE, op.key) || !commited_set.contains(op.key)) {
      if (!superposition_key_time_ub.contains(op.key)) {
        superposition_key_time_ub[op.key] = std::numeric_limits<uint64_t>::min();
      }
    }
  } else if (op.operation_type == OperationType::REMOVE) {
    if (count_ops(concurrent_ops, OperationType::INSERT, op.key) || commited_set.contains(op.key)) {
      commited_set.erase(op.key);
      if (!superposition_key_time_ub.contains(op.key)) {
        superposition_key_time_ub[op.key] = std::numeric_limits<uint64_t>::min();
      }
    }
  }
  filter_out_contains_in_superposition(superposition_key_time_ub, concurrent_ops);
}

template <std::integral Key>
void finish_operation(std::unordered_set<Key>& commited_set,
                      std::unordered_map<Key, uint64_t>& superposition_key_time_ub,
                      std::vector<std::optional<Operation<Key>>>& concurrent_ops, Operation<Key> op,
                      uint64_t finish_time) {
  switch (op.operation_type) {
    case OperationType::INSERT:
      finish_insert(commited_set, superposition_key_time_ub, concurrent_ops, op, finish_time);
      break;
    case OperationType::REMOVE:
      finish_remove(superposition_key_time_ub, concurrent_ops, op, finish_time);
      break;
    default:
      finish_contains(commited_set, op);
      break;
  }
}

template <std::integral Key>
void finish_insert(std::unordered_set<Key>& commited_set, std::unordered_map<Key, uint64_t>& superposition_key_time_ub,
                   std::vector<std::optional<Operation<Key>>>& concurrent_ops, Operation<Key> op,
                   uint64_t finish_time) {
  if (count_ops(concurrent_ops, OperationType::REMOVE, op.key)) {
    superposition_key_time_ub[op.key] = finish_time;
    return;
  }
  if (superposition_key_time_ub.contains(op.key) && superposition_key_time_ub[op.key] > op.time) {
    return;
  }
  superposition_key_time_ub.erase(op.key);
  commited_set.insert(op.key);
}

template <std::integral Key>
void finish_remove(std::unordered_map<Key, uint64_t>& superposition_key_time_ub,
                   std::vector<std::optional<Operation<Key>>>& concurrent_ops, Operation<Key> op,
                   uint64_t finish_time) {
  if (count_ops(concurrent_ops, OperationType::INSERT, op.key)) {
    superposition_key_time_ub[op.key] = finish_time;
    return;
  }
  if (superposition_key_time_ub.contains(op.key) && superposition_key_time_ub[op.key] > op.time) {
    return;
  }
  superposition_key_time_ub.erase(op.key);
}

template <std::integral Key>
void finish_contains(std::unordered_set<Key>& commited_set, Operation<Key> op) {
  assert(commited_set.contains(op.key) == op.contains_res);
}

template <std::integral Key>
void filter_out_contains_in_superposition(const std::unordered_map<Key, uint64_t>& superposition_key_time_ub,
                                          std::vector<std::optional<Operation<Key>>>& concurrent_ops) {
  for (auto& op : concurrent_ops) {
    if (op && superposition_key_time_ub.contains(op.value().key)) {
      op.reset();
    }
  }
}

template <std::integral Key>
uint64_t count_ops(const std::vector<std::optional<Operation<Key>>>& ops, OperationType op_type, Key key) {
  uint64_t res = 0;
  for (const auto& op : ops) {
    res += op && op.value().operation_type == op_type && op.value().key == key;
  }
  return res;
}

template <std::integral Key>
void print_history(const std::vector<std::vector<Operation<Key>>>& history) {
  int i = 0;
  std::cout << std::boolalpha;
  for (const auto& ops : history) {
    std::cout << i++ << ":\n";
    for (const auto& op : ops) {
      std::cout << "Operation<int>{";
      switch (op.operation_type) {
        case OperationType::INSERT:
          std::cout << "OperationType::INSERT, ";
          break;
        case OperationType::REMOVE:
          std::cout << "OperationType::REMOVE, ";
          break;
        default:
          std::cout << "OperationType::CONTAINS, ";
          break;
      }
      std::cout << op.key << ", " << op.time << ", " << op.contains_res << "},\n";
    }
  }
}

void experiment(uint64_t parallelism, uint64_t x, uint64_t duration_seconds) {
  PBST<int> pbst;
  std::cout << "Running experiment for parallelism=" << parallelism << ", x=" << x << ", duration=" << duration_seconds
            << "s..." << std::endl;
  auto history = test<int, 0, 100'000>(pbst, parallelism, x, duration_seconds);

  std::cout << std::boolalpha;
  size_t bandwidth = 0;
  for (const auto& tops : history.operations) {
    bandwidth += tops.size();
  }
  bandwidth /= duration_seconds;
  // print_history(history);
  std::cout << "Experiment finished, running checks..." << std::endl;
  std::cout << "Performed operations history is linearizable: " << history_is_linearizable(history) << std::endl;
  std::cout << "Result structure is a valid external BST: " << pbst.ValidBST() << std::endl;
  std::cout << "Average bandwidth: " << bandwidth << " op/s" << std::endl;
}

int main(int argc, char* argv[]) {
  if (argc < 4) {
    std::cerr << "Wrong use of the application. You should pass desired parallelism, probability parameter x (0 <= x "
                 "<= 5) and test duration in seconds."
              << std::endl;
    std::cerr << "Expected call: " << argv[0] << " <parallelism> <x> <durations>" << std::endl;
    std::cerr << "Example: " << argv[0] << " 4 1 5" << std::endl;
    return 1;
  }
  uint64_t parallelism = strtoull(argv[1], NULL, 10);
  uint64_t x = strtoull(argv[2], NULL, 10);
  uint64_t duration = strtoull(argv[3], NULL, 10);

  experiment(parallelism, x, duration);

  return 0;
}
