//
// Created by fsindustry on 2023/10/16.
//
#include <my_global.h>
#include <mysql_com.h>
#include <algorithm>
#include <memory>
#include "keywords_throttler.h"
#include "throttler_plugin.h"


C_MODE_START

my_bool add_keywords_throttler_rule_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
void add_keywords_throttler_rule_deinit(UDF_INIT *initid);
char *add_keywords_throttler_rule(UDF_INIT *initid, UDF_ARGS *args, char *result,
                                  unsigned long *length, char *is_null, char *error);

my_bool keywords_throttler_rules_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
void keywords_throttler_rules_deinit(UDF_INIT *initid);
char *keywords_throttler_rules(UDF_INIT *initid, UDF_ARGS *args, char *result,
                               unsigned long *length, char *is_null, char *error);

my_bool delete_keywords_throttler_rules_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
void delete_keywords_throttler_rules_deinit(UDF_INIT *initid);
char *delete_keywords_throttler_rules(UDF_INIT *initid, UDF_ARGS *args, char *result,
                                      unsigned long *length, char *is_null, char *error);

my_bool truncate_keywords_throttler_rules_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
void truncate_keywords_throttler_rules_deinit(UDF_INIT *initid);
char *truncate_keywords_throttler_rules(UDF_INIT *initid, UDF_ARGS *args, char *result,
                                        unsigned long *length, char *is_null, char *error);
C_MODE_END

my_bool add_keywords_throttler_rule_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {

  if (args->arg_count != 4) {
    strcpy(message,
           "Wrong arguments count for add_keywords_throttler_rule, need 4 args: id, rule_type, keywords, concurrency");
    return 1;
  }

  if (args->arg_type[0] != STRING_RESULT) {
    strcpy(message, "Wrong argument type for id(string)");
    return 1;
  }
  if (args->arg_type[1] != STRING_RESULT) {
    strcpy(message, "Wrong argument type for rule_type(string)");
    return 1;
  }

  keywords_throttler *throttle = (keywords_throttler *) current_throttler;
  keywords_rule_mamager *rule_manager = throttle->getMamager();
  std::string rule_type_name = std::string(args->args[1], args->lengths[1]);
  transform(rule_type_name.begin(), rule_type_name.end(), rule_type_name.begin(), ::tolower);
  keywords_rule_type rule_type = rule_manager->get_rule_type_by_name(rule_type_name);
  if (rule_type == RULETYPE_UNSUPPORT) {
    std::string err_msg = "Wrong argument type for rule_type(string), invalid rule_type: " + rule_type_name;
    strcpy(message, err_msg.c_str());
    return 1;
  }

  if (args->arg_type[2] != STRING_RESULT) {
    strcpy(message, "Wrong argument type for keywords(string)");
    return 1;
  }

  if (args->arg_type[3] != INT_RESULT) {
    strcpy(message, "Wrong argument type for concurrency(int)");
    return 1;
  }

  return 0;
}

void add_keywords_throttler_rule_deinit(UDF_INIT *initid) {
}

char *add_keywords_throttler_rule(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null,
                                  char *error) {
  keywords_throttler *throttle = (keywords_throttler *) current_throttler;
  keywords_rule_mamager *rule_manager = throttle->getMamager();

  // packet keywords rule according to input args.
  std::vector<std::shared_ptr<keywords_rule>> rules;
  std::shared_ptr<keywords_rule> rule = std::make_shared<keywords_rule>();
  rule->id = std::string(args->args[0], args->lengths[0]);
  std::string rule_type_name = std::string(args->args[1], args->lengths[1]);
  transform(rule_type_name.begin(), rule_type_name.end(), rule_type_name.begin(), ::tolower);
  rule->rule_type = rule_manager->get_rule_type_by_name(rule_type_name);
  rule->keywords = std::string(args->args[2], args->lengths[2]);

  // todo compile regex for current rule
  // 1.变为正则表达式
  // 2.编译正则表达式，产生一个正则对象；
  rule->regex = "";

  int32 max_concurrency = *((int32 *) args->args[3]);
  // if max concurrency is unlimited, adjust it to -1
  if (max_concurrency < 0) {
    max_concurrency = -1;
  }
  rule->max_concurrency = max_concurrency;
  rules.push_back(rule);

  rule_manager->add_rules(&rules);

  // set return value which will be sent to client.
  std::string res = "success";
  strcpy(result, res.c_str());
  *length = res.length();
  return result;
}

my_bool keywords_throttler_rules_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
  return 0;
}

void keywords_throttler_rules_deinit(UDF_INIT *initid) {
  if (initid->ptr) {
    delete[] initid->ptr;
  }
}

char *keywords_throttler_rules(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null,
                               char *error) {
  auto *throttle = (keywords_throttler *) current_throttler;
  keywords_rule_mamager *rule_manager = throttle->getMamager();

  std::vector<std::string> ids;
  uint32 i = 0;
  for (; i < args->arg_count; i++) {
    if (args->arg_type[i] == STRING_RESULT && args->args[i]) {
      ids.emplace_back(args->args[i], args->lengths[i]);
    }
  }

  std::vector<std::shared_ptr<keywords_rule>> rules;
  if (args->arg_count == 0) {
    rules = rule_manager->get_all_rules();
  } else {
    rules = rule_manager->get_rules(&ids);
  }

  // set return value which will be sent to client.
  std::string res;
  if (rules.empty()) {
    res.append("no keywords throttler rules.");
  } else {
    std::sort(rules.begin(), rules.end(), [](const std::shared_ptr<keywords_rule> &r1, const std::shared_ptr<keywords_rule> &r2) {
      return r1->id < r2->id;
    });

    for (const std::shared_ptr<keywords_rule> &rule: rules) {
      res.append(rule->id);
      res.append(",");
      res.append(rule_manager->get_name_by_rule_type(rule->rule_type));
      res.append(",");
      res.append(rule->keywords);
      res.append(",");
      res.append(std::to_string(rule->max_concurrency));
      res.append("\n");
    }
  }

  char *buf = new char[res.length()];
  initid->ptr = buf;
  strcpy(buf, res.c_str());
  *length = res.length();
  return buf;
}

my_bool delete_keywords_throttler_rules_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
  return 0;
}

void delete_keywords_throttler_rules_deinit(UDF_INIT *initid) {

}

char *
delete_keywords_throttler_rules(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null,
                                char *error) {
  keywords_throttler *throttle = (keywords_throttler *) current_throttler;
  keywords_rule_mamager *rule_manager = throttle->getMamager();

  std::vector<std::string> ids;
  uint32 i = 0;
  for (; i < args->arg_count; i++) {
    if (args->arg_type[i] == STRING_RESULT && args->args[i]) {
      ids.push_back(std::string(args->args[i], args->lengths[i]));
    }
  }
  rule_manager->delete_rules(&ids);

  // set return value which will be sent to client.
  std::string res = "success";
  strcpy(result, res.c_str());
  *length = res.length();
  return result;
}

my_bool truncate_keywords_throttler_rules_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
  return 0;
}

void truncate_keywords_throttler_rules_deinit(UDF_INIT *initid) {

}

char *
truncate_keywords_throttler_rules(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null,
                                  char *error) {
  keywords_throttler *throttle = (keywords_throttler *) current_throttler;
  keywords_rule_mamager *rule_manager = throttle->getMamager();
  rule_manager->truncate_rules();

  // set return value which will be sent to client.
  std::string res = "success";
  strcpy(result, res.c_str());
  *length = res.length();
  return result;
}

