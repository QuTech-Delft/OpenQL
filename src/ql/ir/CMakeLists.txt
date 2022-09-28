add_subdirectory(compat)
add_subdirectory(cqasm)

if(OPENQL_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Build the IR tree using tree-gen.
generate_tree(
    "${CMAKE_CURRENT_SOURCE_DIR}/ir.tree"
    "${CMAKE_CURRENT_BINARY_DIR}/include/ql/ir/ir.gen.h"
    "${CMAKE_CURRENT_BINARY_DIR}/ir.gen.cc"
)

add_custom_target(
    generated_files
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/include/ql/ir/ir.gen.h" "${CMAKE_CURRENT_BINARY_DIR}/ir.gen.cc"
)

add_library(ir
    "prim.cc"
    "${CMAKE_CURRENT_BINARY_DIR}/include/ql/ir/ir.gen.h"
    "${CMAKE_CURRENT_BINARY_DIR}/ir.gen.cc"
    "ops.cc"
    "operator_info.cc"
    "describe.cc"
    "consistency.cc"
    "old_to_new.cc"
    "new_to_old.cc"
    "dump.cc"
    "annotations.cc"
)

add_dependencies(ir generated_files)

target_include_directories(ir PUBLIC include)
target_include_directories(ir PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include")

target_link_libraries(ir PUBLIC utils)
target_link_libraries(ir PUBLIC ir_compat)
target_link_libraries(ir PUBLIC ir_cqasm)
target_link_libraries(ir PUBLIC arch_diamond_annotations)
target_link_libraries(ir PUBLIC com)
target_link_libraries(ir PUBLIC com_ana)
target_link_libraries(ir PUBLIC rmgr)