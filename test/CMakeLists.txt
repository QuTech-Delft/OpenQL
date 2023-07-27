# Packages
include(FetchContent)

message(STATUS "Fetching googletest")
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG "f014396910d397b4f408c0c867aa8ca6a000ef7f"
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Test executable
add_executable(${PROJECT_NAME}_test)

# Subdirectories
add_subdirectory(ql)

# Test sources
target_sources(${PROJECT_NAME}_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

# Target options
target_link_libraries(${PROJECT_NAME}_test
    PRIVATE ql
    PUBLIC GTest::gmock
    PUBLIC GTest::gtest
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

include(GoogleTest)
gtest_add_tests(TARGET ${PROJECT_NAME}_test
    SOURCES ${test_sources}
)
