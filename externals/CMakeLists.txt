set(NNG_TESTS OFF CACHE BOOL "" FORCE)
set(NNG_TOOLS OFF CACHE BOOL "" FORCE)
set(NNG_ENABLE_NNGCAT OFF CACHE BOOL "" FORCE)
set(NNG_ENABLE_STATS OFF CACHE BOOL "" FORCE)
set(NNG_PROTO_BUS0 OFF CACHE BOOL "" FORCE)
set(NNG_PROTO_PAIR0 OFF CACHE BOOL "" FORCE)
set(NNG_PROTO_PAIR1 OFF CACHE BOOL "" FORCE)
set(NNG_PROTO_PUB0 OFF CACHE BOOL "" FORCE)
set(NNG_PROTO_PULL0 OFF CACHE BOOL "" FORCE)
set(NNG_PROTO_PUSH0 OFF CACHE BOOL "" FORCE)
set(NNG_PROTO_RESPONDENT0 OFF CACHE BOOL "" FORCE)
set(NNG_PROTO_SUB0 OFF CACHE BOOL "" FORCE)
set(NNG_PROTO_SURVEYOR0 OFF CACHE BOOL "" FORCE)
set(NNG_TRANSPORT_INPROC OFF CACHE BOOL "" FORCE)
set(NNG_TRANSPORT_IPC OFF CACHE BOOL "" FORCE)
add_subdirectory(nng)
set_target_properties(
    nng
    PROPERTIES
    FOLDER "Third-Party/nng"
)
if (SIESTA_NUM_TASKS)
    target_compile_definitions(nng PRIVATE NNG_NUM_TASKQ_THREADS=${SIESTA_NUM_TASKS})
endif()

if (SIESTA_BUILD_TESTS)
    include(fetcher.cmake)
    fetch_repo(
      googletest
      "https://github.com/google/googletest.git"
      release-1.11.0
    )
    set_target_properties(
        gmock gmock_main gtest gtest_main
        PROPERTIES 
        FOLDER "Tests/common"
    )

    fetch_repo(
      ghcfilesystem
      "https://github.com/gulrak/filesystem.git"
      v1.2.10
    )
endif()
