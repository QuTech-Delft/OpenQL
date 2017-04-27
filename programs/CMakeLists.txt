INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/programs
  ${CMAKE_SOURCE_DIR}/tests
)

ADD_EXECUTABLE(simple simple.cc )
TARGET_LINK_LIBRARIES(simple ${LEMON_LIBRARIES} )

ADD_EXECUTABLE(rb_single rb_single.cc )
TARGET_LINK_LIBRARIES(rb_single ${LEMON_LIBRARIES} )


ADD_EXECUTABLE(randomized_benchmarking randomized_benchmarking.cc )
TARGET_LINK_LIBRARIES(randomized_benchmarking ${LEMON_LIBRARIES} )
