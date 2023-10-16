# Set source files variable
SET(THROTTLING_SOURCES throttling_plugin.h throttling_plugin.cc)

# Register plugin
MYSQL_ADD_PLUGIN(throttling ${THROTTLING_SOURCES} MODULE_ONLY MODULE_OUTPUT_NAME "throttling")