
idf_component_register(SRCS minmea/minmea.c
                       INCLUDE_DIRS minmea)

# why does this not work?!@!?#$%
target_compile_definitions(${COMPONENT_TARGET} PRIVATE
                           timegm=mktime)
