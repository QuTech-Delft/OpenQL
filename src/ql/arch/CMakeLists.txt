add_library(all_architectures OBJECT)

macro(add_arch_library name)
    add_library("${name}" OBJECT "${ARGN}")
    target_sources(all_architectures PUBLIC $<TARGET_OBJECTS:${name}>)
endmacro()

add_subdirectory(cc)
add_subdirectory(cc_light)
add_subdirectory(diamond)
add_subdirectory(none)

add_library(arch
    "info_base.cc"
    "architecture.cc"
    "factory.cc"
)

target_include_directories(arch PUBLIC include)

target_link_libraries(arch PUBLIC utils)
target_link_libraries(arch PRIVATE ir_compat)
target_link_libraries(arch PRIVATE pmgr)