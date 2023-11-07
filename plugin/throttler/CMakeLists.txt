set(CMAKE_CXX_STANDARD 11)

# add boost source code into plugin
SET(Boost_USE_STATIC_LIBS OFF)
ADD_DEFINITIONS(-DBOOST_ALL_NO_LIB -DBOOST_NO_AUTO_PTR)
INCLUDE_DIRECTORIES(SYSTEM ${BOOST_INCLUDE_DIR})


# Set source files variable
SET(THROTTLER_SOURCES throttler_plugin.h throttler_plugin.cc throttler.h keywords_throttler.h keywords_throttler.cc keywords_throttler_udf.cc throttler_counter.h)


# Register plugin
MYSQL_ADD_PLUGIN(throttler ${THROTTLER_SOURCES} ${BOOST_LIB_SOURCES}
        MODULE_ONLY MODULE_OUTPUT_NAME "throttler")