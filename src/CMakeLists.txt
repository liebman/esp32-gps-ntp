
file (GLOB SOURCES *.cpp)
idf_component_register(SRCS ${SOURCES}
    INCLUDE_DIRS .
    REQUIRES lvgl_esp32_drivers lvgl lvgl_tft lvgl_touch lvgl_cpp MCP23017 minmea)
target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_CONF_INCLUDE_SIMPLE=1)
component_compile_options(-std=c++17)
component_compile_definitions(minmea PRIVATE -Dtimegm=mktime)
