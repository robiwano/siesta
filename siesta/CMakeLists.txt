set(SOURCES
    src/server.cpp
    src/client.cpp
)

set(HEADERS
    include/siesta/client.h
    include/siesta/common.h
    include/siesta/server.h
)

add_library(siesta_static STATIC 
    ${SOURCES}
    ${HEADERS}
)
target_link_libraries(siesta_static nng)
target_include_directories(siesta_static PUBLIC include)
if (MSVC)
    target_compile_definitions(siesta_static PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
set_target_properties(siesta_static
    PROPERTIES
    CXX_STANDARD 11
)
