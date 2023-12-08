SET(CMAKE_CXX_STANDARD 11)

FIND_LIBRARY(libhs hs ${HYPERSCAN_ROOT}/lib ${HYPERSCAN_ROOT}/lib64)
IF(NOT libhs)
    MESSAGE(FATAL_ERROR “libhs not found”)
ENDIF(NOT libhs)

include_directories(${HYPERSCAN_ROOT}/include)
link_directories(${HYPERSCAN_ROOT}/lib ${HYPERSCAN_ROOT}/lib64)

# Set source files variable
SET(THROTTLER_SOURCES throttler_plugin.h throttler_plugin.cc throttler.h keywords_throttler.h keywords_throttler.cc keywords_throttler_udf.cc throttler_counter.h throttler_white_list.h throttler_white_list.cc)

# Register plugin
MYSQL_ADD_PLUGIN(throttler ${THROTTLER_SOURCES}
        LINK_LIBRARIES hs
        MODULE_ONLY MODULE_OUTPUT_NAME "throttler")
