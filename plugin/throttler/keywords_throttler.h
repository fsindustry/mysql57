//
// Created by fsindustry on 2023/10/23.
//

#include "throttler.h"
#include "throttler_counter.h"
#include <unordered_map>
#include <vector>

/**
 * define keywords rule type
 */
enum keywords_rule_type {
  RULETYPE_UNSUPPORT, // put this to the head of keywords_rule_type
  RULETYPE_SELECT,
  RULETYPE_INSERT,
  RULETYPE_UPDATE,
  RULETYPE_DELETE,
  RULETYPE_REPLACE
};

/**
 * define a throttling rule
 */
class keywords_rule {
public:
  std::string id; // identify a unique rule
  keywords_rule_type rule_type; // SQL command type
  std::string keywords; // delimited by ~, example: A~B~C
  std::string regex; // regex that converted by keywords
  int32 max_concurrency; // max concurrency
  std::unique_ptr<base_counter> concurrency_counter;
  std::unique_ptr<base_counter> reject_counter;
  std::atomic<bool> throttled; // flag to indicate whether triggerred the rule to limit traffic.

  keywords_rule();

  keywords_rule(const keywords_rule &other);

  keywords_rule(keywords_rule &&other) noexcept;

  keywords_rule &operator=(const keywords_rule &other);

  keywords_rule &operator=(keywords_rule &&other) noexcept;

  // 析构函数
  virtual ~keywords_rule() = default;
};

typedef std::unordered_map<std::string, std::shared_ptr<keywords_rule>> rule_map_t;

/**
 * define how to manage throttling rules
 */
class keywords_rule_mamager {
private:

/**
 * rule type name to rule type enum map
 */
  std::unordered_map<std::string, keywords_rule_type> name_rule_type_mapper = {
      {"select",  RULETYPE_SELECT},
      {"insert",  RULETYPE_INSERT},
      {"update",  RULETYPE_UPDATE},
      {"delete",  RULETYPE_DELETE},
      {"replace", RULETYPE_REPLACE}
  };

  /**
 * rule type enum to rule type name map
 */
  std::unordered_map<uint32, std::string> rule_type_name_mapper = {
      {RULETYPE_UNSUPPORT, "unsupport"},
      {RULETYPE_SELECT,    "select"},
      {RULETYPE_INSERT,    "insert"},
      {RULETYPE_UPDATE,    "update"},
      {RULETYPE_DELETE,    "delete"},
      {RULETYPE_REPLACE,   "replace"}
  };

/**
 * sql command type to rule type enum map
 */
  std::unordered_map<uint32, keywords_rule_type> sql_cmd_rule_type_mapper = {
      {SQLCOM_SELECT,         RULETYPE_SELECT},
      {SQLCOM_INSERT,         RULETYPE_INSERT},
      {SQLCOM_INSERT_SELECT,  RULETYPE_INSERT},
      {SQLCOM_UPDATE,         RULETYPE_UPDATE},
      {SQLCOM_UPDATE_MULTI,   RULETYPE_UPDATE},
      {SQLCOM_DELETE,         RULETYPE_DELETE},
      {SQLCOM_DELETE_MULTI,   RULETYPE_DELETE},
      {SQLCOM_REPLACE,        RULETYPE_REPLACE},
      {SQLCOM_REPLACE_SELECT, RULETYPE_REPLACE}
  };
public:

  inline keywords_rule_type get_rule_type_by_name(const std::string &name) {
    auto it = name_rule_type_mapper.find(name);
    if (it != name_rule_type_mapper.end()) {
      return it->second;
    }
    return RULETYPE_UNSUPPORT;
  }

  inline std::string get_name_by_rule_type(keywords_rule_type type) {
    auto it = rule_type_name_mapper.find(type);
    if (it != rule_type_name_mapper.end()) {
      return it->second;
    }
    return rule_type_name_mapper[RULETYPE_UNSUPPORT];
  }

  inline keywords_rule_type get_rule_type_by_sql_cmd(enum_sql_command sql_cmd_type) {
    auto it = sql_cmd_rule_type_mapper.find(sql_cmd_type);
    if (it != sql_cmd_rule_type_mapper.end()) {
      return it->second;
    }
    return RULETYPE_UNSUPPORT;
  }

  inline bool valid_sql_cmd_type(enum_sql_command sql_cmd_type) {
    return get_rule_type_by_sql_cmd(sql_cmd_type) != RULETYPE_UNSUPPORT;
  }

  int add_rules(const std::vector<std::shared_ptr<keywords_rule>> *rules);

  int delete_rules(std::vector<std::string> *ids);

  int truncate_rules();

  std::vector<std::shared_ptr<keywords_rule>> get_rules(const std::vector<std::string> *ids);

  std::vector<std::shared_ptr<keywords_rule>> get_all_rules();

  keywords_rule_mamager();

  ~keywords_rule_mamager();

public:
  // rwlock is used to lock all rules when update
  mysql_rwlock_t keywords_rule_lock{};

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


