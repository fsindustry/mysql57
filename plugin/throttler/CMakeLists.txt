set(CMAKE_CXX_STANDARD 11)

# Set source files variable
SET(THROTTLER_SOURCES throttler_plugin.h throttler_plugin.cc throttler.h keywords_throttler.h keywords_throttler.cc keywords_throttler_udf.cc throttler_counter.h)

# Register plugin
MYSQL_ADD_PLUGIN(throttler ${THROTTLER_SOURCES}
        MODULE_ONLY MODULE_OUTPUT_NAME "throttler")