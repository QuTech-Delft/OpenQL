add_pass_library(pass_sch_list_schedule
    "list_schedule.cc"
)

target_include_directories(pass_sch_list_schedule PUBLIC include)

target_link_libraries(pass_sch_list_schedule PUBLIC pmgr)
target_link_libraries(pass_sch_list_schedule PRIVATE ir)
target_link_libraries(pass_sch_list_schedule PRIVATE com_ddg)
target_link_libraries(pass_sch_list_schedule PRIVATE com_sch)