//
// Created by fsindustry on 2023/10/16.
//

#include "throttling_plugin.h"
#include "log.h"

static volatile int number_of_calls;

/*
  Plugin has been installed.
*/
static my_bool g_plugin_installed = FALSE;

/*
  Record buffer mutex.
*/
static mysql_mutex_t g_record_buffer_mutex;

/*
  Event recording buffer.
*/
static char *g_record_buffer;


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
char throttling_enabled;

static MYSQL_SYSVAR_BOOL(
    enabled,
    throttling_enabled,
    PLUGIN_VAR_OPCMDARG,
    "Enable throttling or not. default: false",
    NULL,
    NULL,
    false);

/**
 * Initialize the plugin at server start or plugin installation.
 * @param arg
 * @return 0 success
 *         1 failure (cannot happen)
 */
static int throttling_plugin_init(void *arg MY_ATTRIBUTE((unused))) {
  g_plugin_installed = TRUE;
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
  number_of_calls++;
  sql_print_information("plugin throttling recived a event: ");
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
            (unsigned long) MYSQL_AUDIT_QUERY_ALL         /* types we need to care */
        }
    };

static struct st_mysql_sys_var *throttling_sys_vars[] = {
    MYSQL_SYSVAR(enabled),
    NULL
};

/*
  Plugin library descriptor
*/
plugin_info throttler_info = {
    "simple_parser",
    "fsindustry",
    "a throttler based on keywords"
};

mysql_declare_plugin(audit_null)
        {
            MYSQL_AUDIT_PLUGIN, /* type */
            &throttling_descriptor, /* descriptor */
            throttler_info.name, /* name */
            throttler_info.author, /* author */
            throttler_info.description, /* description */
            PLUGIN_LICENSE_GPL, /* plugin license */
            throttling_plugin_init, /* init function (when loaded) */
            throttling_plugin_deinit, /* deinit function (when unloaded) */
            0x0100, /* version 1.0 */
            throttling_status, /* status variables */
            throttling_sys_vars, /* system variables */
            NULL,
            0
        }mysql_declare_plugin_end;