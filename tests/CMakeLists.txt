INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/programs
  ${CMAKE_SOURCE_DIR}/tests
)


#ADD_EXECUTABLE(test_alap_rc_schedule test_alap_rc_schedule.cc )
#TARGET_LINK_LIBRARIES(test_alap_rc_schedule ${LEMON_LIBRARIES} )

# we keep an executable because it is easier to track the origin of exceptions using GDB:
ADD_EXECUTABLE(test_cc cc/test_cc.cc )
TARGET_LINK_LIBRARIES(test_cc ${LEMON_LIBRARIES} )
