cmake_minimum_required(VERSION 3.5)
project(kspp)

option(BUILD_PROGRAMS "build programs" ON)
option(BUILD_STATIC_LIBS "enable static linking support" ON)
option(BUILD_SHARED_LIBS "enable static linking support" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

#FEATURES
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wpedantic -Wreturn-type -g -O0")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wpedantic -Wreturn-type -g -O0")

#set(BOOST_LIBS boost_program_options boost_iostreams boost_filesystem boost_system boost_regex z bz2 pthread rt c)

SET(PROMETHEUS_CPP_LIBS prometheus-cpp-core)
SET(MQTT_LIBS paho-mqttpp3 paho-mqtt3cs)
SET(GOOGLE_IOT_DEPS ssl crypto jwt)

SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
add_definitions(-g -D_FILE_OFFSET_BITS=64 -D_REENTRANT -DEXTERNAL_LOCKS -DMULTITHREAD)
add_definitions(-fPIC)
add_definitions(-Wno-deprecated)

include_directories(${MQTT_INCLUDE_PATH} ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR})
link_directories(${MQTT_LIBRARY_PATH})

file(GLOB LIB_SRCS
      mqtt_utils/*.cpp
)

file(GLOB MQTT_INCLUDE_FILES
        include/mqtt_utils/*.h
)


SET(MQTT_LIBS_STATIC mqtt_utils_s ${PROMETHEUS_CPP_LIBS} ${BOOST_LIBS} ${MQTT_LIBS} ${GOOGLE_IOT_DEPS} glog gflags crypto ssl pthread stdc++fs)
SET(MQTT_LIBS_SHARED mqtt_utils ${PROMETHEUS_CPP_LIBS} ${BOOST_LIBS} ${MQTT_LIBS} ${GOOGLE_IOT_DEPS} glog gflags crypto ssl pthread stdc++fs)


if (BUILD_STATIC_LIBS)
    ADD_LIBRARY(mqtt_utils_s STATIC ${LIB_SRCS} ${MQTT_INCLUDE_FILES})
    INSTALL(TARGETS mqtt_utils_s
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
            )
endif ()

if (BUILD_SHARED_LIBS)
    ADD_LIBRARY(mqtt_utils SHARED ${LIB_SRCS} ${MQTT_INCLUDE_FILES})
    INSTALL(TARGETS mqtt_utils
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
            )
endif ()

INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include)

IF (BUILD_PROGRAMS)
    add_subdirectory(programs)
ENDIF ()




