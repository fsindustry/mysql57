//
// Created by fsindustry on 2023/10/16.
//
#include <my_global.h>
#include <my_sys.h>
#include <mysql_com.h>
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

  if (args->arg_count != 3) {
    strcpy(message, "Wrong arguments count for add_keywords_throttler_rule, need 3 args: id, keywords, concurrency");
    return 1;
  }

  if (args->arg_type[0] != STRING_RESULT) {
    strcpy(message, "Wrong argument type for id(string)");
    return 1;
  }

  if (args->arg_type[1] != STRING_RESULT) {
    strcpy(message, "Wrong argument type for keywords(string)");
    return 1;
  }

  if (args->arg_type[2] != INT_RESULT) {
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
  std::vector<keywords_rule> rules;
  keywords_rule rule;
  rule.id = std::string(args->args[0], args->lengths[0]);
  rule.keywords = std::string(args->args[1], args->lengths[1]);
  int32 max_concurrency = *((int32 *) args->args[2]);
  // if max concurrency is unlimited, adjust it to -1
  if (max_concurrency < 0) {
    max_concurrency = -1;
  }
  rule.max_concurrency = max_concurrency;
  rules.push_back(rule);
  rule_manager->add_rules(&rules);

  // set return value which will be sent to client.
  strcpy(result, "success");
  *length = 7;
  return result;
}

my_bool keywords_throttler_rules_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
  return 0;
}

void keywords_throttler_rules_deinit(UDF_INIT *initid) {

}

char *keywords_throttler_rules(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null,
                               char *error) {
  keywords_throttler *throttle = (keywords_throttler *) current_throttler;
  keywords_rule_mamager *rule_manager = throttle->getMamager();

  std::vector<std::string> ids;
  uint32 i = 0;
  for(; i < args->arg_count; i++){
    if(args->arg_type[i] == STRING_RESULT && args->args[i]){
      ids.push_back(std::string(args->args[i], args->lengths[i]));
    }
  }

  std::vector<keywords_rule> rules;
  if(args->arg_count == 0){
    rules = rule_manager->get_all_rules();
  } else {
    rules = rule_manager->get_rules(&ids);
  }

  // set return value which will be sent to client.
  strcpy(result, "success");
  *length = 7;
  return result;
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
  for(; i < args->arg_count; i++){
    if(args->arg_type[i] == STRING_RESULT && args->args[i]){
      ids.push_back(std::string(args->args[i], args->lengths[i]));
    }
  }
  rule_manager->delete_rules(&ids);

  // set return value which will be sent to client.
  strcpy(result, "success");
  *length = 7;
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
  strcpy(result, "success");
  *length = 7;
  return result;
}

