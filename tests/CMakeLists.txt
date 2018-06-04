INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/programs
  ${CMAKE_SOURCE_DIR}/tests
)


ADD_EXECUTABLE(test_uniform test_uniform.cc )
TARGET_LINK_LIBRARIES(test_uniform ${LEMON_LIBRARIES} )

#ADD_EXECUTABLE(simple2input_test simple2input_test.cc )
#TARGET_LINK_LIBRARIES(simple2input_test ${LEMON_LIBRARIES} )

# ADD_EXECUTABLE(toffoli_test toffoli_test.cc )
# TARGET_LINK_LIBRARIES(toffoli_test ${LEMON_LIBRARIES} )

# ADD_EXECUTABLE(custom_instruction_test custom_instruction_test.cc )
# TARGET_LINK_LIBRARIES(custom_instruction_test ${LEMON_LIBRARIES} )

# ADD_EXECUTABLE(qubits_test qubits_test.cc )
# TARGET_LINK_LIBRARIES(qubits_test ${LEMON_LIBRARIES} )

# ADD_EXECUTABLE(quantum_platform_test quantum_platform_test.cc )
# TARGET_LINK_LIBRARIES(quantum_platform_test ${LEMON_LIBRARIES} )

# configure_file(test_cfg_cbox.json test_cfg_cbox.json COPYONLY)
