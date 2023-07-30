target_sources(${PROJECT_NAME}_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/multi_qubits_randomized_benchmarking.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/randomized_benchmarking.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/rb_single.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/simple.cc"
)
