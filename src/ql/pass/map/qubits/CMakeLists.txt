add_subdirectory(map)
add_subdirectory(place_mip)

add_pass_library(pass_map_qubits
    "initial_placement_algo.cc"
)

target_include_directories(pass_map_qubits PUBLIC include)

target_link_libraries(pass_map_qubits PUBLIC pmgr)
target_link_libraries(pass_map_qubits PUBLIC ir)
target_link_libraries(pass_map_qubits PUBLIC utils)
target_link_libraries(pass_map_qubits PUBLIC com)
