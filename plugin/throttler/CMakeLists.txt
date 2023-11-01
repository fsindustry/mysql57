# set(CMAKE_CXX_STANDARD 11)

# Set source files variable
SET(THROTTLER_SOURCES throttler_plugin.h throttler_plugin.cc throttler.h keywords_throttler.h keywords_throttler.cc keywords_throttler_udf.cc)

# Note that no COMPONENTS option is to be used for header only components,
# it would try to find a library
SET(Boost_USE_STATIC_LIBS OFF)
ADD_DEFINITIONS(-DBOOST_ALL_NO_LIB -DBOOST_NO_AUTO_PTR)

MESSAGE(STATUS "Using Boost headers from ${BOOST_INCLUDE_DIR}")

INCLUDE_DIRECTORIES(SYSTEM
        ${BOOST_INCLUDE_DIR}
        )
# Register plugin
MYSQL_ADD_PLUGIN(throttler ${THROTTLER_SOURCES} ${BOOST_LIB_SOURCES}
        MODULE_ONLY MODULE_OUTPUT_NAME "throttler")