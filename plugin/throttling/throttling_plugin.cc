//
// Created by fsindustry on 2023/10/16.
//

#include "throttling_plugin.h"
#include "log.h"
#include "throttle.h"
#include "keywords_throttle.h"


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

static struct st_mysql_show_var throttling_status[] =
    {
        {"throttling_called", (char *) &number_of_calls, SHOW_INT,  SHOW_SCOPE_GLOBAL},
        {NULL, NULL,                                     SHOW_LONG, SHOW_SCOPE_GLOBAL},
    };

/**
 * Define plugin variables.
 * define global vars reference to the system vars
 */
my_bool throttling_enabled;

static void update_throttling_enabled(MYSQL_THD, struct st_mysql_sys_var *, void *,
                                      const void *value) {
  throttling_enabled = *static_cast<const int *>(value);
}


static MYSQL_SYSVAR_BOOL(
    enabled,
    throttling_enabled,
    PLUGIN_VAR_OPCMDARG,
    "Enable throttling or not. default: false",
    NULL,
    update_throttling_enabled,
    false);

static throttle *current_throttle;

/**
 * Initialize the plugin at server start or plugin installation.
 * @param arg
 * @return 0 success
 *         1 failure (cannot happen)
 */
static int throttling_plugin_init(void *arg MY_ATTRIBUTE((unused))) {
  g_plugin_installed = TRUE;
  current_throttle = new keywords_throttle;
  sql_print_information("plugin throttling initialized.");
  return (0);
}

/**
 * Terminate the plugin at server shutdown or plugin deinstallation.
 * @param arg
 * @return 0 success
 *         1 failure (cannot happen)
 */
static int throttling_plugin_deinit(void *arg MY_ATTRIBUTE((unused))) {
  if (g_plugin_installed == TRUE) {
    g_plugin_installed = FALSE;
  }
  delete current_throttle;
  sql_print_information("plugin throttling deinitialized.");
  return (0);
}

/**
 * @brief Throttling plugin function handler.
 * @param thd[in]         Connection context.
 * @param event_class[in] Event class value.
 * @param event[in]       Event data.
 * @return Value indicating, whether the server should abort continuation of the current operation:
 *         if 0, continue
 *         Non-Zero, abort
 */
static int throttling_notify(MYSQL_THD thd,
                             mysql_event_class_t event_class,
                             const void *event) {

  if (!throttling_enabled) {
    return 0;
  }

  number_of_calls++;

  if (event_class == MYSQL_AUDIT_QUERY_CLASS) {
    // query event which contains SQL statements string.
    const struct mysql_event_query *event_query =
        (const struct mysql_event_query *) event;

    unsigned long event_subclass = (unsigned long) *(int *) event;
    LEX_CSTRING event_name = event_to_str(event_class, event_subclass);
    sql_print_information("plugin throttling recived a event: %s, sql: %s", event_name.str, event_query->query.str);

    switch (event_query->event_subclass) {
      case MYSQL_AUDIT_QUERY_START:
        return current_throttle->check_before_execute(thd, event_query);
      case MYSQL_AUDIT_QUERY_STATUS_END:
        return current_throttle->adjust_after_execute(thd, event_query);
      default:
        break;
    }
  }

  return 0;
}

/*
  Plugin type-specific descriptor
*/
static struct st_mysql_audit throttling_descriptor =
    {
        MYSQL_AUDIT_INTERFACE_VERSION,                    /* interface version    */
        NULL,                                             /* release_thd function */
        throttling_notify,                                /* notify function */
        {
            0,
            0,
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

static struct st_mysql_sys_var *throttling_sys_vars[] = {
    MYSQL_SYSVAR(enabled),
    NULL
};

/*
  Plugin library descriptor
*/
plugin_info throttling_info = {
    "throttling",
    "fsindustry",
    "a throttler based on keywords",
    0x0100
};

mysql_declare_plugin(throttling)
        {
            MYSQL_AUDIT_PLUGIN, /* type */
            &throttling_descriptor, /* descriptor */
            throttling_info.name, /* name */
            throttling_info.author, /* author */
            throttling_info.description, /* description */
            PLUGIN_LICENSE_GPL, /* plugin license */
            throttling_plugin_init, /* init function (when loaded) */
            throttling_plugin_deinit, /* deinit function (when unloaded) */
            throttling_info.version, /* version 1.0 */
            throttling_status, /* status variables */
            throttling_sys_vars, /* system variables */
            NULL,
            0
        }mysql_declare_plugin_end;
