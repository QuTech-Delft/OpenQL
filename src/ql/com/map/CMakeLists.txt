add_library(com_map
    "expression_mapper.cc"
    "qubit_mapping.cc"
)

target_include_directories(com_map PUBLIC include)

target_link_libraries(com_map PUBLIC utils)
target_link_libraries(com_map PUBLIC ir)