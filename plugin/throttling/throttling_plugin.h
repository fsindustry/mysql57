//
// Created by fsindustry on 2023/10/16.
//

#ifndef MYSQL_THROTTLING_PLUGIN_H
#define MYSQL_THROTTLING_PLUGIN_H

#include <stdio.h>
#include <m_ctype.h>
#include <mysql/plugin.h>
#include <mysql/plugin_audit.h>
#include <my_sys.h>
#include <mysqld_error.h>
#include "my_compiler.h"

/**
 * description information for plugin
 */
typedef struct {
  const char *name;
  const char *author;
  const char *description;
  unsigned int version;
} plugin_info;


#endif //MYSQL_THROTTLING_PLUGIN_H
