add_subdirectory(qubits)

target_sources(${PROJECT_NAME}_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/map.cc")
