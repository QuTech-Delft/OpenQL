INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/programs
  ${CMAKE_SOURCE_DIR}/tests
)

ADD_EXECUTABLE(simple2inputTest simple2inputTest.cc )
TARGET_LINK_LIBRARIES(simple2inputTest ${LEMON_LIBRARIES} )

ADD_EXECUTABLE(toffoliTest toffoliTest.cc )
TARGET_LINK_LIBRARIES(toffoliTest ${LEMON_LIBRARIES} )


ADD_EXECUTABLE(custom_instruction_test custom_instruction_test.cc )
TARGET_LINK_LIBRARIES(custom_instruction_test ${LEMON_LIBRARIES} )
