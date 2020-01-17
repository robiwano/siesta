include(FetchContent)
function(fetch_repo NAME URL TAG)
    FetchContent_Declare(
        ${NAME}
        GIT_REPOSITORY ${URL}
        GIT_TAG        ${TAG}
    )
    FetchContent_GetProperties(${NAME})
    if(NOT ${NAME}_POPULATED)
        FetchContent_Populate(${NAME})
        add_subdirectory(${${NAME}_SOURCE_DIR} ${${NAME}_BINARY_DIR})
    endif()
endfunction()