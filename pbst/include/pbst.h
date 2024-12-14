#ifndef EXTERNAL_PBST_H
#define EXTERNAL_PBST_H

#include "ticket_lock.h"

#include <algorithm>
#include <concepts>
#include <condition_variable>
#include <limits>
#include <mutex>

template <std::totally_ordered Key>
class PBST {
 private:
  struct Node {
    Key key = std::numeric_limits<Key>::min();
    Node* p_left = nullptr;
    Node* p_right = nullptr;
    bool is_routing = true;
    // std::condition_variable cv;
    TATASLock lock;
    // std::mutex lock;
  };

 public:
  PBST();
  PBST(const PBST& that) = delete;
  PBST(PBST&& that) = delete;
  PBST<Key>& operator=(const PBST& that) = delete;
  PBST<Key>& operator=(PBST&& that) = delete;
  void Insert(const Key& key);
  void Insert(Key&& key);
  void Remove(const Key& key);
  bool Contains(const Key& key) const;
  void Insert(const Key& key, uint64_t& time);
  void Insert(Key&& key, uint64_t& time);
  void Remove(const Key& key, uint64_t& time);
  bool Contains(const Key& key, uint64_t& time) const;
  bool ValidBST() const;
  ~PBST();

 private:
  bool ValidSubtree(Node* node, const Key& lower_bound, const Key& upper_bound) const;
  void Clear(Node* node);
  Node* root_;
  mutable uint64_t timer_;
};

template <std::totally_ordered Key>
PBST<Key>::PBST() : root_(new Node()), timer_(0ull) {
}

template <std::totally_ordered Key>
void PBST<Key>::Insert(const Key& key) {
  uint64_t trash;
  Insert(key, trash);
}

template <std::totally_ordered Key>
void PBST<Key>::Insert(Key&& key) {
  uint64_t trash;
  Insert(key, trash);
}

template <std::totally_ordered Key>
void PBST<Key>::Remove(const Key& key) {
  uint64_t trash;
  Remove(key, trash);
}

template <std::totally_ordered Key>
bool PBST<Key>::Contains(const Key& key) const {
  uint64_t trash;
  return Contains(key, trash);
}

template <std::totally_ordered Key>
void PBST<Key>::Insert(const Key& key, uint64_t& time) {
  auto key_copy = key;
  Insert(std::move(key_copy), time);
}

template <std::totally_ordered Key>
void PBST<Key>::Insert(Key&& key, uint64_t& time) {
  auto prev = root_;
  prev->lock.lock();
  // std::unique_lock prev_lk(prev->lock);
  time = timer_++;
  auto cur = prev->p_right;
  if (!cur) {
    // empty tree
    prev->p_right = new Node();
    prev->p_right->key = std::move(key);
    prev->p_right->is_routing = false;
    prev->lock.unlock();
    return;
  }
  // std::unique_lock cur_lk(cur->lock);
  cur->lock.lock();
  while (cur->is_routing) {
    auto next = cur->key > key ? cur->p_left : cur->p_right;
    next->lock.lock();
    prev->lock.unlock();
    // std::unique_lock next_lk(next->lock);
    // prev_lk = std::move(cur_lk);
    // cur_lk = std::move(next_lk);
    prev = cur;
    cur = next;
  }
  if (cur->key == key) {
    // already in tree
    prev->lock.unlock();
    cur->lock.unlock();
    return;
  }
  auto new_node = new Node();
  auto new_routing = new Node();
  new_node->key = std::move(key);
  new_node->is_routing = false;
  new_routing->key = std::max(new_node->key, cur->key);
  new_routing->p_left = new_node->key < cur->key ? new_node : cur;
  new_routing->p_right = new_node->key < cur->key ? cur : new_node;
  (prev->p_left == cur ? prev->p_left : prev->p_right) = new_routing;
  prev->lock.unlock();
  cur->lock.unlock();
  return;
}

template <std::totally_ordered Key>
void PBST<Key>::Remove(const Key& key, uint64_t& time) {
  auto prev = root_;
  prev->lock.lock();
  // std::unique_lock prev_lk(prev->lock);
  time = timer_++;
  auto cur = prev->p_right;
  if (!cur) {
    // empty tree, nothing to remove
    prev->lock.unlock();
    return;
  }
  if (!cur->is_routing) {
    // single node in tree
    if (cur->key == key) {
      delete cur;
      prev->p_right = nullptr;
    }
    prev->lock.unlock();
    return;
  }
  // std::unique_lock cur_lk(cur->lock);
  cur->lock.lock();
  while (true) {
    auto next = cur->key > key ? cur->p_left : cur->p_right;
    if (!next->is_routing) {
      break;
    }
    next->lock.lock();
    prev->lock.unlock();
    // std::unique_lock next_lk(next->lock);
    // prev_lk = std::move(cur_lk);
    // cur_lk = std::move(next_lk);
    prev = cur;
    cur = next;
  }
  auto p_cur = prev->p_left == cur ? &(prev->p_left) : &(prev->p_right);
  if (cur->p_left && cur->p_left->key == key) {
    delete cur->p_left;
    *p_cur = cur->p_right;
    delete cur;
  } else if (cur->p_right && cur->p_right->key == key) {
    delete cur->p_right;
    *p_cur = cur->p_left;
    delete cur;
  } else {
    cur->lock.unlock();
  }
  prev->lock.unlock();
  return;
}

template <std::totally_ordered Key>
bool PBST<Key>::Contains(const Key& key, uint64_t& time) const {
  auto prev = root_;
  prev->lock.lock();
  // std::unique_lock prev_lk(prev->lock);
  time = timer_++;
  auto cur = prev->p_right;
  if (!cur) {
    // empty tree
    prev->lock.unlock();
    return false;
  }
  // std::unique_lock cur_lk(cur->lock);
  cur->lock.lock();
  while (cur->is_routing) {
    auto next = cur->key > key ? cur->p_left : cur->p_right;
    next->lock.lock();
    prev->lock.unlock();
    // std::unique_lock next_lk(next->lock);
    // prev_lk = std::move(cur_lk);
    // cur_lk = std::move(next_lk);
    prev = cur;
    cur = next;
  }
  prev->lock.unlock();
  cur->lock.unlock();
  return cur->key == key;
}

template <std::totally_ordered Key>
PBST<Key>::~PBST() {
  Clear(root_);
  root_ = nullptr;
}

template <std::totally_ordered Key>
void PBST<Key>::Clear(Node* node) {
  if (!node) {
    return;
  }
  Clear(node->p_left);
  Clear(node->p_right);
  node->p_left = node->p_right = nullptr;
  delete node;
}

template <std::totally_ordered Key>
bool PBST<Key>::ValidBST() const {
  std::unique_lock lk(root_->lock);
  return ValidSubtree(root_->p_right, std::numeric_limits<Key>::min(), std::numeric_limits<Key>::max());
}

template <std::totally_ordered Key>
bool PBST<Key>::ValidSubtree(Node* node, const Key& lower_bound, const Key& upper_bound) const {
  // all keys in subtree are (>= lower_bound && < upper_bound)
  if (!node) {
    return true;
  }
  std::unique_lock lk(node->lock);
  if (node->is_routing && !node->p_left && !node->p_right) {
    return false;
  }
  if (!node->is_routing && (node->p_left || node->p_right)) {
    return false;
  }
  return ValidSubtree(node->p_left, lower_bound, node->key) && ValidSubtree(node->p_right, node->key, upper_bound);
}

#endif  // EXTERNAL_PBST_H
