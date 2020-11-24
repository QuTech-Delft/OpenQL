/*
    file:       test_cc.cc
    author:     Wouter Vlothuizen
    notes:      based on:
                - ../test_uniform.cc, modified and extended for the cc (Central Controller)
                - ../test_hybrid.py
*/
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>

#include <time.h>

#include <openql.h>

#define CFG_FILE_JSON   "test_cfg_cc.json"


// based on tests/test_hybrid.py
void test_classical(std::string scheduler, std::string scheduler_uniform)
{
    const int num_qubits = 17;
    const int num_cregs = 3;

    // create and set platform
    ql::quantum_platform s17("s17", CFG_FILE_JSON);

    // create program
    ql::quantum_program prog(("test_classical_" + scheduler + "_uniform_" + scheduler_uniform), s17, num_qubits, num_cregs);
    ql::quantum_kernel k("kernel7.0", s17, num_qubits, num_cregs);

    // quantum operations
    for (int j=6; j<17; j++) {
        k.gate("x", j);
    }
    k.wait({6,7,8,9,10,11,12,13,14,15,16}, 0);      // help scheduler

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
    k.wait({6,7,8,9,10,11,12,13,14,15,16}, 0);      // help scheduler

    k.gate("cz_park", {6, 7, 11});
    k.gate("cz_park", {12, 13, 15});
    k.gate("cz_park1", {10, 15, 16});   // FIXME:
    k.wait({6,7,8,9,10,11,12,13,14,15,16}, 0);      // help scheduler

    // gate with angle parameter
    double angle = 1.23456; // just some number
    k.gate("x", {6}, {}, 0, angle);
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

    k.gate("rx180", {6}, {}, 0, angle);     // NB: works


    // create classical registers
    ql::creg rd(1);    // destination register
    ql::creg rs1(2);
    ql::creg rs2(3);

#if 0   // FIXME: not implemented in CC backend
    // add/sub/and/or/xor//    k.classical(rd, ql::operation(rs1, std::string("+"), rs2));
    ql::operation op = ql::operation(rs1, std::string("+"), rs2);
    k.classical(rd, op);
#endif

#if 0   // Python
    k.classical(rd, ql.Operation(rs1, "+", rs2))

    // not
    k.classical(rd, ql.Operation("~", rs2))

    // comparison
    k.classical(rd, ql.Operation(rs1, "==", rs2))

    // initialize (r1 = 2)
    k.classical(rs1, ql.Operation(2))

    // assign (r1 = r2)
    k.classical(rs1, ql.Operation(rs2))

    // measure
    k.gate("measure", [0], rs1)
#endif
    k.gate("measure", {7}, {0});
    k.gate("measure", {8}, {1});

    prog.add(k);

    ql::options::set("scheduler", scheduler);
    ql::options::set("scheduler_uniform", scheduler_uniform);
#if 0   // FIXME
    ql::options::set("backend_cc_map_input_file", "test_output/test_classical_ALAP_uniform_no.map");
#endif
    prog.compile();
}


void test_qec_pipelined(std::string scheduler, std::string scheduler_uniform)
{
    const int num_qubits = 17;
    const int num_cregs = 3;

   // create and set platform
    ql::quantum_platform s17("s17", CFG_FILE_JSON);

    // create program
    ql::quantum_program prog(("test_qec_pipelined_" + scheduler + "_uniform_" + scheduler_uniform), s17, num_qubits, num_cregs);
    ql::quantum_kernel k("kernel7.0", s17, num_qubits, num_cregs);

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
//    k.wait({x, xN, xE, xW, xS}, 0);
    // FIXME: above line does not work with new scheduler.h
    k.wait({0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}, 0);

    k.gate("cz", x, xE);
    k.gate("cz", x, xN);
    k.gate("cz", x, xS);
    k.gate("cz", x, xW);
//    k.wait({x, xN, xE, xW, xS}, 0);
    k.wait({0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}, 0);

    k.gate("ry90", x);
    k.gate("ry90", xN);
    k.gate("ry90", xE);
    k.gate("ry90", xW);
    k.gate("ry90", xS);
//    k.wait({x, xN, xE, xW, xS}, 0);
    k.wait({0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}, 0);

    // FIXME:
    // - qubits participating in CZ need phase correction, which may be part of gate, or separate
    // - similar for qubits not participating
    // - phase corrections performed using flux lines:
    //      + duration?
    //      + possible in parallel without doing 2 qubits gate?

    k.gate("measure", {x}, {0});
//    k.wait({x}, 0);
    k.wait({0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}, 0);

    // Z stabilizers
    k.gate("rym90", z);

    k.gate("cz", z, zE);
    k.gate("cz", z, zS);
    k.gate("cz", z, zN);
    k.gate("cz", z, zW);

    k.gate("ry90", z);
    k.gate("measure", {z}, {1});

    prog.add(k);

    ql::options::set("scheduler", scheduler);
    ql::options::set("scheduler_uniform", scheduler_uniform);
    prog.compile();
}


void test_do_while_nested_for(std::string scheduler, std::string scheduler_uniform)
{
   // create and set platform
    ql::quantum_platform s17("s17", CFG_FILE_JSON);

    // create program
    const int num_qubits = 17;
    const int num_cregs = 3;
    ql::quantum_program prog(("test_do_while_nested_for_" + scheduler + "_uniform_" + scheduler_uniform), s17, num_qubits, num_cregs);
//    ql::quantum_kernel k("kernel7.0", s17, num_qubits, num_cregs);

    ql::quantum_program sp1(("sp1"), s17, num_qubits, num_cregs);
    ql::quantum_program sp2(("sp2"), s17, num_qubits, num_cregs);
    ql::quantum_kernel k1("aKernel1", s17, num_qubits, num_cregs);
    ql::quantum_kernel k2("aKernel2", s17, num_qubits, num_cregs);

    // create classical registers
    ql::creg rd(1);    // destination register
    ql::creg rs1(2);
    ql::creg rs2(3);

    // quantum operations
    k1.gate("x", 6);
    k2.gate("y", 6);

    // sp1.add_do_while(k1, ql.Operation(rs1, '>=', rs2))
    ql::operation op1 = ql::operation(rs1, std::string(">="), rs2);
    sp1.add_do_while(k1, op1);

    // sp2.add_for(sp1, 100)
    sp2.add_for(sp1, 100);

    // p.add_program(sp2);
    prog.add_program(sp2);
    // NB: will not run properly, because rs1 and rs2 are never changed

    ql::options::set("scheduler", scheduler);
    ql::options::set("scheduler_uniform", scheduler_uniform);
    prog.compile();
}




void test_rabi( std::string scheduler, std::string scheduler_uniform)
{
    // create and set platform
    ql::quantum_platform s17("s17", "test_cfg_cc_demo.json");

    const int num_qubits = 17;
    const int num_cregs = 3;
    ql::quantum_program prog(("test_rabi_" + scheduler + "_uniform_" + scheduler_uniform), s17, num_qubits, num_cregs);
    ql::quantum_program sp1(("sp1"), s17, num_qubits, num_cregs);
    ql::quantum_kernel k1("aKernel1", s17, num_qubits, num_cregs);

    ql::creg rs1(1);
    ql::creg rs2(2);
    size_t qubit = 10;     // connects to uhfqa-0 and awg8-mw-0

    k1.gate("x", qubit);
    k1.gate("measure", {(ql::utils::UInt)qubit}, {1});

    ql::operation op1 = ql::operation(rs1, std::string(">="), rs2); // FIXME: bogus condition, endless loop
    sp1.add_do_while(k1, op1);
    prog.add_program(sp1);

    ql::options::set("scheduler", scheduler);
    ql::options::set("scheduler_uniform", scheduler_uniform);
    prog.compile();
}


void test_wait( std::string scheduler, std::string scheduler_uniform)
{
    // create and set platform
    ql::quantum_platform s17("s17", CFG_FILE_JSON);

    const int num_qubits = 17;
    const int num_cregs = 3;
    ql::quantum_program prog(("test_wait_" + scheduler + "_uniform_" + scheduler_uniform), s17, num_qubits, num_cregs);
    ql::quantum_program sp1(("sp1"), s17, num_qubits, num_cregs);
    ql::quantum_kernel k("aKernel", s17, num_qubits, num_cregs);

    size_t qubit = 10;     // connects to uhfqa-0 and awg8-mw-0

    for(int delay=1; delay<=10; delay++) {
        k.gate("x", qubit);
        k.wait({(ql::utils::UInt)qubit}, delay*20);
        k.gate("y", qubit);
    }

    prog.add(k);

    ql::options::set("scheduler", scheduler);
    ql::options::set("scheduler_uniform", scheduler_uniform);
    prog.compile();
}

// FIXME: test to find quantum inspire problems 20200325
void test_qi_example( std::string scheduler, std::string scheduler_uniform)
{
    // create and set platform
    ql::quantum_platform s5("s5", "cc_s5_direct_iq.json");

    const int num_qubits = 5;
    const int num_cregs = 5;
    ql::quantum_program prog(("test_qi_example_" + scheduler + "_uniform_" + scheduler_uniform), s5, num_qubits, num_cregs);
    ql::quantum_program sp1(("sp1"), s5, num_qubits, num_cregs);
    ql::quantum_kernel k("aKernel", s5, num_qubits, num_cregs);

    k.gate("prepz", {0, 1, 2, 3, 4});
    k.gate("ry180", {0, 2});     // FIXME: "y" does not work, but gate decomposition should handle?
    k.gate("wait");
    k.gate("cz", {0, 2});
    k.gate("wait");
    k.gate("y90", 2);
    k.gate("measure", {0, 1, 2, 3, 4});

    prog.add(k);

    ql::options::set("scheduler", scheduler);
    ql::options::set("scheduler_uniform", scheduler_uniform);
    ql::options::set("write_qasm_files", "yes");    // so we can see bundles
    prog.compile();
}


int main(int argc, char ** argv)
{
    ql::utils::logger::set_log_level("LOG_DEBUG");      // LOG_DEBUG, LOG_INFO

#if 0
    test_classical("ALAP", "no");
    test_qec_pipelined("ALAP", "no");
    test_do_while_nested_for("ALAP", "no");
    test_rabi("ALAP", "no");
    test_wait("ALAP", "no");
#endif

#if 1
    test_qi_example("ALAP", "no");
#endif

    return 0;
}
