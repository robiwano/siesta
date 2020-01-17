set(
    TEST_SRC
    rest_server
    http_client
    serve_directory
)

if (SIESTA_ENABLE_TLS)
    set(
        TEST_SRC
        ${TEST_SRC}
        rest_secure_server
    )
endif()

foreach(T ${TEST_SRC})
    set(TEST_NAME test_${T})
    add_executable(${TEST_NAME} ${T}.cpp)

    if (MSVC)
        target_compile_definitions(${TEST_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()

    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${TEST_NAME} siesta gtest_main ghc_filesystem)
    set_target_properties(${TEST_NAME}
        PROPERTIES
        FOLDER "Tests"
    )

    add_test(NAME ${T} COMMAND ${TEST_NAME})
endforeach()
