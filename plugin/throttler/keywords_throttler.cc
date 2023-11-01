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

  boost::shared_ptr<rule_map_t> new_rule_map;

  {
    // Create a copy of the current rule_map
    auto_rw_lock_read read_lock(&keywords_rule_lock);
    new_rule_map = rule_map;
  }

  for (std::vector<keywords_rule>::const_iterator it = rules->begin(); it != rules->end(); ++it) {
    new_rule_map->insert(std::make_pair(it->id, *it));
  }

  {
    auto_rw_lock_write write_lock(&keywords_rule_lock);
    rule_map = new_rule_map;
  }

  return 0;
}


int keywords_rule_mamager::delete_rules(std::vector<std::string> *ids) {

  boost::shared_ptr<rule_map_t> new_rule_map;

  {
    // Create a copy of the current rule_map
    auto_rw_lock_read read_lock(&keywords_rule_lock);
    new_rule_map = rule_map;
  }

  for (std::vector<std::string>::iterator it = ids->begin(); it != ids->end(); ++it) {
    new_rule_map->erase(*it);
  }

  {
    auto_rw_lock_write write_lock(&keywords_rule_lock);
    rule_map = new_rule_map;
  }

  return 0;
}

int keywords_rule_mamager::truncate_rules() {
  auto_rw_lock_write write_lock(&keywords_rule_lock);
  rule_map = boost::make_shared<rule_map_t>();
  return 0;
}

std::vector<keywords_rule> keywords_rule_mamager::get_rules(const std::vector<std::string> *ids) {

  auto_rw_lock_read read_lock(&keywords_rule_lock);

  std::vector<keywords_rule> result;
  for (std::vector<std::string>::const_iterator it = ids->begin(); it != ids->end(); ++it) {
    boost::unordered::unordered_map<std::string,keywords_rule>::iterator map_it = rule_map->find(*it);
    if (map_it != rule_map->end()) {
      result.push_back(map_it->second);
    }
  }

  return result;
}

std::vector<keywords_rule> keywords_rule_mamager::get_all_rules() {

  auto_rw_lock_read read_lock(&keywords_rule_lock);

  std::vector<keywords_rule> result;

  boost::unordered::unordered_map<std::string,keywords_rule>::iterator map_it;
  for (map_it = rule_map->begin(); map_it != rule_map->end(); ++map_it) {
    result.push_back(map_it->second);
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

