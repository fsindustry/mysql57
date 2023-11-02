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


keywords_rule::keywords_rule()
    : rule_type(RULETYPE_UNSUPPORT), max_concurrency(0) {
}

keywords_rule::keywords_rule(std::string id, keywords_rule_type rule_type, std::string keywords, int32 max_concurrency)
    : id(std::move(id)), rule_type(rule_type), keywords(std::move(keywords)), max_concurrency(max_concurrency) {
}

int keywords_rule_mamager::add_rules(const std::vector<keywords_rule> *rules) {

  auto_rw_lock_write write_lock(&keywords_rule_lock);

  for (const keywords_rule &rule: *rules) {
    if (rule_map->find(rule.id) != rule_map->end()) {
      (*rule_map)[rule.id] = rule;
    } else {
      rule_map->emplace(rule.id, rule);
    }
  }

  return 0;
}


int keywords_rule_mamager::delete_rules(std::vector<std::string> *ids) {

  auto_rw_lock_write write_lock(&keywords_rule_lock);

  for (const std::string &id: *ids) {
    rule_map->erase(id);
  }

  return 0;
}

int keywords_rule_mamager::truncate_rules() {
  auto_rw_lock_write write_lock(&keywords_rule_lock);
  rule_map = std::make_shared<rule_map_t>();
  return 0;
}

std::vector<keywords_rule> keywords_rule_mamager::get_rules(const std::vector<std::string> *ids) {

  auto_rw_lock_read read_lock(&keywords_rule_lock);

  std::vector<keywords_rule> result;
  for (const auto &id: *ids) {
    auto it = rule_map->find(id);
    if (it != rule_map->end()) {
      result.push_back(it->second);
    }
  }

  return result;
}

std::vector<keywords_rule> keywords_rule_mamager::get_all_rules() {

  auto_rw_lock_read read_lock(&keywords_rule_lock);

  std::vector<keywords_rule> result;
  for (const auto &pair: *rule_map) {
    result.push_back(pair.second);
  }

  return result;
}

keywords_rule_mamager::keywords_rule_mamager() : rule_changed(false) {
  this->rule_map = std::make_shared<rule_map_t>();
  mysql_rwlock_init(key_keywords_throttler_lock_rules_, &keywords_rule_lock);
}

keywords_rule_mamager::~keywords_rule_mamager() {
  mysql_rwlock_destroy(&keywords_rule_lock);
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

