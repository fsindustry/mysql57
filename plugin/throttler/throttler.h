//
// Created by fsindustry on 2023/10/23.
//

#ifndef MYSQL_THROTTLER_H
#define MYSQL_THROTTLER_H

#include <string>
#include <mysql/plugin.h>
#include <mysql/plugin_audit.h>

/**
 * this is a base class, define interface of throttling strategy
 */
class throttler {
public:

  /**
   * after thd initialled, call this method to initial throttler thread context.
   * @param thd session context
   * @param event connect event
   * @return zero, success;
   *         non-zero, triggered limiting rules or an error occur
   */
  virtual int after_thd_initialled(MYSQL_THD thd, const mysql_event_connection *event) = 0;

  /**
   * before thd destoryed, call this method to clean throttler thread context.
   * @param thd session context
   * @param event disconnect event
   * @return zero, success;
   *         non-zero, triggered limiting rules or an error occur
   */
  virtual int before_thd_destroyed(MYSQL_THD thd, const mysql_event_connection *event) = 0;

  /**
   * Check whether the throttling rules are triggered before query executing
   * @param thd session context
   * @param event query event
   * @return zero, success;
   *         non-zero, triggered limiting rules or an error occur
   */
  virtual int check_before_execute(MYSQL_THD thd, const mysql_event_query *event) = 0;

  /**
   * Adjust the current throttling status after query executed
   * @param thd session context
   * @param event query event
   * @return zero, success;
   *         non-zero, an error occur
   */
  virtual int adjust_after_execute(MYSQL_THD thd, const mysql_event_query *event) = 0;

  virtual ~throttler() = 0;
};


#endif //MYSQL_THROTTLER_H
