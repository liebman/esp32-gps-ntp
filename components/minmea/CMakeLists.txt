
idf_component_register(SRCS minmea/minmea.c
                       INCLUDE_DIRS minmea)

target_compile_definitions(${COMPONENT_TARGET} PRIVATE
                           timegm=mktime)
