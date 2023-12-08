//
// Created by fsindustry on 2023/10/16.
//

#ifndef MYSQL_THROTTLER_PLUGIN_H
#define MYSQL_THROTTLER_PLUGIN_H

#include <cstdio>
#include <m_ctype.h>
#include <mysql/plugin.h>
#include <mysql/plugin_audit.h>
#include <my_sys.h>
#include "my_compiler.h"
#include "throttler.h"

/**
 * description information for plugin
 */
typedef struct {
  const char *name;
  const char *author;
  const char *description;
  unsigned int version;
} plugin_info;

extern my_bool throttler_enabled;
extern throttler *current_throttler;

#endif //MYSQL_THROTTLER_PLUGIN_H
