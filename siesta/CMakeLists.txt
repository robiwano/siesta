set(SOURCES
    src/server.cpp
    src/client.cpp
)

set(HEADERS
    include/siesta/client.h
    include/siesta/common.h
    include/siesta/server.h
)

add_library(siesta STATIC 
    ${SOURCES}
    ${HEADERS}
)
target_link_libraries(siesta nng)
target_include_directories(siesta PUBLIC include)
if (MSVC)
    target_compile_definitions(siesta PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
if (SIESTA_ENABLE_TLS) 
    target_compile_definitions(siesta PUBLIC SIESTA_ENABLE_TLS=1)
endif()
set_target_properties(siesta
    PROPERTIES
    CXX_STANDARD 11
)
