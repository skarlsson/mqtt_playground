project(mqtt_ciotc)

SET(SRCS
mqtt_ciotc.c
)

add_executable(mqtt_ciotc ${SRCS})
target_link_libraries(mqtt_ciotc ${MQTT_LIBS_STATIC})
