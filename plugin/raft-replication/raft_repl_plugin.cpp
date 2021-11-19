//
// Created by fsindustry on 2021/8/20.
//

#include <log.h>
#include "raft_repl_plugin.h"

/**
 * define Replication plugin descriptor
 */
struct Mysql_replication raft_repl_plugin = {
        MYSQL_REPLICATION_INTERFACE_VERSION
};


int show_raft_repl_role(void *thd,
                        struct st_mysql_show_var *out,
                        char *buf) {
    out->type = SHOW_CHAR;
    my_stpncpy(buf, raft_repl_role.c_str(), raft_repl_role.size());
    out->value = buf; // it's of SHOW_VAR_FUNC_BUFF_SIZE bytes
    return 0;
}

int show_raft_repl_status(void *thd,
                          struct st_mysql_show_var *out,
                          char *buf) {
    out->type = SHOW_CHAR;
    my_stpncpy(buf, raft_repl_status.c_str(), raft_repl_status.size());
    out->value = buf; // it's of SHOW_VAR_FUNC_BUFF_SIZE bytes
    return 0;
}

/**
 * plugin status variables
 */
static st_mysql_show_var raft_repl_status_vars[] = {
        {"raft_repl_role",
                (char *) &show_raft_repl_role,
                SHOW_FUNC,
                SHOW_SCOPE_GLOBAL},
        {"raft_repl_term",
                (char *) &raft_repl_term,
                SHOW_LONG,
                SHOW_SCOPE_GLOBAL},
        {"raft_repl_last_log_index",
                (char *) &raft_repl_last_log_index,
                SHOW_LONG,
                SHOW_SCOPE_GLOBAL},
        {"raft_repl_status",
                (char *) &show_raft_repl_status,
                SHOW_FUNC,
                SHOW_SCOPE_GLOBAL},
        {NULL,
                NULL,
                SHOW_LONG,
                SHOW_SCOPE_GLOBAL},
};

static MYSQL_SYSVAR_BOOL(
        enabled,
        raft_repl_enabled,
        PLUGIN_VAR_OPCMDARG,
        "Enable raft replication or not. default: false",
        NULL,
        NULL,
        false);

static MYSQL_SYSVAR_STR(
        node_addr,
        raft_repl_node_addr,
        PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_MEMALLOC,
        "node address for raft replication service. format: ip:port",
        NULL,
        NULL,
        "127.0.0.1:8001");

static MYSQL_SYSVAR_STR(
        cluster_addrs,
        raft_repl_cluster_addrs,
        PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_MEMALLOC,
        "cluster addresses for raft service cluster. format: ip1:port1,ip2:port2,...",
        NULL,
        NULL,
        "");

static MYSQL_SYSVAR_INT(
        election_timeout,
        raft_repl_election_timeout,
        PLUGIN_VAR_OPCMDARG,
        "election timeout millis for raft service.",
        NULL,
        NULL,
        5000, 1000, 3600 * 1000, 1000);

static MYSQL_SYSVAR_INT(
        snapshot_interval,
        raft_repl_snapshot_interval,
        PLUGIN_VAR_OPCMDARG,
        "snapshot interval seconds for raft service.",
        NULL,
        NULL,
        3600, 60, 24 * 3600, 1);


static MYSQL_SYSVAR_STR(
        data_dir,
        raft_repl_data_dir,
        PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_MEMALLOC,
        "root dir for raft service where raft data to store.",
        NULL,
        NULL,
        "/var/raft-repl/data");


/**
 * plugin system variables
 */
static st_mysql_sys_var *raft_repl_system_vars[] = {
        MYSQL_SYSVAR(enabled),
        MYSQL_SYSVAR(node_addr),
        MYSQL_SYSVAR(cluster_addrs),
        MYSQL_SYSVAR(election_timeout),
        MYSQL_SYSVAR(snapshot_interval),
        MYSQL_SYSVAR(data_dir),
        NULL,
};

/**
 * declare install plugin function
 */
static int raft_repl_plugin_init(void *p);

/**
 * declare uninstall plugin function
 */
static int raft_repl_plugin_deinit(void *p);

/**
 * Plugin library descriptor
 */
mysql_declare_plugin(raft_repl_plugin)
                {
                        MYSQL_REPLICATION_PLUGIN, // plugin type
                        &raft_repl_plugin, // plugin descriptor
                        "raft_repl", // plugin name
                        "fsindustry", // author
                        "raft replication plugin", // plugin description
                        PLUGIN_LICENSE_GPL,
                        raft_repl_plugin_init, /* Plugin Init */
                        raft_repl_plugin_deinit, /* Plugin Deinit */
                        0x0100 /* 1.0 */,
                        raft_repl_status_vars, /* status variables */
                        raft_repl_system_vars, /* system variables */
                        NULL,                         /* config options */
                        0,                            /* flags */
                }mysql_declare_plugin_end;

/**
 * the function to invoke when plugin is loaded
 * <p>
 * The server executes this function when it loads the plugin,
 * which happens for INSTALL PLUGIN or,
 * for plugins listed in the mysql.plugin table, at server startup.
 *
 * @param p points to the internal structure used to identify the plugin
 * @return returns zero for success and nonzero for failure.
 */
static int raft_repl_plugin_init(void *p) {

    raft_repl_role = "leader";
    raft_repl_term = 1;
    raft_repl_last_log_index = 1;
    raft_repl_status = "normal";
    sql_print_information("plugin raft_repl initialized.");
    return 0;
}

/**
 * the function to invoke when plugin is unloaded
 * <p>
 * The server executes this function when it unloads the plugin,
 * which happens for UNINSTALL PLUGIN or,
 * for plugins listed in the mysql.plugin table, at server shutdown.
 *
 * @param p points to the internal structure used to identify the plugin
 * @return returns zero for success and nonzero for failure.
 */
static int raft_repl_plugin_deinit(void *p) {

    raft_repl_role = "";
    raft_repl_term = -1;
    raft_repl_last_log_index = -1;
    raft_repl_status = "";

    sql_print_information("plugin raft_repl deinitialized.");
    return 0;
}