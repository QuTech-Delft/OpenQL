add_subdirectory(example)

target_sources(${PROJECT_NAME}_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/test_179.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_cc.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_cqasm_reader.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_mapper.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_multi_core_4_4.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_program.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_unitary.cc"
)
