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
if(MSVC)
    target_compile_options(${PROJECT_NAME}_test PRIVATE
        /W3 /WX /w34996
        /D_CONSOLE /DCONSOLE
        /D_UNICODE /DUNICODE
        /diagnostics:column /EHsc /FC /fp:precise /Gd /GS /MP /sdl /utf-8 /Zc:inline
    )
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(${PROJECT_NAME}_test PRIVATE
        #-pedantic-errors -Werror -Wall -Wextra
        -Wl,-z,defs
        -Wno-deprecated
        # -Wconversion -Wsign-conversion -Wno-sign-conversion  # do not sort flags; order matters
    )
endif()

include(GoogleTest)
gtest_add_tests(TARGET ${PROJECT_NAME}_test
    SOURCES ${test_sources}
)
