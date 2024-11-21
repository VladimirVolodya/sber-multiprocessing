#ifndef GRAPH_H
#define GRAPH_H

#include "pram_executor.h"

#include <vector>
#include <cstdint>
#include <functional>
#include <queue>
#include <limits>
#include <concepts>
#include <iostream>
#include <format>
#include <sstream>

template <std::unsigned_integral vertex_idx_type, std::unsigned_integral weight_type = vertex_idx_type>
class DenseGraph {
 private:
  struct PbfsResult {
    std::vector<vertex_idx_type> next_vertices;
  };

 public:
  struct Edge {
    vertex_idx_type to;
    weight_type weight;
  };

  DenseGraph(vertex_idx_type size, const std::function<std::vector<Edge>(vertex_idx_type)>& edges_getter)
      : size_(size), edges_getter_(edges_getter) {
  }

  DenseGraph(vertex_idx_type size, std::function<std::vector<Edge>(vertex_idx_type)>&& edges_getter)
      : size_(size), edges_getter_(std::move(edges_getter)) {
  }

  auto bfs_distances(vertex_idx_type from) {
    std::vector<weight_type> distances(size_, std::numeric_limits<weight_type>::max());
    distances[from] = 0;
    bfs(from, [&distances](vertex_idx_type idx, const std::vector<Edge>& edges) {
      for (const auto& e : edges) {
        distances[e.to] = std::min(distances[e.to], distances[idx] + e.weight);
      }
    });
    return distances;
  }

  auto pbfs_distances(vertex_idx_type from, size_t parallelism = 4) {
    std::vector<weight_type> distances(size_, std::numeric_limits<weight_type>::max());
    distances[from] = 0;
    pbfs(
        from,
        [&distances](vertex_idx_type idx, const std::vector<Edge>& edges) {
          for (const auto& e : edges) {
            distances[e.to] = std::min(distances[e.to], distances[idx] + e.weight);
          }
        },
        parallelism);
    return distances;
  }

  void bfs(vertex_idx_type start, const std::function<void(vertex_idx_type, const std::vector<Edge>&)>& visitor) {
    std::queue<vertex_idx_type> bfs_queue;
    std::vector<bool> visited(size_, false);

    bfs_queue.emplace(start);

    while (!bfs_queue.empty()) {
      vertex_idx_type cur = bfs_queue.front();
      bfs_queue.pop();
      if (visited[cur]) {
        continue;
      }
      auto edges = edges_getter_(cur);
      visitor(cur, edges);
      visited[cur] = true;
      for (const auto& e : edges) {
        // if (!visited[e.to]) {
        bfs_queue.emplace(e.to);
        // }
      }
      // std::cout << std::format("Processed {}, bfs queue size {}\n", cur, bfs_queue.size());
    }
  }

  void pbfs(vertex_idx_type start, const std::function<void(vertex_idx_type, const std::vector<Edge>&)>& visitor,
            size_t parallelism) {
    PRAMExecutor executor(parallelism);
    std::vector<std::atomic_bool> visited(size_);
    std::unordered_map<std::thread::id, size_t> tid_to_local_idx;
    std::vector<std::vector<vertex_idx_type>> cur_level(parallelism);
    std::vector<std::vector<vertex_idx_type>> next_level(parallelism);

    for (auto& atomic : visited) {
      std::atomic_init(&atomic, false);
    }
    visited[start].store(true, std::memory_order_acquire);

    tid_to_local_idx.reserve(parallelism);
    cur_level.front().emplace_back(start);
    size_t level_size = 1;

    auto tids = executor.get_tids();
    for (size_t i = 0; i < parallelism; ++i) {
      tid_to_local_idx[tids[i]] = i;
    }

    while (level_size) {
      // std::cout << "Visited: ";
      // for (const auto& v : visited) {
      //   std::cout << v.load() << " ";
      // }
      // std::cout << "\n";
      // std::cout << "Levels:\n";
      // for (size_t i = 0; i < cur_level.size(); ++i) {
      //   std::cout << "#" << i << ": ";
      //   for (const auto& v : cur_level[i]) {
      //     std::cout << v << " ";
      //   }
      //   std::cout << "\n";
      // }
      // std::cout << "\n";

      executor.pfor<size_t>(0ull, level_size,
                            [&visited, &tid_to_local_idx, &cur_level, &next_level, &visitor, this](auto idx) {
                              this->pbfs_body(idx, visited, tid_to_local_idx, cur_level, next_level, visitor);
                            });

      cur_level = std::move(next_level);
      next_level.resize(parallelism);
      level_size = 0;
      for (const auto& level_part : cur_level) {
        level_size += level_part.size();
      }
    }

    executor.finish_all();
  }

 private:
  void pbfs_body(size_t idx, std::vector<std::atomic_bool>& visited,
                 const std::unordered_map<std::thread::id, size_t>& tid_to_local_idx,
                 const std::vector<std::vector<vertex_idx_type>>& cur_level,
                 std::vector<std::vector<vertex_idx_type>>& next_level,
                 const std::function<void(vertex_idx_type, const std::vector<Edge>&)>& visitor) {
    for (const auto& level_part : cur_level) {
      if (idx >= level_part.size()) {
        idx -= level_part.size();
        continue;
      }
      vertex_idx_type cur = level_part[idx];
      size_t local_tid = tid_to_local_idx.at(std::this_thread::get_id());

      // if (visited[cur].load(std::memory_order_release)) {
      //   return;
      // }
      auto edges = edges_getter_(cur);
      visitor(cur, edges);
      // visited[cur] = true;
      for (const auto& e : edges) {
        bool expected = false;
        if (visited[e.to].compare_exchange_strong(expected, true, std::memory_order_acquire,
                                                  std::memory_order_relaxed)) {
          next_level[local_tid].emplace_back(e.to);
        }
      }

      return;
    }
  }

  vertex_idx_type size_;
  std::function<std::vector<Edge>(vertex_idx_type)> edges_getter_;
};

#endif  // GRAPH_H
