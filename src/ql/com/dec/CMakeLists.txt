add_library(com_dec
    "unitary.cc"
    "rules.cc"
    "structure.cc"
)

target_include_directories(com_dec PUBLIC include)

target_link_libraries(com_dec PUBLIC utils)
target_link_libraries(com_dec PUBLIC ir)
target_link_libraries(com_dec PUBLIC com_map)
target_link_libraries(com_dec PUBLIC eigen)