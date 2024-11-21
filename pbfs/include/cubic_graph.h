#ifndef CUBIC_GRAPH_H
#define CUBIC_GRAPH_H

#include "dense_graph.h"

template <std::unsigned_integral vertex_idx_type>
class CubicGraph : public DenseGraph<vertex_idx_type> {
 public:
  CubicGraph(vertex_idx_type side);
  inline std::tuple<vertex_idx_type, vertex_idx_type, vertex_idx_type> idx_1dto3d(vertex_idx_type vertex_idx) const;
  inline auto idx_3dto1d(vertex_idx_type vertex_x, vertex_idx_type vertex_y, vertex_idx_type vertex_z) const;

 private:
  auto cubic_graph_edges_getter(vertex_idx_type vertex) const;

  static vertex_idx_type minus_one_;

  vertex_idx_type side_;
};

template <std::unsigned_integral vertex_idx_type>
CubicGraph<vertex_idx_type>::CubicGraph(vertex_idx_type side)
    : side_(side), DenseGraph<vertex_idx_type>(side * side * side, [this](vertex_idx_type vertex) {
      return this->cubic_graph_edges_getter(vertex);
    }){};

template <std::unsigned_integral vertex_idx_type>
std::tuple<vertex_idx_type, vertex_idx_type, vertex_idx_type> CubicGraph<vertex_idx_type>::idx_1dto3d(
    vertex_idx_type vertex_idx) const {
  return {vertex_idx % side_, vertex_idx / side_ % side_, vertex_idx / side_ / side_};
}

template <std::unsigned_integral vertex_idx_type>
auto CubicGraph<vertex_idx_type>::idx_3dto1d(vertex_idx_type vertex_x, vertex_idx_type vertex_y,
                                             vertex_idx_type vertex_z) const {
  return vertex_z * side_ * side_ + vertex_y * side_ + vertex_x;
}

template <std::unsigned_integral vertex_idx_type>
auto CubicGraph<vertex_idx_type>::cubic_graph_edges_getter(vertex_idx_type vertex) const {
  using Edge = DenseGraph<vertex_idx_type>::Edge;
  std::vector<Edge> edges;
  edges.reserve(6);

  auto [x, y, z] = idx_1dto3d(vertex);
  if (x) {
    edges.emplace_back(Edge{idx_3dto1d(x - 1, y, z), 1});
  }
  if (y) {
    edges.emplace_back(Edge{idx_3dto1d(x, y - 1, z), 1});
  }
  if (z) {
    edges.emplace_back(Edge{idx_3dto1d(x, y, z - 1), 1});
  }
  if (x + 1 != side_) {
    edges.emplace_back(Edge{idx_3dto1d(x + 1, y, z), 1});
  }
  if (y + 1 != side_) {
    edges.emplace_back(Edge{idx_3dto1d(x, y + 1, z), 1});
  }
  if (z + 1 != side_) {
    edges.emplace_back(Edge{idx_3dto1d(x, y, z + 1), 1});
  }

  return edges;
}

#endif  // CUBIC_GRAPH_H
