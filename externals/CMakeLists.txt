set(NNG_TESTS OFF CACHE BOOL "" FORCE)
set(NNG_TOOLS OFF CACHE BOOL "" FORCE)
set(NNG_ENABLE_NNGCAT OFF CACHE BOOL "" FORCE)
add_subdirectory(nng)

if (SIESTA_BUILD_TESTS)
    add_subdirectory(googletest)
endif()
