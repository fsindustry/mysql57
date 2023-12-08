//
// Created by fsindustry on 2023/10/23.
//

#include <utility>
#include <cstring>
#include <algorithm>
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

thread_local_key_t THR_THROTTLER_CONTEXT_MAP_KEY;

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

keywords_rule_database::keywords_rule_database() : access_all(true),
                                                   id_map(nullptr),
                                                   database(nullptr) {}

keywords_rule_database::~keywords_rule_database() {
  delete id_map;
  if (database) {
    hs_free_database(database);
  }
}

int keywords_rule_database::init(const std::shared_ptr<rule_map_t> &rule_map) {
  const uint64_t rule_count = rule_map->size();

  // if no rules for current database
  if (rule_count == 0) {
    access_all = true;
    id_map = nullptr;
    database = nullptr;
    return 0;
  }

  // collect resources for database compiling
  int i = 0;
  char **patterns = new char *[rule_count];
  auto *ids = new uint32_t[rule_count];
  auto *flags = new uint32_t[rule_count];
  auto *tmp_map = new std::unordered_map<uint32_t, std::string>(rule_count);
  for (auto &pair: *rule_map) {
    uint64_t length = pair.second->regex.size();
    patterns[i] = new char[length + 1];
    std::strcpy(patterns[i], pair.second->regex.c_str());
    ids[i] = i;
    flags[i] = HS_FLAG_DOTALL | HS_FLAG_SINGLEMATCH;
    tmp_map->emplace(i, pair.second->id);
  }

  // compile database
  hs_database_t *tmp_database;
  hs_compile_error_t *compile_err;
  if (hs_compile_multi(patterns, flags, ids, 4,
                       HS_MODE_BLOCK, nullptr, &tmp_database,
                       &compile_err) != HS_SUCCESS) {
    // todo print errors

    delete[] patterns;
    delete[] ids;
    delete[] flags;
    delete tmp_map;
    hs_free_compile_error(compile_err);
    return 1;
  }

  // set attributes
  access_all = false;
  id_map = tmp_map;
  database = tmp_database;

  // release resources
  delete[] patterns;
  delete[] ids;
  delete[] flags;
  delete tmp_map;
  return 0;
}

keywords_rule_shard::keywords_rule_shard() {
  sql_type = keywords_sql_type();
  rule_map = std::make_shared<rule_map_t>();
  rule_database = std::make_shared<keywords_rule_database>();
  current_version = 0;
  mysql_rwlock_init(key_keywords_throttler_lock_rules_, &shard_lock);
}

keywords_rule_shard::keywords_rule_shard(const keywords_rule_shard &other) {
  sql_type = other.sql_type;
  rule_map = other.rule_map;
  rule_database = other.rule_database;
  current_version.store(other.current_version);
  mysql_rwlock_init(key_keywords_throttler_lock_rules_, &shard_lock);
}

keywords_rule_shard::keywords_rule_shard(keywords_rule_shard &&other) noexcept {
  sql_type = other.sql_type;
  rule_map = std::move(other.rule_map);
  rule_database = std::move(other.rule_database);
  current_version.store(other.current_version);
  mysql_rwlock_init(key_keywords_throttler_lock_rules_, &shard_lock);
}

keywords_rule_shard &keywords_rule_shard::operator=(const keywords_rule_shard &other) {
  if (this != &other) {
    sql_type = other.sql_type;
    rule_map = other.rule_map;
    current_version.store(other.current_version);
    mysql_rwlock_init(key_keywords_throttler_lock_rules_, &shard_lock);
  }
  return *this;
}

keywords_rule_shard &keywords_rule_shard::operator=(keywords_rule_shard &&other) noexcept {
  if (this != &other) {
    sql_type = other.sql_type;
    rule_map = std::move(other.rule_map);
    rule_database = std::move(other.rule_database);
    current_version.store(other.current_version);
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

  return update_database();
}


int keywords_rule_shard::delete_rules(std::vector<std::string> *ids) {

  auto_rw_lock_write write_lock(&shard_lock);

  for (const std::string &id: *ids) {
    rule_map->erase(id);
  }

  return update_database();
}

int keywords_rule_shard::truncate_rules() {
  auto_rw_lock_write write_lock(&shard_lock);
  rule_map = std::make_shared<rule_map_t>();
  return update_database();
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


std::shared_ptr<keywords_rule> keywords_rule_shard::get_rule_by_id(const std::string &rule_id) {
  auto_rw_lock_read read_lock(&shard_lock);

  auto it = rule_map->find(rule_id);
  if (it == rule_map->end()) {
    return nullptr;
  }
  return it->second;
}

bool keywords_rule_shard::changed(uint32_t version) {
  return current_version != version;
}

int keywords_rule_shard::update_database() {
  std::shared_ptr<keywords_rule_database> new_database = std::make_shared<keywords_rule_database>();
  int error = new_database->init(rule_map);
  if (error) {
    // todo print error
    return error;
  }

  rule_database = new_database;
  update_version();
  return 0;
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
    int error = rule_shard->add_rules(&pair.second);
    if (error) {
      // todo print error msg
      return error;
    }
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
    int error = rule_shard->delete_rules(&pair.second);
    if (error) {
      // todo print error msg
      return error;
    }
  }

  return 0;
}

int keywords_rule_mamager::truncate_rules() {
  // truncate rules by shard
  for (auto &pair: *rule_shard_map) {
    std::shared_ptr<keywords_rule_shard> rule_shard = pair.second;
    int error = rule_shard->truncate_rules();
    if (error) {
      // todo print error msg
      return error;
    }
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

keywords_rule_context::keywords_rule_context() : current_version(0),
                                                 rule_database(std::make_shared<keywords_rule_database>()),
                                                 scratch(nullptr),
                                                 matched_rule_ids(std::vector<std::string>()),
                                                 last_matched_rule(nullptr) {
}

keywords_rule_context::~keywords_rule_context() {
  if (scratch) {
    hs_free_scratch(scratch);
  }
}

int keywords_rule_context::init(uint32_t version, const std::shared_ptr<keywords_rule_database> &database) {

  current_version = version;
  if (!database) {
    rule_database = std::make_shared<keywords_rule_database>();
    if (scratch) {
      hs_free_scratch(scratch);
      scratch = nullptr;
    }
    return 0;
  }

  rule_database = database;
  hs_scratch_t *tmp_scratch = nullptr;
  if (hs_alloc_scratch(rule_database->get_database(), &tmp_scratch) != HS_SUCCESS) {
    // todo print error log
    return -1;
  }
  scratch = tmp_scratch;
  matched_rule_ids.clear();
  last_matched_rule = nullptr;
  return 0;
}

static int matched_callback(unsigned int id, unsigned long long from,
                            unsigned long long to, unsigned int flags, void *ctx) {
  auto *context = (keywords_rule_context *) ctx;
  std::string rule_id = context->rule_database->get_rule_id(id);
  context->matched_rule_ids.push_back(rule_id);
  return 0;
}

int keywords_rule_context::match(const char *query, size_t length) {
  if (hs_scan(this->rule_database->get_database(), query, length, 0, scratch, matched_callback, this) !=
      HS_SUCCESS) {
    // todo print error log
    return -1;
  }

  return 0;
}


bool keywords_rule_context::has_matched_rules() {
  return !matched_rule_ids.empty();
}

void keywords_rule_context::clean_matched_rules() {
  matched_rule_ids.clear();
}

std::string keywords_rule_context::get_first_matched_rule() {
  if (has_matched_rules()) {
    return matched_rule_ids[0];
  }
  return "";
}

throttler_thd_context::throttler_thd_context(bool whitelist_user, std::string user_name)
    : whitelist_user(whitelist_user), user_name(user_name) {
  auto *tmp_map = new rule_context_map_t;
  tmp_map->emplace(RULETYPE_SELECT, std::make_shared<keywords_rule_context>());
  tmp_map->emplace(RULETYPE_INSERT, std::make_shared<keywords_rule_context>());
  tmp_map->emplace(RULETYPE_UPDATE, std::make_shared<keywords_rule_context>());
  tmp_map->emplace(RULETYPE_DELETE, std::make_shared<keywords_rule_context>());
  tmp_map->emplace(RULETYPE_REPLACE, std::make_shared<keywords_rule_context>());
  this->context_map = tmp_map;
}

throttler_thd_context::~throttler_thd_context() {
  delete context_map;
}

std::shared_ptr<keywords_rule_context> throttler_thd_context::get_context_by_sql_type(keywords_sql_type sql_type) {
  auto iter = context_map->find(sql_type);
  if (iter == context_map->end()) {
    return nullptr;
  }
  return iter->second;
}

keywords_throttler::keywords_throttler() {
#ifdef HAVE_PSI_INTERFACE
  init_keywords_throttle_psi_keys();
#endif
  my_create_thread_local_key(&THR_THROTTLER_CONTEXT_MAP_KEY, NULL);
  mamager = new keywords_rule_mamager;
  whitelist = new throttler_whitelist;
}

keywords_throttler::~keywords_throttler() {
  my_delete_thread_local_key(THR_THROTTLER_CONTEXT_MAP_KEY);
  delete mamager;
  delete whitelist;
}


int keywords_throttler::after_thd_initialled(THD *thd, const mysql_event_connection *event) {
  std::string curr_user(event->user.str, event->user.length);
  add_thd_context(thd, curr_user);
  return 0;
}

void keywords_throttler::add_thd_context(THD *thd, std::string &curr_user) {
  bool is_whitelist_user = false;
  if (is_super_user(thd) || whitelist->contains(curr_user)) {
    is_whitelist_user = true;
  }
  throttler_thd_context *thd_context = new throttler_thd_context(is_whitelist_user, curr_user);
  set_thd_context(thd_context);
}

int keywords_throttler::before_thd_destroyed(THD *thd, const mysql_event_connection *event) {
  auto *thd_context = get_thd_context();
  delete thd_context;
  set_thd_context(NULL);
  return 0;
}

bool get_property(MYSQL_SECURITY_CONTEXT m_sctx, const char *property, LEX_CSTRING *value) {
  value->length = 0;
  value->str = 0;
  return security_context_get_option(m_sctx, property, value);
}

std::string keywords_throttler::get_user(THD *thd) {
  MYSQL_SECURITY_CONTEXT m_sctx;
  if (thd_get_security_context(thd, &m_sctx)) {
    return "";
  }

  MYSQL_LEX_CSTRING user_str;
  if (get_property(m_sctx, "user", &user_str)) {
    return "";
  }

  return std::string(user_str.str, user_str.length);
}

bool keywords_throttler::is_super_user(THD *thd) {
  MYSQL_SECURITY_CONTEXT m_sctx;
  if (thd_get_security_context(thd, &m_sctx)) {
    return "";
  }

  bool has_super = false;
  if (security_context_get_option(m_sctx, "privilege_super", &has_super))
    return false;

  return has_super;
}

int keywords_throttler::check_before_execute(THD *thd, const mysql_event_query *event) {

  // if thd or event is null, just return
  if (!thd || !event) {
    return 0;
  }

  // get thd context
  auto *thd_context = get_thd_context();
  if (!thd_context) {
    bool is_whitelist_user = false;
    std::string curr_user = get_user(thd);
    add_thd_context(thd, curr_user);
    thd_context = get_thd_context();
  }

  // if current user is whitelist user, just return
  if (thd_context->whitelist_user) {
    return 0;
  }

  keywords_sql_type sql_type = mamager->get_sql_type_by_sql_cmd(event->sql_command_id);
  if (sql_type == RULETYPE_UNSUPPORT) { // unsupportted sql type, just pass through it.
    return 0;
  }

  std::shared_ptr<keywords_rule_shard> shard = mamager->get_shard_by_sql_type(sql_type);
  std::shared_ptr<keywords_rule_context> context = thd_context->get_context_by_sql_type(sql_type);

  // if rule updated, rebuild context
  if (shard->get_current_version() != context->current_version) {
    context = std::make_shared<keywords_rule_context>();
    context->init(shard->get_current_version(), shard->get_rule_database());
    (*thd_context->context_map)[sql_type] = context;
  }

  // no rules for current shard, just access all request
  if (context->rule_database->is_access_all()) {
    return 0;
  }

  if (!context->match(event->query.str, event->query.length)) {
    // todo print error log
    return -1;
  }

  // if no rules matched, just access request
  if (!context->has_matched_rules()) {
    return 0;
  }

  std::string rule_id = context->get_first_matched_rule();
  std::shared_ptr<keywords_rule> rule = shard->get_rule_by_id(rule_id);
  // if can't find rules, just access request
  if (!rule) {
    return 0;
  }

  // if rule has been throttled, reject the request, and increase reject counter
  if (rule->throttled) {
    // todo print error log
    rule->reject_counter->inc();
    return -1;
  }

  // if rule is not throttled, increase concurrency counter
  rule->concurrency_counter->inc();
  context->last_matched_rule = rule;

  return 0;
}

int keywords_throttler::adjust_after_execute(THD *thd, const mysql_event_query *event) {

  // if thd or event is null, just return
  if (!thd || !event) {
    return 0;
  }

  // get thd context
  auto *thd_context = get_thd_context();
  if (!thd_context) {
    return 0;
  }

  // if current user is whitelist user, just return
  if (thd_context->whitelist_user) {
    return 0;
  }

  keywords_sql_type sql_type = mamager->get_sql_type_by_sql_cmd(event->sql_command_id);
  if (sql_type == RULETYPE_UNSUPPORT) { // unsupportted sql type, just pass through it.
    return 0;
  }

  // if current SQL matched rule, we need to decrease the concurrency counter.
  std::shared_ptr<keywords_rule_context> context = thd_context->get_context_by_sql_type(sql_type);
  if (context->last_matched_rule) {
    context->last_matched_rule->concurrency_counter->dec();
  }
  return 0;
}

throttler_whitelist *keywords_throttler::get_whitelist() {
  return whitelist;
}

throttler_thd_context *keywords_throttler::get_thd_context() {
  return (throttler_thd_context *) my_get_thread_local(THR_THROTTLER_CONTEXT_MAP_KEY);
}

void keywords_throttler::set_thd_context(throttler_thd_context *ctx) {
  my_set_thread_local(THR_THROTTLER_CONTEXT_MAP_KEY, ctx);
}

throttler_whitelist::throttler_whitelist() : whitelist_users(std::make_shared<std::unordered_set<std::string>>()) {
  mysql_rwlock_init(key_keywords_throttler_lock_rules_, &whitelist_lock);
}

throttler_whitelist::~throttler_whitelist() {
  mysql_rwlock_destroy(&whitelist_lock);
}

std::string trim(const std::string &str) {
  auto start = std::find_if_not(str.begin(), str.end(), [](int ch) {
    return std::isspace(ch);
  });

  auto end = std::find_if_not(str.rbegin(), str.rend(), [](int ch) {
    return std::isspace(ch);
  }).base();

  return (start < end ? std::string(start, end) : std::string());
}

void throttler_whitelist::update(std::string &users) {

  auto *tmpset = new std::unordered_set<std::string>();
  std::string token;
  std::stringstream ss(users);
  while (std::getline(ss, token, ',')) {
    token = trim(token);
    if (token.empty()) {
      continue;
    }
    tmpset->insert(trim(token));
  }

  auto_rw_lock_write write_lock(&whitelist_lock);
  whitelist_users->swap(*tmpset);
}

bool throttler_whitelist::contains(std::string &user) {
  auto_rw_lock_read read_lock(&whitelist_lock);
  return whitelist_users->find(user) != whitelist_users->end();
}
