INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/programs
  ${CMAKE_SOURCE_DIR}/tests
)

ADD_EXECUTABLE(test_mapper test_mapper.cc )
ADD_EXECUTABLE(test_metrics test_metrics.cc )
TARGET_LINK_LIBRARIES(test_mapper ${LEMON_LIBRARIES} )
TARGET_LINK_LIBRARIES(test_metrics ${LEMON_LIBRARIES} )
