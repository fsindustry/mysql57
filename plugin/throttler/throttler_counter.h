//
// Created by fsindustry on 2023/11/2.
//

#include <atomic>
#include <memory>
#include "my_global.h"

class base_counter {
public:
  virtual uint64 get() = 0;

  virtual uint64 inc() = 0;

  virtual uint64 dec() = 0;

  virtual void reset() = 0;

  base_counter() = default;

  virtual ~base_counter() = default;
};


/**
 * counter implemented by atomic variable.
 */
class atomic_counter : public base_counter {
private:
  std::atomic<uint64> counter{};
public:
  uint64 get() override {
    return counter.load(std::memory_order_acquire);
  };

  uint64 inc() override {
    return ++counter;
  };

  uint64 dec() override {
    return --counter;
  };

  void reset() override {
    counter = 0;
  };

  atomic_counter() : counter(0) {};

  explicit atomic_counter(const uint64 count) : counter(count) {}

  atomic_counter(atomic_counter &&other) noexcept {
    this->counter = other.get();
  }

  atomic_counter &operator=(atomic_counter &&other) {
    this->counter = other.get();
    return *this;
  }

  ~atomic_counter() override = default;
};