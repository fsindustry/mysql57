//
// Created by fsindustry on 2023/10/23.
//

#include "throttler.h"
#include <vector>
#include <memory>
#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>


/**
 * define a throttling rule
 */
class keywords_rule {
public:
  std::string id; // identify a unique rule
  std::string keywords; // delimited by ~, example: A~B~C
  int32 max_concurrency;
  uint32 current_concurrency;
  uint64 reject_count;

  // todo add compiled regex here ...
};

typedef boost::unordered_map<std::string, keywords_rule> rule_map_t;

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

public:
  // rwlock is used to lock all rules when update
  mysql_rwlock_t keywords_rule_lock;

  // map is used to store rules
  // format: map( id, keywords_rule* )
  boost::shared_ptr<rule_map_t> rule_map;
};

/**
 * keywords throttle implementation
 */
class keywords_throttler : public throttler {
public:
  keywords_throttler();

  virtual ~keywords_throttler();

  int check_before_execute(THD *thd, const mysql_event_query *event);

  int adjust_after_execute(THD *thd, const mysql_event_query *event);

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
