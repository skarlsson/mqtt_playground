add_executable(export-sample export-sample.cpp)

if (LINK_SHARED)
    target_link_libraries(export-sample ${MQTT_LIBS_SHARED})
else ()
    target_link_libraries(export-sample ${MQTT_LIBS_STATIC})
endif ()

INSTALL(TARGETS export-sample RUNTIME DESTINATION bin)



