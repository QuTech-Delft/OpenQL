find_package(GTest REQUIRED)

# Test executable
add_executable(${PROJECT_NAME}_test)

# Subdirectories
add_subdirectory(ql)
add_subdirectory(v1x)

# Test sources
target_sources(${PROJECT_NAME}_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

# Target options
target_link_libraries(${PROJECT_NAME}_test PRIVATE
    ql
    GTest::gmock
    GTest::gtest
)
if(CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(${PROJECT_NAME}_test PRIVATE
        -Wall -Wextra -Werror -Wfatal-errors
        -Wno-error=restrict
        -Wno-error=deprecated-declarations
    )
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    target_compile_options(${PROJECT_NAME}_test PRIVATE
        -Wall -Wextra -Werror -Wfatal-errors
        -Wno-error=unused-private-field
        -Wno-error=unused-but-set-variable
    )
elseif(MSVC)
    target_compile_options(${PROJECT_NAME}_test PRIVATE
        /MP /D_USE_MATH_DEFINES /EHsc /bigobj
    )
else()
    message(SEND_ERROR "Unknown compiler!")
endif()

gtest_discover_tests(${PROJECT_NAME}_test
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
