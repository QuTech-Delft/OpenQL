add_library(pmgr
    "group.cc"
    "factory.cc"
    "manager.cc"
    "condition.cc"
    "pass_types/base.cc"
    "pass_types/specializations.cc"
)

target_include_directories(pmgr PUBLIC include)

target_link_libraries(pmgr PUBLIC utils)
target_link_libraries(pmgr PUBLIC ir)
target_link_libraries(pmgr PUBLIC ir_cqasm)
target_link_libraries(pmgr PUBLIC ir_compat)