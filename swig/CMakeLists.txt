FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})


if(${CMAKE_VERSION} VERSION_LESS "3.12.0") 
	FIND_PACKAGE(PythonInterp)
	if (${PYTHONINTERP_FOUND})
	    FIND_PACKAGE(PythonLibs "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
	endif()
	INCLUDE_DIRECTORIES( ${PYTHON_INCLUDE_DIRS} )
else()
	find_package (Python COMPONENTS Interpreter Development)
	INCLUDE_DIRECTORIES( ${Python_INCLUDE_DIRS} )
endif()

INCLUDE_DIRECTORIES(
    ${LEMON_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
)

SET_SOURCE_FILES_PROPERTIES(openql.i PROPERTIES CPLUSPLUS ON)

SET_PROPERTY(SOURCE openql.i PROPERTY SWIG_FLAGS -castmode -modern -keyword)
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
    SWIG_LINK_LIBRARIES(openql ${LEMON_LIBRARIES} ql)
    set_target_properties(_openql PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
else ()
    SWIG_LINK_LIBRARIES(openql ${PYTHON_LIBRARIES} ${LEMON_LIBRARIES} ql)
endif()
