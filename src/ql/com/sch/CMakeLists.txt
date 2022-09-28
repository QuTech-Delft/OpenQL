add_library(com_sch
    "heuristics.cc"
    "scheduler.cc"
)

target_include_directories(com_sch PUBLIC include)

target_link_libraries(com_sch PUBLIC utils)
target_link_libraries(com_sch PUBLIC com_ddg)
target_link_libraries(com_sch PUBLIC ir)
target_link_libraries(com_sch PUBLIC rmgr)