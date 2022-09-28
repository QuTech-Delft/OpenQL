add_pass_library(pass_sch_schedule
    "detail/scheduler.cc"
    "schedule.cc"
)

target_include_directories(pass_sch_schedule PUBLIC include)

target_link_libraries(pass_sch_schedule PUBLIC pmgr)
target_link_libraries(pass_sch_schedule PUBLIC rmgr)
target_link_libraries(pass_sch_schedule PUBLIC utils)
target_link_libraries(pass_sch_schedule PUBLIC com)

target_link_libraries(pass_sch_schedule PUBLIC lemon)