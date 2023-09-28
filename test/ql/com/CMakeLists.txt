add_subdirectory(ddg)

target_sources(${PROJECT_NAME}_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/topology.cc")
