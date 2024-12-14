#ifndef TATAS_LOCK_H
#define TATAS_LOCK_H

#include <atomic>

class TATASLock {
 public:
  TATASLock() : locked_(false) {
  }

  void lock() {
    while (locked_.load() || locked_.exchange(true)) {
    }
  }

  void unlock() {
    locked_.store(false);
  }

 private:
  std::atomic_bool locked_;
};

#endif  // TATAS_LOCK_H
