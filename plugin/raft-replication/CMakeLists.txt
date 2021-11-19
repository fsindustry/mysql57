# Set source files variable
SET(RAFT_REPL_SOURCES raft_repl_plugin.h raft_repl_plugin.cpp)

# Register plugin
MYSQL_ADD_PLUGIN(raft_replication ${RAFT_REPL_SOURCES}
        MODULE_ONLY MODULE_OUTPUT_NAME "raft_replication")