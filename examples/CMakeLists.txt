set(
    EXAMPLE_SRC
    rest_client
    rest_server
)

foreach(E ${EXAMPLE_SRC})
    set(EXAMPLE_NAME example_${E})
    add_executable(${EXAMPLE_NAME} ${E}.cpp)

    if (MSVC)
        target_compile_definitions(${EXAMPLE_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()

    target_include_directories(${EXAMPLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${EXAMPLE_NAME} siesta)
    set_target_properties(${EXAMPLE_NAME}
        PROPERTIES
        FOLDER "Examples"
    )
endforeach()
