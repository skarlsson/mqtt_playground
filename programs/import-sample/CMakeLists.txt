add_executable(import-sample import-sample.cpp)

if (LINK_SHARED)
    target_link_libraries(import-sample ${MQTT_LIBS_SHARED})
else ()
    target_link_libraries(import-sample ${MQTT_LIBS_STATIC})
endif ()

INSTALL(TARGETS import-sample RUNTIME DESTINATION bin)



