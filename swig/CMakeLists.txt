FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})

INCLUDE_DIRECTORIES(
    ${LEMON_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
)

SET_SOURCE_FILES_PROPERTIES(openql.i PROPERTIES CPLUSPLUS ON)

SET_PROPERTY(SOURCE openql.i PROPERTY SWIG_FLAGS -castmode -modern -keyword -w511)
#WJV
# -modern is deprecated
# -keyword gives lots of warnings
#SET_PROPERTY(SOURCE openql.i PROPERTY SWIG_FLAGS -castmode)

if (${CMAKE_VERSION} VERSION_LESS "3.8.0")
	SWIG_ADD_MODULE(openql python openql.i)
else()
    SWIG_ADD_LIBRARY( openql
                      TYPE MODULE
                      LANGUAGE python
                      SOURCES openql.i
                    )
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    SWIG_LINK_LIBRARIES(openql ${LEMON_LIBRARIES} ql lexgramstatic)
    set_target_properties(_openql PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
else ()
    if (${CMAKE_VERSION} VERSION_LESS "3.12.0")
        SWIG_LINK_LIBRARIES(openql ${PYTHON_LIBRARIES} ${LEMON_LIBRARIES} ql lexgramstatic)
    else()
        SWIG_LINK_LIBRARIES(openql ${Python_LIBRARIES} ${LEMON_LIBRARIES} ql lexgramstatic)
    endif()
endif()
