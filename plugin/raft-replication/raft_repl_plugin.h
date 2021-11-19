//
// Created by fsindustry on 2021/8/20.
//

#ifndef MYSQL_RAFT_REPLICATION_PLUGIN_H
#define MYSQL_RAFT_REPLICATION_PLUGIN_H

// plugin.h defines the MYSQL_REPLICATION_PLUGIN server plugin type and the data structures needed to declare the plugin.
#include <mysql/plugin.h>

/**
 * define global vars reference to the status vars
 */
std::string raft_repl_role;
unsigned long raft_repl_term;
unsigned long raft_repl_last_log_index;
std::string raft_repl_status;

/**
 * define global vars reference to the system vars
 */
char raft_repl_enabled;
char *raft_repl_node_addr;
char *raft_repl_cluster_addrs;
int raft_repl_election_timeout;
int raft_repl_snapshot_interval;
char *raft_repl_data_dir;
#endif //MYSQL_RAFT_REPLICATION_PLUGIN_H
