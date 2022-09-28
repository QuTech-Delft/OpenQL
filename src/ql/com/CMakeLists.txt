add_subdirectory(dec)
add_subdirectory(ddg)
add_subdirectory(ana)
add_subdirectory(map)
add_subdirectory(cfg)
add_subdirectory(sch)

add_library(com
    "options.cc"
    "topology.cc"
)

target_include_directories(com PUBLIC include)

target_link_libraries(com PUBLIC utils)