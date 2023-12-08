//
// Created by fsindustry on 2023/12/5.
//

#ifndef MYSQL_THROTTLER_WHITE_LIST_H
#define MYSQL_THROTTLER_WHITE_LIST_H
#include "throttler.h"
#include <unordered_set>

class throttler_white_list {

private:
  mysql_rwlock_t white_list_lock; // rwlock to control rule changes in concurrent environment.
  std::shared_ptr<std::unordered_set<std::string>> white_list_users;

public:



  void update();

  bool contains(std::string& user);
};


#endif //MYSQL_THROTTLER_WHITE_LIST_H
