
file (GLOB SOURCES *.cpp  highint5.S)
idf_component_register(SRCS ${SOURCES}
    INCLUDE_DIRS .
    REQUIRES lvgl_esp32_drivers lvgl lvgl_tft lvgl_touch lvgl_cpp minmea nvs_flash)
component_compile_options(-std=c++17)
target_link_libraries(${COMPONENT_TARGET} "-u ld_include_my_isr_file")
