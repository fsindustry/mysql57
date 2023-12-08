//
// Created by fsindustry on 2023/10/16.
//

#include "throttler_plugin.h"
#include "log.h"
#include "throttler.h"
#include "keywords_throttler.h"


/** Event strings. */
LEX_CSTRING event_names[][6] = {
    /** MYSQL_AUDIT_GENERAL_CLASS */
    {{C_STRING_WITH_LEN("MYSQL_AUDIT_GENERAL_LOG")},
        {C_STRING_WITH_LEN("MYSQL_AUDIT_GENERAL_ERROR")},
        {C_STRING_WITH_LEN("MYSQL_AUDIT_GENERAL_RESULT")},
        {C_STRING_WITH_LEN("MYSQL_AUDIT_GENERAL_STATUS")},
    },
    /** MYSQL_AUDIT_CONNECTION_CLASS */
    {{C_STRING_WITH_LEN("MYSQL_AUDIT_CONNECTION_CONNECT")},
        {C_STRING_WITH_LEN("MYSQL_AUDIT_CONNECTION_DISCONNECT")},
        {C_STRING_WITH_LEN("MYSQL_AUDIT_CONNECTION_CHANGE_USER")},
        {C_STRING_WITH_LEN("MYSQL_AUDIT_CONNECTION_PRE_AUTHENTICATE")},
    },
    /** MYSQL_AUDIT_PARSE_CLASS */
    {{C_STRING_WITH_LEN("MYSQL_AUDIT_PARSE_PREPARSE")},
        {C_STRING_WITH_LEN("MYSQL_AUDIT_PARSE_POSTPARSE")},
    },
    /** MYSQL_AUDIT_AUTHORIZATION_CLASS */
    {{C_STRING_WITH_LEN("MYSQL_AUDIT_AUTHORIZATION_USER")},
        {C_STRING_WITH_LEN("MYSQL_AUDIT_AUTHORIZATION_DB")},
        {C_STRING_WITH_LEN("MYSQL_AUDIT_AUTHORIZATION_TABLE")},
        {C_STRING_WITH_LEN("MYSQL_AUDIT_AUTHORIZATION_COLUMN")},
        {C_STRING_WITH_LEN("MYSQL_AUDIT_AUTHORIZATION_PROCEDURE")},
        {C_STRING_WITH_LEN("MYSQL_AUDIT_AUTHORIZATION_PROXY")},
    },
    /** MYSQL_AUDIT_TABLE_ROW_ACCES_CLASS */
    {
     {C_STRING_WITH_LEN("MYSQL_AUDIT_TABLE_ACCESS_READ")},
        {C_STRING_WITH_LEN("MYSQL_AUDIT_TABLE_ACCESS_INSERT")},
        {C_STRING_WITH_LEN("MYSQL_AUDIT_TABLE_ACCESS_UPDATE")},
        {C_STRING_WITH_LEN("MYSQL_AUDIT_TABLE_ACCESS_DELETE")},
    },
    /** MYSQL_AUDIT_GLOBAL_VARIABLE_CLASS */
    {
     {C_STRING_WITH_LEN("MYSQL_AUDIT_GLOBAL_VARIABLE_GET")},
        {C_STRING_WITH_LEN("MYSQL_AUDIT_GLOBAL_VARIABLE_SET")},
    },
    /** MYSQL_AUDIT_SERVER_STARTUP_CLASS */
    {
     {C_STRING_WITH_LEN("MYSQL_AUDIT_SERVER_STARTUP_STARTUP")},
    },
    /** MYSQL_AUDIT_SERVER_SHUTDOWN_CLASS */
    {
     {C_STRING_WITH_LEN("MYSQL_AUDIT_SERVER_SHUTDOWN_SHUTDOWN")},
    },
    /** MYSQL_AUDIT_COMMAND_CLASS */
    {
     {C_STRING_WITH_LEN("MYSQL_AUDIT_COMMAND_START")},
        {C_STRING_WITH_LEN("MYSQL_AUDIT_COMMAND_END")},
    },
    /** MYSQL_AUDIT_QUERY_CLASS */
    {
     {C_STRING_WITH_LEN("MYSQL_AUDIT_QUERY_START")},
        {C_STRING_WITH_LEN("MYSQL_AUDIT_QUERY_NESTED_START")},
        {C_STRING_WITH_LEN("MYSQL_AUDIT_QUERY_STATUS_END")},
        {C_STRING_WITH_LEN("MYSQL_AUDIT_QUERY_NESTED_STATUS_END")},
    },
    /** MYSQL_AUDIT_STORED_PROGRAM_CLASS */
    {
     {C_STRING_WITH_LEN("MYSQL_AUDIT_STORED_PROGRAM_EXECUTE")},
    }
};

static LEX_CSTRING event_to_str(unsigned int event_class,
                                unsigned long event_subclass) {
  int count;
  for (count = 0; event_subclass; count++, event_subclass >>= 1);

  return event_names[event_class][count - 1];
}


static volatile int number_of_calls;

/*
  Plugin has been installed.
*/
static my_bool g_plugin_installed = FALSE;

/*
  Plugin status variables for SHOW STATUS
*/

static struct st_mysql_show_var throttler_status[] =
    {
        {"throttler_called", (char *) &number_of_calls, SHOW_INT,  SHOW_SCOPE_GLOBAL},
        {nullptr,            nullptr,                   SHOW_LONG, SHOW_SCOPE_GLOBAL},
    };

/**
 * Define plugin variables.
 * define global vars reference to the system vars
 */
my_bool throttler_enabled;
char * throttler_white_list;

static void update_throttler_enabled(MYSQL_THD, struct st_mysql_sys_var *, void *,
                                     const void *value) {
  throttler_enabled = *static_cast<const int *>(value);
}


static MYSQL_SYSVAR_BOOL(
    enabled,
    throttler_enabled,
    PLUGIN_VAR_OPCMDARG,
    "Enable throttler or not. default: false",
    nullptr,
    update_throttler_enabled,
    false);

static int update_throttler_white_list(MYSQL_THD thd, st_mysql_sys_var *var, void* save,
                            struct st_mysql_value *value){
  DBUG_ENTER("update_throttler_white_list");

  char buff[NAME_CHAR_LEN];
  const char *str;
  int length= sizeof(buff);
  std::string config_string;
  if (!(str= value->val_str(value, buff, &length))){
    DBUG_RETURN(1);
  }


  std::shared_ptr<std::unordered_set<std::string>> tmpset = std::make_shared<std::unordered_set<std::string>>();
  std::string token;
  std::stringstream ss(str);
  while (std::getline(ss, token, ',')) {
    // 将分解的子字符串插入到哈希集合中
    tmpset->insert(token);
  }

  white_list_users.swap(tmpset);

  DBUG_RETURN(0);
}

static MYSQL_SYSVAR_STR(
    white_list,
    throttler_white_list,
/* optional var | malloc string | no set default */
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_MEMALLOC | PLUGIN_VAR_NODEFAULT,
    "The white list of throttler.",
    update_throttler_white_list,
    NULL,
    NULL);

throttler *current_throttler;

/**
 * Initialize the plugin at server start or plugin installation.
 * @param arg
 * @return 0 success
 *         1 failure (cannot happen)
 */
static int throttler_plugin_init(void *arg MY_ATTRIBUTE((unused))) {
  g_plugin_installed = TRUE;
  current_throttler = new keywords_throttler;
  sql_print_information("plugin throttler initialized.");
  return (0);
}

/**
 * Terminate the plugin at server shutdown or plugin deinstallation.
 * @param arg
 * @return 0 success
 *         1 failure (cannot happen)
 */
static int throttler_plugin_deinit(void *arg MY_ATTRIBUTE((unused))) {
  if (g_plugin_installed == TRUE) {
    g_plugin_installed = FALSE;
  }
  delete current_throttler;
  sql_print_information("plugin throttler deinitialized.");
  return (0);
}

/**
 * @brief throttler plugin function handler.
 * @param thd[in]         Connection context.
 * @param event_class[in] Event class value.
 * @param event[in]       Event data.
 * @return Value indicating, whether the server should abort continuation of the current operation:
 *         if 0, continue
 *         Non-Zero, abort
 */
static int throttler_notify(MYSQL_THD thd,
                            mysql_event_class_t event_class,
                            const void *event) {

  if (!throttler_enabled) {
    return 0;
  }

  if (!current_throttler) {
    return 0;
  }

  // todo if connection user is inner superuser, just skip
  number_of_calls++;

  if (event_class == MYSQL_AUDIT_QUERY_CLASS) {
    // query event which contains SQL statements string.
    const auto *event_query =
        (const struct mysql_event_query *) event;

    // if sql_cmd_type is not support throttler, just return and continue run
    auto *throttle = (keywords_throttler *) current_throttler;
    keywords_rule_mamager *rule_manager = throttle->get_mamager();
    if (!rule_manager->valid_sql_cmd_type(event_query->sql_command_id)) {
      return 0;
    }

    auto event_subclass = (unsigned long) *(int *) event;
    LEX_CSTRING event_name = event_to_str(event_class, event_subclass);
    sql_print_information("plugin throttler recived a event: %s, sql: %s", event_name.str, event_query->query.str);

    switch (event_query->event_subclass) {
      case MYSQL_AUDIT_QUERY_START:
        return current_throttler->check_before_execute(thd, event_query);
      case MYSQL_AUDIT_QUERY_STATUS_END:
        return current_throttler->adjust_after_execute(thd, event_query);
      default:
        break;
    }
  } else if (event_class == MYSQL_AUDIT_CONNECTION_CLASS) {
    // connection event which contains connection information.
    const auto *event_connection =
        (const struct mysql_event_connection *) event;
    switch (event_connection->event_subclass) {
      case MYSQL_AUDIT_CONNECTION_CONNECT:
        return current_throttler->after_thd_initialled(thd, event_connection);
      case MYSQL_AUDIT_CONNECTION_DISCONNECT:
        return current_throttler->before_thd_destroyed(thd, event_connection);
      default:
        break;
    }
  }

  return 0;
}

/*
  Plugin type-specific descriptor
*/
static struct st_mysql_audit throttler_descriptor =
    {
        MYSQL_AUDIT_INTERFACE_VERSION,                    /* interface version    */
        nullptr,                                             /* release_thd function */
        throttler_notify,                                /* notify function */
        {
            0,
            (unsigned long) MYSQL_AUDIT_CONNECTION_ALL,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            (unsigned long) MYSQL_AUDIT_QUERY_ALL,
            0
        } /* types we need to care */

    };

static struct st_mysql_sys_var *throttler_sys_vars[] = {
    MYSQL_SYSVAR(enabled),
    MYSQL_SYSVAR(white_list),
    nullptr
};

/*
  Plugin library descriptor
*/
plugin_info throttler_info = {
    "throttler",
    "fsindustry",
    "a throttler based on keywords",
    0x0100
};

mysql_declare_plugin(throttler)
        {
            MYSQL_AUDIT_PLUGIN, /* type */
            &throttler_descriptor, /* descriptor */
            throttler_info.name, /* name */
            throttler_info.author, /* author */
            throttler_info.description, /* description */
            PLUGIN_LICENSE_GPL, /* plugin license */
            throttler_plugin_init, /* init function (when loaded) */
            throttler_plugin_deinit, /* deinit function (when unloaded) */
            throttler_info.version, /* version 1.0 */
            throttler_status, /* status variables */
            throttler_sys_vars, /* system variables */
            nullptr,
            0
        }mysql_declare_plugin_end;
