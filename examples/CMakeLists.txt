include(FetchContent)

include(FetchContent)
FetchContent_Declare(
  ghcfilesystem
  GIT_REPOSITORY https://github.com/gulrak/filesystem.git
  GIT_TAG        v1.2.10
)

FetchContent_GetProperties(ghcfilesystem)
if(NOT ghcfilesystem_POPULATED)
  FetchContent_Populate(ghcfilesystem)
  add_subdirectory(${ghcfilesystem_SOURCE_DIR} ${ghcfilesystem_BINARY_DIR})
endif()


set(
    EXAMPLE_SRC
    rest_client
    rest_server
    static_file_server
    web_socket_client
    web_socket_server
)

add_library(ctrl_c_handler STATIC ctrl_c_handler.cpp)
target_include_directories(ctrl_c_handler PUBLIC .)
set_target_properties(ctrl_c_handler
    PROPERTIES
    FOLDER "Examples/common"
)

foreach(E ${EXAMPLE_SRC})
    set(EXAMPLE_NAME example_${E})
    add_executable(${EXAMPLE_NAME} ${E}.cpp)

    if (MSVC)
        target_compile_definitions(${EXAMPLE_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()

    target_include_directories(${EXAMPLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${EXAMPLE_NAME} siesta ghc_filesystem ctrl_c_handler)
    set_target_properties(${EXAMPLE_NAME}
        PROPERTIES
        FOLDER "Examples"
    )
endforeach()
