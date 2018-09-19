INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}
#  ${CMAKE_SOURCE_DIR}/programs
  ${CMAKE_SOURCE_DIR}/tests/cc
)


ADD_EXECUTABLE(test_cc_uniform test_cc_uniform.cc )
TARGET_LINK_LIBRARIES(test_cc_uniform ${LEMON_LIBRARIES} )

# configure_file(test_cfg_cbox.json test_cfg_cbox.json COPYONLY)
