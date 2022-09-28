add_subdirectory(resource_types)

add_library(rmgr
    "types.cc"
    "factory.cc"
    "state.cc"
    "manager.cc"
)

target_include_directories(rmgr PUBLIC include)
target_link_libraries(rmgr PUBLIC utils)
target_link_libraries(rmgr PUBLIC ir_compat)
target_link_libraries(rmgr PUBLIC ir)
target_link_libraries(rmgr PUBLIC rmgr_resource_types)
target_link_libraries(rmgr PRIVATE resource_)