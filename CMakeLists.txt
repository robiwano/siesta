
cmake_minimum_required (VERSION 3.4)

set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_SOURCE_DIR}/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/cxx_flag_overrides.cmake)

project(siesta C CXX)

unset(SIESTA_STANDALONE)
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    option(SIESTA_BUILD_TESTS ON "Build tests for Siesta")
    option(SIESTA_BUILD_EXAMPLES ON "Build examples for Siesta")
    option(SIESTA_BUILD_DOCS ON "Build documentation for Siesta")
endif()

option(SIESTA_EXCLUDE_EXTERNALS OFF "Set to ON if nng and json targets are otherwise provided")
if (NOT SIESTA_EXCLUDE_EXTERNALS)
    add_subdirectory(externals EXCLUDE_FROM_ALL)
endif()
add_subdirectory(siesta)

if (SIESTA_BUILD_TESTS)
    add_subdirectory(test)
endif()

if (SIESTA_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if (SIESTA_BUILD_DOCS)
    add_subdirectory(docs)
endif()
