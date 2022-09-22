add_library(rmgr_resource_types
    "base.cc"
)

target_include_directories(rmgr_resource_types PUBLIC include)
target_link_libraries(rmgr_resource_types PUBLIC utils)
target_link_libraries(rmgr_resource_types PUBLIC ir)
target_link_libraries(rmgr_resource_types PUBLIC ir_compat)