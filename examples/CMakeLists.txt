INCLUDE_DIRECTORIES(
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/examples
  ${PROJECT_SOURCE_DIR}/tests
)

# Uncomment the programs which you want to compile
# you can also add your own programs on the similar lines

# ADD_EXECUTABLE(simple simple.cc )
# TARGET_LINK_LIBRARIES(simple ${LEMON_LIBRARIES} )

# ADD_EXECUTABLE(rb_single rb_single.cc )
# TARGET_LINK_LIBRARIES(rb_single ${LEMON_LIBRARIES} )

# ADD_EXECUTABLE(randomized_benchmarking randomized_benchmarking.cc )
# TARGET_LINK_LIBRARIES(randomized_benchmarking ${LEMON_LIBRARIES} )

# ADD_EXECUTABLE(multi_qubits_randomized_benchmarking multi_qubits_randomized_benchmarking.cc )
# TARGET_LINK_LIBRARIES(multi_qubits_randomized_benchmarking ${LEMON_LIBRARIES} )

# create output directory for test outputs
ADD_CUSTOM_TARGET(examples-output-directory ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory test_output)

# copy configuration files
CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/tests/test_cfg_cbox.json
    test_cfg_cbox.json COPYONLY)

CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/tests/hardware_config_cc_light.json
    hardware_config_cc_light.json COPYONLY)

CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/tests/hardware_config_qx.json
    hardware_config_qx.json COPYONLY)
