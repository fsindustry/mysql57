//
// Created by fsindustry on 2023/10/23.
//

#include "throttler.h"
#include <unordered_map>
#include <vector>
#include <atomic>
#include <memory>

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


/**
 * define a throttling rule
 */
class keywords_rule {
public:
  std::string id; // identify a unique rule
  std::string keywords; // delimited by ~, example: A~B~C
  int32 max_concurrency;

  // todo add compiled regex here ...

  keywords_rule();

  keywords_rule(std::string id,
                std::string keywords,
                int32 max_concurrency);

  virtual ~keywords_rule() = default;
};

typedef std::unordered_map<std::string, keywords_rule> rule_map_t;

/**
 * define how to manage throttling rules
 */
class keywords_rule_mamager {
public:
  int add_rules(const std::vector<keywords_rule> *rules);

  int delete_rules(std::vector<std::string> *ids);

  int truncate_rules();

  std::vector<keywords_rule> get_rules(const std::vector<std::string> *ids);

  std::vector<keywords_rule> get_all_rules();

  keywords_rule_mamager();

public:
  // rwlock is used to lock all rules when update
  mysql_rwlock_t keywords_rule_lock;

  // map is used to store rules
  // format: map( id, keywords_rule* )
  std::shared_ptr<rule_map_t> rule_map;

  // set true if rule changed
  std::atomic<bool> rule_changed;
};

/**
 * keywords throttle implementation
 */
class keywords_throttler : public throttler {
public:
  keywords_throttler();

  virtual ~keywords_throttler();

  int check_before_execute(THD *thd, const mysql_event_query *event) override;

  int adjust_after_execute(THD *thd, const mysql_event_query *event) override;

  inline keywords_rule_mamager *getMamager() const {
    return mamager;
  }

private:
  keywords_rule_mamager *mamager;
};

/**
 * define a read lock guard for mysql_rwlock_t
 */
class auto_rw_lock_read {
public:
  explicit auto_rw_lock_read(mysql_rwlock_t *lock) : rw_lock(NULL) {
    if (lock && 0 == mysql_rwlock_rdlock(lock))
      rw_lock = lock;
  }

  ~auto_rw_lock_read() {
    if (rw_lock)
      mysql_rwlock_unlock(rw_lock);
  }

private:
  mysql_rwlock_t *rw_lock;

  auto_rw_lock_read(const auto_rw_lock_read &);         /* Not copyable. */
  void operator=(const auto_rw_lock_read &);            /* Not assignable. */
};

/**
 * define a write lock guard for mysql_rwlock_t
 */
class auto_rw_lock_write {
public:
  explicit auto_rw_lock_write(mysql_rwlock_t *lock) : rw_lock(NULL) {
    if (lock && 0 == mysql_rwlock_wrlock(lock))
      rw_lock = lock;
  }

  ~auto_rw_lock_write() {
    if (rw_lock)
      mysql_rwlock_unlock(rw_lock);
  }

private:
  mysql_rwlock_t *rw_lock;

  auto_rw_lock_write(const auto_rw_lock_write &);        /* Non-copyable */
  void operator=(const auto_rw_lock_write &);            /* Non-assignable */
};


