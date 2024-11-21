#include "cubic_graph.h"
#include "blocking_queue.h"
#include "pram_executor.h"

#include <tuple>
#include <iostream>
#include <thread>
#include <sstream>
#include <chrono>

template <std::unsigned_integral T, std::unsigned_integral U>
using Edge = typename DenseGraph<T, U>::Edge;

template <std::unsigned_integral idx_type>
bool validate_cube_distances(const std::vector<idx_type>& distances, const CubicGraph<idx_type>& graph) {
  for (idx_type idx = 0; idx < distances.size(); ++idx) {
    auto [x, y, z] = graph.idx_1dto3d(idx);
    // std::cout << std::format("Distance to ({}, {}, {}) = {}\n", x, y, z, distances[idx]);
    if (distances[idx] != x + y + z) {
      return false;
    }
  }
  return true;
}

template <std::unsigned_integral idx_type>
int64_t run_experiment(const std::function<std::vector<idx_type>()>& distances_provider,
                       const CubicGraph<idx_type>& graph) {
  std::chrono::steady_clock::time_point begin_time, end_time;
  begin_time = std::chrono::steady_clock::now();
  auto distances = distances_provider();
  end_time = std::chrono::steady_clock::now();
  std::cout << "Algorithm found valid distances: " << validate_cube_distances(distances, graph) << std::endl;
  return std::chrono::duration_cast<std::chrono::seconds>(end_time - begin_time).count();
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Missing parallelism parameter. Example: ./" << argv[0] << " 4" << std::endl;
    return 1;
  }
  size_t parallelism = strtoull(argv[1], nullptr, 10);
  const uint32_t side = 500;
  CubicGraph graph(side);
  std::cout << std::boolalpha;
  int64_t experiment_time;
  experiment_time = run_experiment<uint32_t>([&graph] { return graph.bfs_distances(0); }, graph);
  std::cout << "Single threaded result: " << experiment_time << "[s]\n";
  if (parallelism) {
    experiment_time =
        run_experiment<uint32_t>([&graph, parallelism] { return graph.pbfs_distances(0, parallelism); }, graph);
    std::cout << "Multi threaded result: " << experiment_time << "[s]\n";
  }
  return 0;
}
