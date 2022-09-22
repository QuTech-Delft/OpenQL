add_openql_unit_test(ir_test ir_test.cc)

target_link_libraries(ir_test PRIVATE pmgr ir com_ddg arch_cc_light pass_io_cqasm rmgr utils com_sch)