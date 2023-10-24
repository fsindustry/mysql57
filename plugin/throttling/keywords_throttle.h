//
// Created by fsindustry on 2023/10/23.
//

#include "throttle.h"
#include <unordered_map>
#include <vector>
#include <memory>

/**
 * define a throttling rule
 */
class keywords_rule {
public:
  std::string id; // identify a unique rule
  std::string keywords; // delimited by ~, example: A~B~C
  int max_concurrency;
  int current_concurrency;
  long reject_count;

  // todo add compiled regex here ...

  keywords_rule() = default;
  keywords_rule(const keywords_rule& other) = default;
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

private:

public:
  // rwlock is used to lock all rules when update
  mysql_rwlock_t keywords_rule_lock;

  // map is used to store rules
  // format: map( id, keywords_rule* )
  std::shared_ptr<rule_map_t> rule_map;
};

/**
 * keywords throttle implementation
 */
class keywords_throttle : public throttle {
public:
  keywords_throttle();

  virtual ~keywords_throttle();

  int check_before_execute(THD *thd, const mysql_event_query *event) override;

  int adjust_after_execute(THD *thd, const mysql_event_query *event) override;

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
