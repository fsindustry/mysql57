//
// Created by fsindustry on 2023/10/23.
//

#include <utility>
#include "keywords_throttler.h"

#ifdef HAVE_PSI_INTERFACE
PSI_rwlock_key key_keywords_throttler_lock_rules_;

static PSI_rwlock_info all_keywords_throttler_rwlocks[] =
    {
        {&key_keywords_throttler_lock_rules_, "LOCK_plugin_keywords_throttler_rules_", 0}
    };

static void init_keywords_throttle_psi_keys() {
  const char *category = "keywords_throttler";
  int count;
  count = array_elements(all_keywords_throttler_rwlocks);
  mysql_rwlock_register(category, all_keywords_throttler_rwlocks, count);
}

#endif

keywords_rule::keywords_rule() : id(""),
                                 sql_type(keywords_sql_type()),
                                 keywords(""),
                                 regex(""),
                                 max_concurrency(0),
                                 concurrency_counter(new atomic_counter(0)),
                                 reject_counter(new atomic_counter(0)),
                                 throttled(false) {}

keywords_rule::keywords_rule(const keywords_rule &other)
    : id(other.id),
      sql_type(other.sql_type),
      keywords(other.keywords),
      regex(other.regex),
      max_concurrency(other.max_concurrency),
      throttled(other.throttled.load()) {

  if (other.concurrency_counter) {
    concurrency_counter = std::unique_ptr<base_counter>(new atomic_counter(other.concurrency_counter->get()));
  } else {
    concurrency_counter = nullptr;
  }

  if (other.reject_counter) {
    reject_counter = std::unique_ptr<base_counter>(new atomic_counter(other.reject_counter->get()));
  } else {
    reject_counter = nullptr;
  }
}

keywords_rule::keywords_rule(keywords_rule &&other) noexcept
    : id(std::move(other.id)),
      sql_type(other.sql_type),
      keywords(std::move(other.keywords)),
      regex(std::move(other.regex)),
      max_concurrency(other.max_concurrency),
      concurrency_counter(std::move(other.concurrency_counter)),
      reject_counter(std::move(other.reject_counter)),
      throttled(other.throttled.load()) {}

keywords_rule &keywords_rule::operator=(const keywords_rule &other) {
  if (this != &other) {
    id = other.id;
    sql_type = other.sql_type;
    keywords = other.keywords;
    regex = other.regex;
    max_concurrency = other.max_concurrency;
    throttled.store(other.throttled.load());

    if (other.concurrency_counter) {
      concurrency_counter = std::unique_ptr<base_counter>(new atomic_counter(other.concurrency_counter->get()));
    } else {
      concurrency_counter = nullptr;
    }

    if (other.reject_counter) {
      reject_counter = std::unique_ptr<base_counter>(new atomic_counter(other.reject_counter->get()));
    } else {
      reject_counter = nullptr;
    }
  }
  return *this;
}

keywords_rule &keywords_rule::operator=(keywords_rule &&other) noexcept {
  if (this != &other) {
    id = std::move(other.id);
    sql_type = other.sql_type;
    keywords = std::move(other.keywords);
    regex = std::move(other.regex);
    max_concurrency = other.max_concurrency;
    concurrency_counter = std::move(other.concurrency_counter);
    reject_counter = std::move(other.reject_counter);
    throttled.store(other.throttled.load());
  }
  return *this;
}

keywords_rule_shard::keywords_rule_shard() {
  sql_type = keywords_sql_type();
  rule_map = std::make_shared<rule_map_t>();
  mysql_rwlock_init(key_keywords_throttler_lock_rules_, &shard_lock);
}

keywords_rule_shard::keywords_rule_shard(const keywords_rule_shard &other) {
  sql_type = other.sql_type;
  rule_map = other.rule_map;
  mysql_rwlock_init(key_keywords_throttler_lock_rules_, &shard_lock);
}

keywords_rule_shard::keywords_rule_shard(keywords_rule_shard &&other) noexcept {
  sql_type = other.sql_type;
  rule_map = std::move(other.rule_map);
  mysql_rwlock_init(key_keywords_throttler_lock_rules_, &shard_lock);
}

keywords_rule_shard &keywords_rule_shard::operator=(const keywords_rule_shard &other) {
  if (this != &other) {
    sql_type = other.sql_type;
    rule_map = other.rule_map;
    mysql_rwlock_init(key_keywords_throttler_lock_rules_, &shard_lock);
  }
  return *this;
}

keywords_rule_shard &keywords_rule_shard::operator=(keywords_rule_shard &&other) noexcept {
  if (this != &other) {
    sql_type = other.sql_type;
    rule_map = std::move(other.rule_map);
    mysql_rwlock_init(key_keywords_throttler_lock_rules_, &shard_lock);
  }
  return *this;
}

keywords_rule_shard::~keywords_rule_shard() {
  mysql_rwlock_destroy(&shard_lock);
}

int keywords_rule_shard::add_rules(const std::vector<std::shared_ptr<keywords_rule>> *rules) {

  auto_rw_lock_write write_lock(&shard_lock);

  for (const std::shared_ptr<keywords_rule> &rule: *rules) {
    if (rule_map->find(rule->id) != rule_map->end()) {
      (*rule_map)[rule->id] = rule;
    } else {
      rule_map->emplace(rule->id, rule);
    }
  }

  return 0;
}


int keywords_rule_shard::delete_rules(std::vector<std::string> *ids) {

  auto_rw_lock_write write_lock(&shard_lock);

  for (const std::string &id: *ids) {
    rule_map->erase(id);
  }

  return 0;
}

int keywords_rule_shard::truncate_rules() {
  auto_rw_lock_write write_lock(&shard_lock);
  rule_map = std::make_shared<rule_map_t>();
  return 0;
}

std::vector<std::shared_ptr<keywords_rule>> keywords_rule_shard::get_rules(const std::vector<std::string> *ids) {

  auto_rw_lock_read read_lock(&shard_lock);

  std::vector<std::shared_ptr<keywords_rule>> result;
  for (const auto &id: *ids) {
    auto it = rule_map->find(id);
    if (it != rule_map->end()) {
      result.push_back(it->second);
    }
  }

  return result;
}

std::vector<std::shared_ptr<keywords_rule>> keywords_rule_shard::get_all_rules() {

  auto_rw_lock_read read_lock(&shard_lock);

  std::vector<std::shared_ptr<keywords_rule>> result;
  for (const auto &pair: *rule_map) {
    result.push_back(pair.second);
  }

  return result;
}

keywords_rule_mamager::keywords_rule_mamager() {
  rule_shard_map = std::make_shared<rule_shard_map_t>();
  rule_shard_map->emplace(RULETYPE_SELECT, std::make_shared<keywords_rule_shard>());
  rule_shard_map->emplace(RULETYPE_INSERT, std::make_shared<keywords_rule_shard>());
  rule_shard_map->emplace(RULETYPE_UPDATE, std::make_shared<keywords_rule_shard>());
  rule_shard_map->emplace(RULETYPE_DELETE, std::make_shared<keywords_rule_shard>());
  rule_shard_map->emplace(RULETYPE_REPLACE, std::make_shared<keywords_rule_shard>());
}

int keywords_rule_mamager::add_rules(const std::vector<std::shared_ptr<keywords_rule>> *rules) {

  if (rules->empty()) {
    return 0;
  }

  // list rules by sql type
  std::unordered_map<keywords_sql_type, std::vector<std::shared_ptr<keywords_rule>>> tmp_map;
  for (auto &rule: *rules) {
    if (rule->sql_type == RULETYPE_UNSUPPORT) {
      continue;
    }

    if (tmp_map.find(rule->sql_type) == tmp_map.end()) {
      tmp_map.emplace(rule->sql_type, std::vector<std::shared_ptr<keywords_rule>>{});
    }
    tmp_map[rule->sql_type].push_back(rule);
  }

  if (tmp_map.empty()) {
    return 0;
  }

  // add rules by shard
  for (auto &pair: tmp_map) {
    auto iter = rule_shard_map->find(pair.first);
    if (iter == rule_shard_map->end()) {
      continue;
    }
    std::shared_ptr<keywords_rule_shard> rule_shard = iter->second;
    rule_shard->add_rules(&pair.second);
  }

  return 0;
}

int keywords_rule_mamager::delete_rules(std::vector<std::string> *ids) {

  // query rules by id, and list rules by sql type
  // map( sql type, ids )
  std::unordered_map<keywords_sql_type, std::vector<std::string>> tmp_map;
  for (auto &pair: *rule_shard_map) {
    std::shared_ptr<keywords_rule_shard> rule_shard = pair.second;
    std::vector<std::shared_ptr<keywords_rule>> rules = rule_shard->get_rules(ids);
    if (rules.empty()) {
      continue;
    }

    // list rules by sql type
    for (auto &rule: rules) {
      if (rule->sql_type == RULETYPE_UNSUPPORT) {
        continue;
      }

      if (tmp_map.find(rule->sql_type) == tmp_map.end()) {
        tmp_map.emplace(rule->sql_type, std::vector<std::string>{});
      }
      tmp_map[rule->sql_type].push_back(rule->id);
    }
  }

  // delete rules by shard
  for (auto &pair: tmp_map) {
    auto iter = rule_shard_map->find(pair.first);
    if (iter == rule_shard_map->end()) {
      continue;
    }
    std::shared_ptr<keywords_rule_shard> rule_shard = iter->second;
    rule_shard->delete_rules(&pair.second);
  }

  return 0;
}

int keywords_rule_mamager::truncate_rules() {
  // truncate rules by shard
  for (auto &pair: *rule_shard_map) {
    std::shared_ptr<keywords_rule_shard> rule_shard = pair.second;
    rule_shard->truncate_rules();
  }
  return 0;
}

std::vector<std::shared_ptr<keywords_rule>> keywords_rule_mamager::get_rules(const std::vector<std::string> *ids) {
  // query rules by shard
  std::vector<std::shared_ptr<keywords_rule>> result;
  for (auto &pair: *rule_shard_map) {
    std::shared_ptr<keywords_rule_shard> rule_shard = pair.second;
    std::vector<std::shared_ptr<keywords_rule>> rules = rule_shard->get_rules(ids);
    if (rules.empty()) {
      continue;
    }
    result.emplace(rules.begin());
  }

  return result;
}

std::vector<std::shared_ptr<keywords_rule>> keywords_rule_mamager::get_all_rules() {
  // query all rules by shard
  std::vector<std::shared_ptr<keywords_rule>> result;
  for (auto &pair: *rule_shard_map) {
    std::shared_ptr<keywords_rule_shard> rule_shard = pair.second;
    std::vector<std::shared_ptr<keywords_rule>> rules = rule_shard->get_all_rules();
    if (rules.empty()) {
      continue;
    }
    result.emplace(rules.begin());
  }

  return result;
}

keywords_throttler::keywords_throttler() {
#ifdef HAVE_PSI_INTERFACE
  init_keywords_throttle_psi_keys();
#endif
  mamager = new keywords_rule_mamager;
}

keywords_throttler::~keywords_throttler() {
  delete mamager;
}

int keywords_throttler::check_before_execute(THD *thd, const mysql_event_query *event) {
  return 0;
}

int keywords_throttler::adjust_after_execute(THD *thd, const mysql_event_query *event) {
  return 0;
}
