if(OPENQL_BUILD_TESTS)
    add_subdirectory(tests)
endif()

add_library(utils
    "num.cc"
    "str.cc"
    "rangemap.cc"
    "exception.cc"
    "logger.cc"
    "filesystem.cc"
    "json.cc"
    "tree.cc"
    "vcd.cc"
    "options.cc"
    "progress.cc"
)

target_include_directories(utils PUBLIC include)

target_link_libraries(utils PUBLIC config)
target_link_libraries(utils PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(utils PUBLIC cqasm)

# Stack trace helper library, nothing functional here.
add_backward(utils)

# add_backward doesn't set INTERFACE_LINK_LIBRARIES, only LINK_LIBRARIES. That
# goes wrong when we're compiling statically, because said libraries are shared
# and need to be included in the final link.
set_property(TARGET utils APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${BACKWARD_LIBRARIES})