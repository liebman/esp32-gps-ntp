if(ESP_PLATFORM)

set(SOURCES MCP23017.cpp)
idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS .)

endif()
