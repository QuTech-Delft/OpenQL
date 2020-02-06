INCLUDE_DIRECTORIES(
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/examples
  ${PROJECT_SOURCE_DIR}/tests
  ${PROJECT_SOURCE_DIR}/swig
)

# here are the C++ tests which will be compiled from this directory
# ADD_EXECUTABLE(program_test program_test.cc )
# TARGET_LINK_LIBRARIES(program_test ${LEMON_LIBRARIES} )

# we keep an executable because it is easier to track the origin of exceptions using GDB.
# Must be built manually to limit default build time
ADD_EXECUTABLE(test_cc EXCLUDE_FROM_ALL cc/test_cc.cc )
TARGET_LINK_LIBRARIES(test_cc ql ${LEMON_LIBRARIES} )

# create output directory for test outputs
ADD_CUSTOM_TARGET(tests-output-directory ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory test_output)

# copy configuration files
CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/tests/test_cfg_cbox.json
	test_cfg_cbox.json COPYONLY)

CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/tests/hardware_config_cc_light.json
	hardware_config_cc_light.json COPYONLY)

CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/tests/hardware_config_qx.json
	hardware_config_qx.json COPYONLY)

CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/tests/test_mapper.json
	test_mapper.json COPYONLY)

CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/tests/test_mapper17.json
	test_mapper17.json COPYONLY)

CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/tests/test_rig.json
	test_rig.json COPYONLY)

CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/tests/cc/test_cfg_cc.json
	test_cfg_cc.json COPYONLY)

ADD_EXECUTABLE(test_mapper test_mapper.cc )
TARGET_LINK_LIBRARIES(test_mapper ql ${LEMON_LIBRARIES} )
