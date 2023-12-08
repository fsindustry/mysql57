//
// Created by fsindustry on 2023/10/23.
//

#include "throttler.h"
#include "throttler_counter.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <map>
#include <hs/hs.h>
#include <hs/hs_common.h>

/**
 * define keywords rule type
 */
enum keywords_sql_type {
  RULETYPE_UNSUPPORT, // put this to the head of keywords_sql_type
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
  keywords_sql_type sql_type; // SQL command type
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

  virtual ~keywords_rule() = default;
};

/**
 * store all rules for current sql type, map( rule id, rule )
 * we use ordered map to keep the order of rules by rule id here.
 */
typedef std::map<std::string, std::shared_ptr<keywords_rule>> rule_map_t;

/**
 * store compiled keywords rule regex
 */
class keywords_rule_database {
private:
  /**
   * when there is no rules for current database, just set access_all = true
   */
  bool access_all;

  /**
   * store relationships between hyperscan database indics and keywords rule ids.
   * format: map ( database index, rule id )
   */
  std::unordered_map<uint32_t, std::string> *id_map;

  /**
   * compiled hyperscan database object
   */
  hs_database_t *database;

public:
  keywords_rule_database();

  virtual ~keywords_rule_database();

  int init(const std::shared_ptr<rule_map_t> &rule_map);

  inline bool is_access_all() const {
    return access_all;
  }

  inline hs_database_t *get_database() const {
    return database;
  }

  inline std::string get_rule_id(uint32_t index) {
    auto iter = id_map->find(index);
    if (iter == id_map->end()) {
      return "";
    }
    return iter->second;
  }
};

/**
 * define a throttling rule shard
 * which divide keywords rules into different shard by sql type
 */
class keywords_rule_shard {
private:
  keywords_sql_type sql_type; // sql type to flag current shard
  std::shared_ptr<rule_map_t> rule_map; // store all rules for current sql type, map( rule id, rule )
  std::shared_ptr<keywords_rule_database> rule_database; // store compiled regex object here, we use hyperscan hs_database_t
  std::atomic<uint32_t> current_version; // record current version of compiled rules
  mysql_rwlock_t shard_lock; // rwlock to control rule changes in concurrent environment.

  inline void update_version() {
    current_version++;
  }

  int update_database();

public:

  keywords_rule_shard();

  keywords_rule_shard(const keywords_rule_shard &other);

  keywords_rule_shard(keywords_rule_shard &&other) noexcept;

  keywords_rule_shard &operator=(const keywords_rule_shard &other);

  keywords_rule_shard &operator=(keywords_rule_shard &&other) noexcept;

  virtual ~keywords_rule_shard();

  int add_rules(const std::vector<std::shared_ptr<keywords_rule>> *rules);

  int delete_rules(std::vector<std::string> *ids);

  int truncate_rules();

  std::vector<std::shared_ptr<keywords_rule>> get_rules(const std::vector<std::string> *ids);

  std::vector<std::shared_ptr<keywords_rule>> get_all_rules();

  std::shared_ptr<keywords_rule> get_rule_by_id(const std::string &rule_id);

  bool changed(uint32_t version);

  inline const std::atomic<uint32_t> &get_current_version() const {
    return current_version;
  }

  inline const std::shared_ptr<keywords_rule_database> &get_rule_database() const {
    return rule_database;
  }
};

typedef std::unordered_map<keywords_sql_type, std::shared_ptr<keywords_rule_shard>> rule_shard_map_t;

/**
 * define how to manage throttling rules
 */
class keywords_rule_mamager {
private:

/**
 * rule type name to rule type enum map
 */
  std::unordered_map<std::string, keywords_sql_type> name_to_sql_type_mapper = {
      {"select",  RULETYPE_SELECT},
      {"insert",  RULETYPE_INSERT},
      {"update",  RULETYPE_UPDATE},
      {"delete",  RULETYPE_DELETE},
      {"replace", RULETYPE_REPLACE}
  };

  /**
 * rule type enum to rule type name map
 */
  std::unordered_map<uint32, std::string> sql_type_to_name_mapper = {
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
  std::unordered_map<uint32, keywords_sql_type> sql_cmd_to_sql_type_mapper = {
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

  /**
   * map is used to store rule shard
   * map( sql_type, rule shard)
   */
  std::shared_ptr<rule_shard_map_t> rule_shard_map;

public:
  keywords_rule_mamager();

  virtual ~keywords_rule_mamager() = default;

  inline keywords_sql_type get_sql_type_by_name(const std::string &name) {
    auto it = name_to_sql_type_mapper.find(name);
    if (it != name_to_sql_type_mapper.end()) {
      return it->second;
    }
    return RULETYPE_UNSUPPORT;
  }

  inline std::string get_name_by_sql_type(keywords_sql_type type) {
    auto it = sql_type_to_name_mapper.find(type);
    if (it != sql_type_to_name_mapper.end()) {
      return it->second;
    }
    return sql_type_to_name_mapper[RULETYPE_UNSUPPORT];
  }

  inline keywords_sql_type get_sql_type_by_sql_cmd(enum_sql_command sql_cmd_type) {
    auto it = sql_cmd_to_sql_type_mapper.find(sql_cmd_type);
    if (it != sql_cmd_to_sql_type_mapper.end()) {
      return it->second;
    }
    return RULETYPE_UNSUPPORT;
  }

  inline bool valid_sql_cmd_type(enum_sql_command sql_cmd_type) {
    return get_sql_type_by_sql_cmd(sql_cmd_type) != RULETYPE_UNSUPPORT;
  }

  int add_rules(const std::vector<std::shared_ptr<keywords_rule>> *rules);

  int delete_rules(std::vector<std::string> *ids);

  int truncate_rules();

  std::vector<std::shared_ptr<keywords_rule>> get_rules(const std::vector<std::string> *ids);

  std::vector<std::shared_ptr<keywords_rule>> get_all_rules();

  inline std::shared_ptr<keywords_rule_shard> get_shard_by_sql_type(keywords_sql_type sql_type) {
    auto iter = rule_shard_map->find(sql_type);
    if (iter == rule_shard_map->end()) {
      return nullptr;
    }
    return iter->second;
  }
};


/**
 * rule context which used for rule matching and status store.
 */
class keywords_rule_context {
public:
  uint32_t current_version; // rule version for appointed sql type
  std::shared_ptr<keywords_rule_database> rule_database; // compiled database for appointed sql type
  hs_scratch_t *scratch; // scratch allocated for database to match rules.
  std::vector<std::string> matched_rule_ids;
  std::shared_ptr<keywords_rule> last_matched_rule;

  keywords_rule_context();

  virtual ~keywords_rule_context();

  int init(uint32_t version, const std::shared_ptr<keywords_rule_database> &database);

  int match(const char *query, size_t length);

  bool has_matched_rules();

  void clean_matched_rules();

  std::string get_first_matched_rule();
};

/**
 * store rule context by sql type
 */
typedef std::map<keywords_sql_type, std::shared_ptr<keywords_rule_context>> rule_context_map_t;

class throttler_thd_context {
public:
  bool whitelist_user;
  std::string user_name;
  rule_context_map_t *context_map;

  throttler_thd_context(bool whitelist_user,std::string user_name);

  ~throttler_thd_context();

  std::shared_ptr<keywords_rule_context> get_context_by_sql_type(keywords_sql_type sql_type);
};

/**
 * keywords throttle implementation
 */
class keywords_throttler : public throttler {
public:
  keywords_throttler();

  ~keywords_throttler() override;

  int after_thd_initialled(MYSQL_THD thd, const mysql_event_connection *event) override;

  int before_thd_destroyed(MYSQL_THD thd, const mysql_event_connection *event) override;

  int check_before_execute(THD *thd, const mysql_event_query *event) override;

  int adjust_after_execute(THD *thd, const mysql_event_query *event) override;

  inline keywords_rule_mamager *get_mamager() const {
    return mamager;
  }

  throttler_whitelist *get_whitelist() override;

  throttler_thd_context *get_thd_context();

  void set_thd_context(throttler_thd_context *ctx);

  std::string get_user(THD *thd);
  bool is_super_user(THD *thd);

private:

  void add_thd_context(THD *thd, std::string &curr_user);

private:
  keywords_rule_mamager *mamager;
  throttler_whitelist *whitelist;

};

/**
 * define a read lock guard for mysql_rwlock_t
 */
class auto_rw_lock_read {
public:
  auto_rw_lock_read(const auto_rw_lock_read &) = delete;         /* Not copyable. */
  void operator=(const auto_rw_lock_read &) = delete;            /* Not assignable. */
  explicit auto_rw_lock_read(mysql_rwlock_t *lock) : rw_lock(nullptr) {
    if (lock && 0 == mysql_rwlock_rdlock(lock))
      rw_lock = lock;
  }

  ~auto_rw_lock_read() {
    if (rw_lock)
      mysql_rwlock_unlock(rw_lock);
  }

private:
  mysql_rwlock_t *rw_lock;
};

/**
 * define a write lock guard for mysql_rwlock_t
 */
class auto_rw_lock_write {
public:
  auto_rw_lock_write(const auto_rw_lock_write &) = delete;        /* Non-copyable */
  void operator=(const auto_rw_lock_write &) = delete;            /* Non-assignable */
  explicit auto_rw_lock_write(mysql_rwlock_t *lock) : rw_lock(nullptr) {
    if (lock && 0 == mysql_rwlock_wrlock(lock))
      rw_lock = lock;
  }

  ~auto_rw_lock_write() {
    if (rw_lock)
      mysql_rwlock_unlock(rw_lock);
  }

private:
  mysql_rwlock_t *rw_lock;
};