/*
    file:       test_cc.cc
    author:     Wouter Vlothuizen
    notes:      based on:
                - ../test_uniform.cc, modified and extended for the cc (Central Controller)
                - ../test_hybrid.py
*/

#include "openql.h"

#include <string>
#include <algorithm>

using namespace ql::utils;

#define CFG_FILE_JSON   "test_cfg_cc.json"

// based on tests/test_hybrid.py
void test_classical() {
    const int num_qubits = 17;
    const int num_cregs = 3;

    // create and set platform
    auto s17 = ql::Platform("s17", CFG_FILE_JSON);

    // create program
    auto prog = ql::Program("test_classical", s17, num_qubits, num_cregs);
    auto k = ql::Kernel("kernel7.0", s17, num_qubits, num_cregs);

    // quantum operations
    for (int j=6; j<17; j++) {
        k.gate("x", j);
    }
    k.barrier();      // help scheduler

    // 1/2/3 qubit flux
#if 0 // misaligns cz and park_cz (using old scheduler)
    k.gate("cnot", 6, 7);
    k.gate("park_cz", 11);  // NB: not necessarily correct qubit

    k.gate("cnot", 12, 13);
    k.gate("park_cz", 17);

    k.gate("cnot", 10, 15);
    k.gate("park_cz", 16);
#else
    k.gate("cz", 6, 7);
    k.gate("park_cz", 11);  // NB: not necessarily correct qubit

    k.gate("cz", 12, 13);
    k.gate("park_cz", 15);

    k.gate("cz", 10, 15);
    k.gate("park_cz", 16);
#endif
    k.barrier();      // help scheduler

    k.gate("cz_park", {6, 7, 11});
    k.gate("cz_park", {12, 13, 15});
    k.gate("cz_park1", {10, 15, 16});   // FIXME:
    k.barrier();      // help scheduler

    // gate with angle parameter
    double angle = 1.23456; // just some number
    k.gate("x", {6}, 0, angle);
#if 0    // FIXME: drops angle
[OPENQL] /mnt/mac/GIT/OpenQL/ql/kernel.h:762 Adding gate : x with qubits [6]
[OPENQL] /mnt/mac/GIT/OpenQL/ql/kernel.h:765 trying to add specialized decomposed gate for: x
[OPENQL] /mnt/mac/GIT/OpenQL/ql/kernel.h:525 Checking if specialized decomposition is available for x
[OPENQL] /mnt/mac/GIT/OpenQL/ql/kernel.h:539 decomposed specialized instruction name: x q6
[OPENQL] /mnt/mac/GIT/OpenQL/ql/kernel.h:612 composite gate not found for x q6
[OPENQL] /mnt/mac/GIT/OpenQL/ql/kernel.h:774 trying to add parameterized decomposed gate for: x
[OPENQL] /mnt/mac/GIT/OpenQL/ql/kernel.h:623 Checking if parameterized decomposition is available for x
[OPENQL] /mnt/mac/GIT/OpenQL/ql/kernel.h:637 decomposed parameterized instruction name: x %0
[OPENQL] /mnt/mac/GIT/OpenQL/ql/kernel.h:643 parameterized composite gate found for x %0
[OPENQL] /mnt/mac/GIT/OpenQL/ql/kernel.h:647 composite gate type
[OPENQL] /mnt/mac/GIT/OpenQL/ql/kernel.h:504 composite ins: x %0
[OPENQL] /mnt/mac/GIT/OpenQL/ql/kernel.h:508   sub ins: rx180 %0
[OPENQL] /mnt/mac/GIT/OpenQL/ql/kernel.h:659 Adding sub ins: rx180 %0
[OPENQL] /mnt/mac/GIT/OpenQL/ql/kernel.h:661  after comma removal, sub ins: rx180 %0
[OPENQL] /mnt/mac/GIT/OpenQL/ql/kernel.h:675 actual qubits of this gate: [6]

Breakpoint 1, ql::quantum_kernel::add_custom_gate_if_available (this=this@entry=0x7fffffffe410, gname="rx180", qubits=std::vector of length 1, capacity 1 = {...}, cregs=std::vector of length 0, capacity 0, duration=duration@entry=0,
    angle=angle@entry=0) at /mnt/mac/GIT/OpenQL/ql/kernel.h:485
485                     c.push_back(g);
(gdb) print *g
$1 = {<ql::gate> = {_vptr.gate = 0x4f1eb0 <vtable for ql::custom_gate+16>, optimization_enabled = true, name = "rx180", operands = std::vector of length 1, capacity 1 = {6}, creg_operands = std::vector of length 0, capacity 0, duration = 20,
    angle = 3.7321244583827756e-317}, m = {m = {{_M_value = 0 + 1 * I}, {_M_value = 1 + 0 * I}, {_M_value = 1 + 0 * I}, {_M_value = 0 + 0 * I}}}, parameters = 0, operation_type = ql::flux_t, used_hardware = std::vector of length 0, capacity 0,
  arch_operation_name = ""}
(gdb) print angle
$2 = 0
(gdb) bt
#0  ql::quantum_kernel::add_custom_gate_if_available (this=this@entry=0x7fffffffe410, gname="rx180", qubits=std::vector of length 1, capacity 1 = {...}, cregs=std::vector of length 0, capacity 0, duration=duration@entry=0, angle=angle@entry=0)
    at /mnt/mac/GIT/OpenQL/ql/kernel.h:485
#1  0x000000000048798e in ql::quantum_kernel::add_param_decomposed_gate_if_available (this=this@entry=0x7fffffffe410, gate_name="x", all_qubits=std::vector of length 1, capacity 1 = {...}, cregs=std::vector of length 0, capacity 0)
    at /mnt/mac/GIT/OpenQL/ql/kernel.h:678
#2  0x0000000000489adc in ql::quantum_kernel::gate (this=this@entry=0x7fffffffe410, gname="x", qubits=std::vector of length 1, capacity 1 = {...}, cregs=std::vector of length 0, capacity 0, duration=duration@entry=0,
    angle=angle@entry=1.2345600000000001) at /mnt/mac/GIT/OpenQL/ql/kernel.h:775
#3  0x000000000043b81c in test_classical (scheduler="ALAP", scheduler_uniform="no") at /mnt/mac/GIT/OpenQL/tests/cc/test_cc.cc:63
#4  0x000000000042f330 in main (argc=<optimized out>, argv=<optimized out>) at /mnt/mac/GIT/OpenQL/tests/cc/test_cc.cc:598
(gdb)
#endif

    k.gate("rx180", {6}, 0, angle);     // NB: works

    k.gate("measure", 7, 0);
    k.gate("measure", 8, 1);

    prog.add_kernel(k);

#if 0   // FIXME
    ql::set_option("backend_cc_map_input_file", "test_output/test_classical_ALAP_uniform_no.map");
#endif
    prog.compile();
}


void test_qec_pipelined() {
    const int num_qubits = 17;
    const int num_cregs = 3;

   // create and set platform
    auto s17 = ql::Platform("s17", CFG_FILE_JSON);

    // create program
    auto prog = ql::Program("test_qec_pipelined", s17, num_qubits, num_cregs);
    auto k = ql::Kernel("kernel7.0", s17, num_qubits, num_cregs);

    // pipelined QEC: [
    // see: R. Versluis et al., Phys. Rev. A 8, 034021 (2017)
    // - nw, ne, sw, se] -> [n, e, w, s] because we rotate grid
    // - H -> rym90, ry90, see Fig 2 of reference
    //
    // class SurfaceCode, qubits, tiles, width, getNeighbourN, getNeighbourE, getNeighbourW, getNeighbourS, getX, getZ, getData

    const unsigned int x = 7;
    const unsigned int xN = x-5;
    const unsigned int xE = x+1;
    const unsigned int xS = x+5;
    const unsigned int xW = x-1;

    const unsigned int z = 11;
    const unsigned int zN = z-5;
    const unsigned int zE = z+1;
    const unsigned int zS = z+5;
    const unsigned int zW = z-1;

    // X stabilizers
    k.gate("rym90", x);
    k.gate("rym90", xN);
    k.gate("rym90", xE);
    k.gate("rym90", xW);
    k.gate("rym90", xS);
    k.barrier();      // help scheduler

    k.gate("cz", x, xE);
    k.gate("cz", x, xN);
    k.gate("cz", x, xS);
    k.gate("cz", x, xW);
    k.barrier();      // help scheduler

    k.gate("ry90", x);
    k.gate("ry90", xN);
    k.gate("ry90", xE);
    k.gate("ry90", xW);
    k.gate("ry90", xS);
    k.barrier();      // help scheduler

    // FIXME:
    // - qubits participating in CZ need phase correction, which may be part of gate, or separate
    // - similar for qubits not participating
    // - phase corrections performed using flux lines:
    //      + duration?
    //      + possible in parallel without doing 2 qubits gate?

    k.gate("measure", x, 0);
    k.barrier();      // help scheduler

    // Z stabilizers
    k.gate("rym90", z);

    k.gate("cz", z, zE);
    k.gate("cz", z, zS);
    k.gate("cz", z, zN);
    k.gate("cz", z, zW);

    k.gate("ry90", z);
    k.gate("measure", z, 1);

    prog.add_kernel(k);

    prog.compile();
}


void test_do_while_nested_for() {
   // create and set platform
    auto s17 = ql::Platform("s17", CFG_FILE_JSON);

    // create program
    const int num_qubits = 17;
    const int num_cregs = 3;
    auto prog = ql::Program("test_do_while_nested_for", s17, num_qubits, num_cregs);
//    ql::quantum_kernel k("kernel7.0", s17, num_qubits, num_cregs);

    auto sp1 = ql::Program(("sp1"), s17, num_qubits, num_cregs);
    auto sp2 = ql::Program(("sp2"), s17, num_qubits, num_cregs);
    auto k1 = ql::Kernel("aKernel1", s17, num_qubits, num_cregs);
    auto k2 = ql::Kernel("aKernel2", s17, num_qubits, num_cregs);

    // create classical registers
    ql::CReg rd(1);    // destination register
    ql::CReg rs1(2);
    ql::CReg rs2(3);

    // quantum operations
    k1.gate("x", 6);
    k2.gate("y", 6);

    // sp1.add_do_while(k1, ql.Operation(rs1, '>=', rs2))
    auto op1 = ql::Operation(rs1, std::string(">="), rs2);
    sp1.add_do_while(k1, op1);

    // sp2.add_for(sp1, 100)
    sp2.add_for(sp1, 100);

    // p.add_program(sp2);
    prog.add_program(sp2);
    // NB: will not run properly, because rs1 and rs2 are never changed

    prog.compile();
}


void test_rabi() {
    // create and set platform
    auto s17 = ql::Platform("s17", "test_cfg_cc_demo.json");

    const int num_qubits = 17;
    const int num_cregs = 3;
    auto prog = ql::Program("test_rabi", s17, num_qubits, num_cregs);
    auto sp1 = ql::Program("sp1", s17, num_qubits, num_cregs);
    auto k1 = ql::Kernel("aKernel1", s17, num_qubits, num_cregs);

    ql::CReg rs1(1);
    ql::CReg rs2(2);
    size_t qubit = 10;     // connects to uhfqa-0 and awg8-mw-0

    k1.gate("x", qubit);
    k1.gate("measure", qubit, 1);

    auto op1 = ql::Operation(rs1, std::string(">="), rs2); // FIXME: bogus condition, endless loop
    sp1.add_do_while(k1, op1);
    prog.add_program(sp1);

    prog.compile();
}


void test_wait() {
    // create and set platform
    auto s17 = ql::Platform("s17", CFG_FILE_JSON);

    const int num_qubits = 17;
    const int num_cregs = 3;
    auto prog = ql::Program("test_wait", s17, num_qubits, num_cregs);
    auto sp1 = ql::Program("sp1", s17, num_qubits, num_cregs);
    auto k = ql::Kernel("aKernel", s17, num_qubits, num_cregs);

    size_t qubit = 10;     // connects to uhfqa-0 and awg8-mw-0

    for(int delay=1; delay<=10; delay++) {
        k.gate("x", qubit);
        k.wait({(ql::utils::UInt)qubit}, delay*20);
        k.gate("y", qubit);
    }

    prog.add_kernel(k);

    prog.compile();
}

// FIXME: test to find quantum inspire problems 20200325
void test_qi_example() {
    // create and set platform
    auto s17 = ql::Platform("s17", "config_cc_s17_direct_iq_openql_0_10.json");

    const int num_qubits = 17;
    const int num_cregs = 17;
    auto prog = ql::Program("test_qi_example", s17, num_qubits, num_cregs);
    auto sp1 = ql::Program("sp1", s17, num_qubits, num_cregs);
    auto k = ql::Kernel("aKernel", s17, num_qubits, num_cregs);

    for(size_t i=0; i<5; i++) {
        k.gate("prepz", i);
    }
    k.barrier();      // help scheduler
    k.gate("ry180", {0, 2});     // FIXME: "y" does not work, but gate decomposition should handle?
    //k.gate("wait"); // ???
#if 0   // FIXME: requires decomposition pass
    k.gate("cz", {8, 10});   // FIXME: was: k.gate("cz", {0, 2});
#endif
    //k.gate("wait"); // ???
    k.gate("y90", 2);

    k.barrier();      // help scheduler
    for(size_t i=0; i<5; i++) {
        k.gate("measure", i);
    }
    k.barrier();      // help scheduler

    prog.add_kernel(k);

    ql::set_option("write_qasm_files", "yes");    // so we can see bundles
    prog.compile();
}


#if 0   // FIXME: if_1_break deprecated in CC backend
void test_break() {
    // create and set platform
    auto s5 = ql::Platform("s5", "cc_s5_direct_iq.json");
    ql::set_option("write_qasm_files", "yes");        // so we can see bundles

    const int num_qubits = 5;
    const int num_cregs = 5;
    const int num_bregs = 5;
    auto prog = ql::Program("test_break", s5, num_qubits, num_cregs, num_bregs);
    auto k = ql::Kernel("aKernel", s5, num_qubits, num_cregs, num_bregs);

    k.gate("prepz", 1);
    k.gate("measure_fb", 1);
    k.gate("if_1_break", 1);

    prog.add_for(k, 100);

    prog.compile();
}
#endif


void test_condex() {
    // create and set platform
    auto s5 = ql::Platform("s5", "cc_s5_direct_iq.json");
    ql::set_option("write_qasm_files", "yes");        // so we can see bundles

    const int num_qubits = 5;
    const int num_cregs = 5;
    const int num_bregs = 5;
    auto prog = ql::Program("test_condex", s5, num_qubits, num_cregs, num_bregs);
    auto k = ql::Kernel("aKernel", s5, num_qubits, num_cregs, num_bregs);

    k.gate("prepz", 1);    // FIXME: program makes no sense
    k.gate("measure_fb", 1);
    k.gate("measure_fb", 2);

    k.condgate("x", {0}, "COND_ALWAYS", {});
    k.barrier();      // help scheduler
    k.condgate("x", {0}, "COND_NEVER", {});
    k.barrier();

    k.condgate("x", {0}, "COND_UNARY", {1});
    k.barrier();
    k.condgate("x", {0}, "COND_NOT", {1});
    k.barrier();

    k.condgate("x", {0}, "COND_AND", {1,2});
    k.barrier();
    k.condgate("x", {0}, "COND_NAND", {1,2});
    k.barrier();
    k.condgate("x", {0}, "COND_OR", {1,2});
    k.barrier();
    k.condgate("x", {0}, "COND_NOR", {1,2});
    k.barrier();
    k.condgate("x", {0}, "COND_XOR", {1,2});
    k.barrier();
    k.condgate("x", {0}, "COND_NXOR", {1,2});
    k.barrier();

    prog.add_for(k, 100);

    prog.compile();
}

void test_cqasm_condex() {
    // create platform
    auto platform = ql::Platform("s5", "cc_s5_direct_iq.json");
    size_t num_qubits = platform.get_qubit_number();
    // create program
    auto program = ql::Program("qasm_qi_example", platform, num_qubits);
#if 0    // FIXME: fails to compile (tested on Macos): "error: invalid application of 'sizeof' to an incomplete type 'ql::cqasm::ReaderImpl'"
    ql::cqasm::Reader cqasm_rdr(platform, program);
    cqasm_rdr.string2circuit(R"(
    version 1.0
    qubits 5
    prep_z q[0,1,2,3,4]
    y q[0,2]
    cz q[0], q[2]
    y90 q[2]
    measure_all
    )");
#endif

    // compile the resulting program
    program.compile();
}

int main(int argc, char **argv) {
    ql::initialize();
    ql::utils::logger::set_log_level("LOG_INFO");      // LOG_DEBUG, LOG_INFO

#if 0    // FIXME
    test_classical();
    test_qec_pipelined();
    test_do_while_nested_for();
    test_rabi();
    test_wait();
#endif

    test_qi_example();
//    test_break();
//    test_condex();    // FIXME: fails on for loop
//    test_cqasm_condex();

    return 0;
}
