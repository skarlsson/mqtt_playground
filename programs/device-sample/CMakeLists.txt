add_executable(device-sample device-sample.cpp)

if (LINK_SHARED)
    target_link_libraries(device-sample ${MQTT_LIBS_SHARED})
else ()
    target_link_libraries(device-sample ${MQTT_LIBS_STATIC})
endif ()

INSTALL(TARGETS device-sample RUNTIME DESTINATION bin)



