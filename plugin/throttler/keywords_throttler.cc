//
// Created by fsindustry on 2023/10/23.
//

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


int keywords_rule_mamager::add_rules(const std::vector<keywords_rule> *rules) {

  std::shared_ptr<rule_map_t> new_rule_map;

  {
    // Create a copy of the current rule_map
    auto_rw_lock_read read_lock(&keywords_rule_lock);
    new_rule_map = rule_map;
  }

  for (const keywords_rule& rule : *rules) {
    new_rule_map->emplace(rule.id, rule);
  }

  {
    auto_rw_lock_write write_lock(&keywords_rule_lock);
    rule_map = new_rule_map;
  }

  return 0;
}


int keywords_rule_mamager::delete_rules(std::vector<std::string> *ids) {

  std::shared_ptr<rule_map_t> new_rule_map;

  {
    // Create a copy of the current rule_map
    auto_rw_lock_read read_lock(&keywords_rule_lock);
    new_rule_map = rule_map;
  }

  for (const std::string& id : *ids) {
    new_rule_map->erase(id);
  }

  {
    auto_rw_lock_write write_lock(&keywords_rule_lock);
    rule_map = new_rule_map;
  }

  return 0;
}

int keywords_rule_mamager::truncate_rules() {
  auto_rw_lock_write write_lock(&keywords_rule_lock);
  rule_map = std::make_shared<rule_map_t>();
}

std::vector<keywords_rule> keywords_rule_mamager::get_rules(const std::vector<std::string> *ids) {

  auto_rw_lock_read read_lock(&keywords_rule_lock);

  std::vector<keywords_rule> result;
  for (const auto &id : *ids) {
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
  for (const auto& pair : *rule_map) {
    result.push_back(pair.second);
  }

  return result;
}


keywords_throttler::keywords_throttler() {
#ifdef HAVE_PSI_INTERFACE
  init_keywords_throttle_psi_keys();
#endif
  mamager = new keywords_rule_mamager;
  mysql_rwlock_init(key_keywords_throttler_lock_rules_, &mamager->keywords_rule_lock);
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

