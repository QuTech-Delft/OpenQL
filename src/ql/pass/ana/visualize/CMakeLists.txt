add_pass_library(pass_ana_visualize
    "detail/types.cc"
    "detail/common.cc"
    "detail/image.cc"
    "detail/circuit.cc"
    "detail/interaction.cc"
    "detail/mapping.cc"
    "circuit.cc"
    "interaction.cc"
    "mapping.cc"
)

target_include_directories(pass_ana_visualize PUBLIC include)

target_link_libraries(pass_ana_visualize PUBLIC utils)
target_link_libraries(pass_ana_visualize PUBLIC pmgr)

# Only enable the visualizer if building on Windows or the X11 library is found when building on Linux or Mac.
if(WIN32)
    set(QL_VISUALIZER yes)
else()
    find_package(X11)
    if(X11_FOUND)
        set(QL_VISUALIZER yes)
        message("X11 libraries: ${X11_LIBRARIES}")
        target_link_libraries(pass_ana_visualize PUBLIC ${X11_LIBRARIES})
        message("X11 include path: ${X11_INCLUDE_DIR}")
        target_include_directories(pass_ana_visualize PRIVATE "${X11_INCLUDE_DIR}")
    else()
        set(QL_VISUALIZER no)
    endif()
endif()

if(QL_VISUALIZER)
    target_compile_definitions(pass_ana_visualize PRIVATE WITH_VISUALIZER)
    target_include_directories(pass_ana_visualize PRIVATE "${CMAKE_SOURCE_DIR}/deps/cimg/include/")
endif()
