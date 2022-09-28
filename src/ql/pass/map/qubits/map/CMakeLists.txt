add_pass_library(pass_map_qubits_map
    "detail/options.cc"
    "detail/free_cycle.cc"
    "detail/past.cc"
    "detail/alter.cc"
    "detail/future.cc"
    "detail/mapper.cc"
    "map.cc"
)

target_include_directories(pass_map_qubits_map PUBLIC include)

target_link_libraries(pass_map_qubits_map PUBLIC pmgr)
target_link_libraries(pass_map_qubits_map PUBLIC rmgr)
target_link_libraries(pass_map_qubits_map PUBLIC utils)
target_link_libraries(pass_map_qubits_map PUBLIC com)
target_link_libraries(pass_map_qubits_map PUBLIC com_map)
target_link_libraries(pass_map_qubits_map PUBLIC pass_map_qubits)

# FIXME: this dependency is a bit weird. Also there is another Scheduler class in com/sch.
target_link_libraries(pass_map_qubits_map PUBLIC pass_sch_schedule)
