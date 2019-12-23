
# Need 3.11.4 for FetchContent
cmake_minimum_required (VERSION 3.11.4 FATAL_ERROR)

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

option(SIESTA_ENABLE_TLS OFF "Set to ON to enable the secure Siesta server")
if (SIESTA_ENABLE_TLS)
    include(FetchContent)
    FetchContent_Declare(
        mbedtls
        GIT_REPOSITORY https://github.com/ARMmbed/mbedtls.git
        GIT_TAG        yotta-2.3.2
    )
    FetchContent_GetProperties(mbedtls)
    if(NOT mbedtls_POPULATED)
        FetchContent_Populate(mbedtls)
        set(ENABLE_PROGRAMS OFF)
        set(ENABLE_TESTING OFF)
        add_subdirectory(${mbedtls_SOURCE_DIR} ${mbedtls_BINARY_DIR})
        target_include_directories(mbedtls PUBLIC $<BUILD_INTERFACE:${mbedtls_SOURCE_DIR}/include>)
        install (TARGETS mbedtls mbedx509 mbedcrypto
            EXPORT nng-target
            FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Library
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Library
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Library
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Tools
        )
        set_target_properties(
            apidoc mbedtls mbedx509 mbedcrypto lib
            PROPERTIES
            FOLDER "Third-Party/mbedTLS"
        )
    endif()
    set(NNG_ENABLE_TLS ON CACHE BOOL "" FORCE)
else()
    set(NNG_ENABLE_TLS OFF CACHE BOOL "" FORCE)
endif()

option(SIESTA_EXCLUDE_EXTERNALS OFF "Set to ON if nng target is otherwise provided")
if (NOT SIESTA_EXCLUDE_EXTERNALS)
    add_subdirectory(externals EXCLUDE_FROM_ALL)
endif()

add_subdirectory(siesta)

if (SIESTA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

if (SIESTA_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if (SIESTA_BUILD_DOCS)
    add_subdirectory(docs)
endif()
