add_library(all_passes INTERFACE)

macro(add_pass_library name)
    add_library("${name}" OBJECT "${ARGN}")
    target_sources(all_passes PUBLIC $<TARGET_OBJECTS:${name}>)
endmacro()

add_subdirectory(ana)
add_subdirectory(dec)
add_subdirectory(io)
add_subdirectory(map/qubits)
add_subdirectory(opt)
add_subdirectory(sch)