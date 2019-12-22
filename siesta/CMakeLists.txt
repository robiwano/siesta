set(SOURCES
    src/server.cpp
)

set(HEADERS
    include/siesta/server.h
)

add_library(siesta_static STATIC 
    ${SOURCES}
    ${HEADERS}
)
target_link_libraries(siesta_static nng)
target_include_directories(siesta_static PUBLIC include)
set_target_properties(siesta_static
    PROPERTIES
    CXX_STANDARD 11
)
