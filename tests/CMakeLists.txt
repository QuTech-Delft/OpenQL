INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/programs
  ${CMAKE_SOURCE_DIR}/tests
)

# here are the C++ tests which will be compiled from this directory
# ADD_EXECUTABLE(program_test program_test.cc )
# TARGET_LINK_LIBRARIES(program_test ${LEMON_LIBRARIES} )

# we keep an executable because it is easier to track the origin of exceptions using GDB:
ADD_EXECUTABLE(test_cc cc/test_cc.cc )
TARGET_LINK_LIBRARIES(test_cc ${LEMON_LIBRARIES} )


# create output directory for test outputs
ADD_CUSTOM_TARGET(tests-output-directory ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory test_output)

# copy configuration files
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/tests/test_cfg_cbox.json 
	test_cfg_cbox.json COPYONLY)

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/tests/hardware_config_cc_light.json 
	hardware_config_cc_light.json COPYONLY)

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/tests/hardware_config_qx.json 
	hardware_config_qx.json COPYONLY)