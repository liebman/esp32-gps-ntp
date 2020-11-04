
file (GLOB SOURCES *.cpp)
idf_component_register(SRCS ${SOURCES}
    INCLUDE_DIRS .
    REQUIRES lvgl_esp32_drivers lvgl lvgl_tft lvgl_touch lvgl_cpp minmea)
target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_CONF_INCLUDE_SIMPLE=1)
component_compile_options(-std=c++17)
component_compile_definitions(minmea PRIVATE -Dtimegm=mktime)

#
# ULP support additions to component CMakeLists.txt.
#
# 1. The ULP app name must be unique (if multiple components use ULP).
set(ulp_app_name ulp_main)
#
# 2. Specify all assembly source files.
#    Files should be placed into a separate directory (in this case, ulp/),
#    which should not be added to COMPONENT_SRCS.
set(ulp_s_sources "../ulp/i2c-ds3231.S" "../ulp/i2c-util.S" "../ulp/i2c.S" "../ulp/main.S")
#
# 3. List all the component source files which include automatically
#    generated ULP export file, ${ulp_app_name}.h:
set(ulp_exp_dep_srcs "main.cpp PPS.cpp")
#
# 4. Call function to build ULP binary and embed in project using the argument
#    values above.
ulp_embed_binary(${ulp_app_name} "${ulp_s_sources}" "${ulp_exp_dep_srcs}")
