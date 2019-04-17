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

#include "ql/openql.h"
#include "ql/utils.h"


#define CFG_FILE_JSON   "test_cfg_cc.json"


// based on tests/test_hybrid.py
void test_classical(std::string scheduler, std::string scheduler_uniform)
{
    const int num_qubits = 25;
    const int num_cregs = 3;

   // create and set platform
    ql::quantum_platform s17("s17", CFG_FILE_JSON);
    ql::set_platform(s17);

    // create program
    ql::quantum_program prog(("test_classical_" + scheduler + "_uniform_" + scheduler_uniform), s17, num_qubits, num_cregs);
    ql::quantum_kernel k("kernel7.0", s17, num_qubits, num_cregs);

    // quantum operations
    for (int j=6; j<19; j++) {
        k.gate("x", j);
    }
    k.wait({6,7,8,9,10,11,12,13,14,15,16,17,18}, 0);      // help scheduler

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
    k.gate("park_cz", 17);

    k.gate("cz", 10, 15);
    k.gate("park_cz", 16);
#endif
    k.wait({6,7,11,12,13,17,10,15,16}, 0); // help scheduler

    k.gate("cz_park", std::vector<size_t> {6, 7, 11});
    k.gate("cz_park", std::vector<size_t> {12, 13, 17});
    k.gate("cz_park", std::vector<size_t> {10, 15, 16});
    k.wait({6,7,11,12,13,17,10,15,16}, 0); // help scheduler

    // gate with angle parameter
    double angle = 1.23456; // just some number
    k.gate("x", std::vector<size_t> {6}, std::vector<size_t> {}, 0, angle);
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

    k.gate("rx180", std::vector<size_t> {6}, std::vector<size_t> {}, 0, angle);     // NB: works


    // create classical registers
    ql::creg rd;    // destination register
    ql::creg rs1;
    ql::creg rs2;

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
    k.gate("measure", std::vector<size_t> {7}, std::vector<size_t> {0});
    k.gate("measure", std::vector<size_t> {8}, std::vector<size_t> {1});

    prog.add(k);

    ql::options::set("scheduler", scheduler);
    ql::options::set("scheduler_uniform", scheduler_uniform);
    prog.compile( );
}


void test_qec_pipelined(std::string scheduler, std::string scheduler_uniform)
{
    const int num_qubits = 25;
    const int num_cregs = 3;

   // create and set platform
    ql::quantum_platform s17("s17", CFG_FILE_JSON);
    ql::set_platform(s17);

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
    k.wait({x, xN, xE, xW, xS}, 0);

    k.gate("cz", x, xE);
    k.gate("cz", x, xN);
    k.gate("cz", x, xS);
    k.gate("cz", x, xW);
    k.wait({x, xN, xE, xW, xS}, 0);

    k.gate("ry90", x);
    k.gate("ry90", xN);
    k.gate("ry90", xE);
    k.gate("ry90", xW);
    k.gate("ry90", xS);
    k.wait({x, xN, xE, xW, xS}, 0);

    k.gate("measure", std::vector<size_t> {x}, std::vector<size_t> {0});
    k.wait({x}, 0);

    // Z stabilizers
    k.gate("rym90", z);

    k.gate("cz", z, zE);
    k.gate("cz", z, zS);
    k.gate("cz", z, zN);
    k.gate("cz", z, zW);

    k.gate("ry90", z);
    k.gate("measure", std::vector<size_t> {z}, std::vector<size_t> {1});

    prog.add(k);

    ql::options::set("scheduler", scheduler);
    ql::options::set("scheduler_uniform", scheduler_uniform);
    prog.compile( );
}


void test_do_while_nested_for(std::string scheduler, std::string scheduler_uniform)
{
   // create and set platform
    ql::quantum_platform s17("s17", CFG_FILE_JSON);
    ql::set_platform(s17);

    // create program
    const int num_qubits = 25;
    const int num_cregs = 3;
    ql::quantum_program prog(("test_do_while_nested_for_" + scheduler + "_uniform_" + scheduler_uniform), s17, num_qubits, num_cregs);
    ql::quantum_kernel k("kernel7.0", s17, num_qubits, num_cregs);

    // FIXME: sweep points

    ql::quantum_program sp1(("sp1"), s17, num_qubits, num_cregs);
    ql::quantum_program sp2(("sp2"), s17, num_qubits, num_cregs);
    ql::quantum_kernel k1("aKernel1", s17, num_qubits, num_cregs);
    ql::quantum_kernel k2("aKernel2", s17, num_qubits, num_cregs);

    // create classical registers
    ql::creg rd;    // destination register
    ql::creg rs1;
    ql::creg rs2;

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
    prog.compile( );
}

#if 0 // Python
    p = ql.Program('test_do_while_nested_for', platform, num_qubits, num_cregs)
    sweep_points = [1,2]
    p.set_sweep_points(sweep_points, len(sweep_points))

    sp1 = ql.Program('subprogram1', platform, num_qubits, num_cregs)
    sp2 = ql.Program('subprogram2', platform, num_qubits, num_cregs)

    k1 = ql.Kernel('aKernel1', platform, num_qubits, num_cregs)
    k2 = ql.Kernel('aKernel2', platform, num_qubits, num_cregs)

    # create classical registers
    rd = ql.CReg()
    rs1 = ql.CReg()
    rs2 = ql.CReg()

    # quanutm operations
    k1.gate('x', [0])
    k2.gate('y', [0])

    sp1.add_do_while(k1, ql.Operation(rs1, '>=', rs2))
    sp2.add_for(sp1, 100)
    p.add_program(sp2)

    p.compile()

    QISA_fn = os.path.join(output_dir, p.name_+'.qisa')
    assemble(QISA_fn)
#endif




// a simple first test
// the x gates serve to separate the cnot gates wrt dependences
// this creates big bundles with 7 x gates
// and small bundles with just a cnot
// after uniform scheduling, one or more x gates
// should have been moved next to the cnot
// those will move that do not have operands that overlap those of the cnot
void
test_0( std::string scheduler, std::string scheduler_uniform)
{
    // create and set platform
    ql::quantum_platform s17("s17", CFG_FILE_JSON);
    ql::set_platform(s17);

    // create program
    ql::quantum_program prog(("test_0_" + scheduler + "_uniform_" + scheduler_uniform), s17, 7, 0);
    ql::quantum_kernel k("kernel7.0", s17, 7, 0);

    for (int j=0; j<7; j++)
        k.gate("x", j);
    k.gate("cnot", 0,2);
    for (int j=0; j<7; j++)
        k.gate("x", j);
    k.gate("cnot", 6,3);
    for (int j=0; j<7; j++)
        k.gate("x", j);
    k.gate("cnot", 1,4);

    prog.add(k);

    ql::options::set("scheduler", scheduler);
    ql::options::set("scheduler_uniform", scheduler_uniform);
    prog.compile( );
}

// just as the previous one
// but then more of the same
void
test_1( std::string scheduler, std::string scheduler_uniform)
{
    // create and set platform
    ql::quantum_platform s17("s17", CFG_FILE_JSON);
    ql::set_platform(s17);

    // create program
    ql::quantum_program prog(("test_1_" + scheduler + "_uniform_" + scheduler_uniform), s17, 7, 0);
    ql::quantum_kernel k("kernel7.1", s17, 7, 0);

    for (int j=0; j<7; j++)
        k.gate("x", j);
    k.gate("cnot", 0,2);
    for (int j=0; j<7; j++)
        k.gate("x", j);
    k.gate("cnot", 6,3);
    for (int j=0; j<7; j++)
        k.gate("x", j);
    k.gate("cnot", 1,4);

    for (int j=0; j<7; j++)
        k.gate("x", j);
    k.gate("cnot", 2,5);
    for (int j=0; j<7; j++)
        k.gate("x", j);
    k.gate("cnot", 3,1);
    for (int j=0; j<7; j++)
        k.gate("x", j);
    k.gate("cnot", 4,6);
    for (int j=0; j<7; j++)
        k.gate("x", j);
    k.gate("cnot", 2,0);
    for (int j=0; j<7; j++)
        k.gate("x", j);
    k.gate("cnot", 3,6);
    for (int j=0; j<7; j++)
        k.gate("x", j);
    k.gate("cnot", 4,1);
    for (int j=0; j<7; j++)
        k.gate("x", j);
    k.gate("cnot", 5,2);
    for (int j=0; j<7; j++)
        k.gate("x", j);
    k.gate("cnot", 1,3);
    for (int j=0; j<7; j++)
        k.gate("x", j);
    k.gate("cnot", 6,4);
    for (int j=0; j<7; j++)
        k.gate("x", j);

    prog.add(k);

    ql::options::set("scheduler", scheduler);
    ql::options::set("scheduler_uniform", scheduler_uniform);
    prog.compile( );
}

// big bundles with x gates
// alternated with cnot bundles
// these cnots were chosen to be mutually independent
// so will be going all 3 in one bundle
// the single independent x will be moved with it
void
test_2( std::string scheduler, std::string scheduler_uniform)
{
    // create and set platform
    ql::quantum_platform s17("s17", CFG_FILE_JSON);
    ql::set_platform(s17);

    // create program
    ql::quantum_program prog(("test_2_" + scheduler + "_uniform_" + scheduler_uniform), s17, 7, 0);
    ql::quantum_kernel k("kernel7.2", s17, 7, 0);

    for (int j=0; j<7; j++)
        k.gate("x", j);
    k.gate("cnot", 0,2);
    k.gate("cnot", 6,3);
    k.gate("cnot", 1,4);
    for (int j=0; j<7; j++)
        k.gate("x", j);
    k.gate("cnot", 2,5);
    k.gate("cnot", 3,1);
    k.gate("cnot", 4,6);
    for (int j=0; j<7; j++)
        k.gate("x", j);
    k.gate("cnot", 2,0);
    k.gate("cnot", 3,6);
    k.gate("cnot", 4,1);
    for (int j=0; j<7; j++)
        k.gate("x", j);
    k.gate("cnot", 5,2);
    k.gate("cnot", 1,3);
    k.gate("cnot", 6,4);
    for (int j=0; j<7; j++)
        k.gate("x", j);

    prog.add(k);

    ql::options::set("scheduler", scheduler);
    ql::options::set("scheduler_uniform", scheduler_uniform);
    prog.compile( );
}

// again big bundles with x gates
// alternated with cnot bundles;
// these cnots were chosen to be largely dependent
// this already creates smaller bundles but more of them
void
test_3( std::string scheduler, std::string scheduler_uniform)
{
    // create and set platform
    ql::quantum_platform s17("s17", CFG_FILE_JSON);
    ql::set_platform(s17);

    // create program
    ql::quantum_program prog(("test_3_" + scheduler + "_uniform_" + scheduler_uniform), s17, 7, 0);
    ql::quantum_kernel k("kernel7.3", s17, 7, 0);

    for (int j=0; j<7; j++)
        k.gate("x", j);
    k.gate("cnot", 6,3);
    k.gate("cnot", 0,2);
    k.gate("cnot", 1,3);
    k.gate("cnot", 1,4);
    k.gate("cnot", 0,3);
    for (int j=0; j<7; j++)
        k.gate("x", j);
    k.gate("cnot", 2,5);
    k.gate("cnot", 3,1);
    k.gate("cnot", 2,0);
    k.gate("cnot", 3,6);
    for (int j=0; j<7; j++)
        k.gate("x", j);
    k.gate("cnot", 4,1);
    k.gate("cnot", 3,0);
    k.gate("cnot", 4,6);
    for (int j=0; j<7; j++)
        k.gate("x", j);
    k.gate("cnot", 3,5);
    k.gate("cnot", 5,2);
    k.gate("cnot", 6,4);
    k.gate("cnot", 5,3);
    for (int j=0; j<7; j++)
        k.gate("x", j);

    prog.add(k);

    ql::options::set("scheduler", scheduler);
    ql::options::set("scheduler_uniform", scheduler_uniform);
    prog.compile( );
}

// as with test 3 but now without the big x bundles
// just the cnots in lexicographic order
// the worst you can imagine,
// creating the smallest bundles
void
test_4( std::string scheduler, std::string scheduler_uniform)
{
    // create and set platform
    ql::quantum_platform s17("s17", CFG_FILE_JSON);
    ql::set_platform(s17);

    // create program
    ql::quantum_program prog(("test_4_" + scheduler + "_uniform_" + scheduler_uniform), s17, 7, 0);
    ql::quantum_kernel k("kernel7.4", s17, 7, 0);

    for (int j=0; j<7; j++)
        k.gate("x", j);

    k.gate("cnot", 0,2);
    k.gate("cnot", 0,3);
    k.gate("cnot", 1,3);
    k.gate("cnot", 1,4);
    k.gate("cnot", 2,0);
    k.gate("cnot", 2,5);
    k.gate("cnot", 3,0);
    k.gate("cnot", 3,1);
    k.gate("cnot", 3,5);
    k.gate("cnot", 3,6);
    k.gate("cnot", 4,1);
    k.gate("cnot", 4,6);
    k.gate("cnot", 5,2);
    k.gate("cnot", 5,3);
    k.gate("cnot", 6,3);
    k.gate("cnot", 6,4);

    prog.add(k);

    ql::options::set("scheduler", scheduler);
    ql::options::set("scheduler_uniform", scheduler_uniform);
    prog.compile( );
}

void
test_5( std::string scheduler, std::string scheduler_uniform)
{
    // create and set platform
    ql::quantum_platform s17("s17", CFG_FILE_JSON);
    ql::set_platform(s17);

    // create program
    ql::quantum_program prog(("test_5_" + scheduler + "_uniform_" + scheduler_uniform), s17, 7, 0);
    ql::quantum_kernel k("kernel7.5", s17, 7, 0);

    // empty kernel

    prog.add(k);

    ql::options::set("scheduler", scheduler);
    ql::options::set("scheduler_uniform", scheduler_uniform);
    prog.compile( );
}

// code with a lot of preps at the start, meas at the end and some work in the middle
// all is equally critical so gain here
void
test_6( std::string scheduler, std::string scheduler_uniform)
{
    // create and set platform
    ql::quantum_platform s17("s17", CFG_FILE_JSON);
    ql::set_platform(s17);

    // create program
    ql::quantum_program prog(("test_6_" + scheduler + "_uniform_" + scheduler_uniform), s17, 7, 0);
    ql::quantum_kernel k("kernel7.6", s17, 7, 0);

    k.gate("prepz", 0);
    k.gate("prepz", 1);
    k.gate("prepz", 2);
    k.gate("prepz", 3);
    k.gate("prepz", 4);
    k.gate("prepz", 5);
    k.gate("prepz", 6);

    k.gate("t", 0);
    k.gate("t", 1);
    k.gate("t", 2);
    k.gate("t", 3);
    k.gate("t", 4);
    k.gate("t", 5);
    k.gate("t", 6);

    k.gate("measz", 0);
    k.gate("measz", 1);
    k.gate("measz", 2);
    k.gate("measz", 3);
    k.gate("measz", 4);
    k.gate("measz", 5);
    k.gate("measz", 6);

    prog.add(k);

    ql::options::set("scheduler", scheduler);
    ql::options::set("scheduler_uniform", scheduler_uniform);
    prog.compile( );
}

// code with a lot of preps at the start
void
test_7( std::string scheduler, std::string scheduler_uniform)
{
    // create and set platform
    ql::quantum_platform s17("s17", CFG_FILE_JSON);
    ql::set_platform(s17);

    // create program
    ql::quantum_program prog(("test_7_" + scheduler + "_uniform_" + scheduler_uniform), s17, 7, 0);
    ql::quantum_kernel k("kernel7.7", s17, 7, 0);

    k.gate("prepz", 0);
    k.gate("prepz", 1);
    k.gate("prepz", 2);
    k.gate("prepz", 3);
    k.gate("prepz", 4);
    k.gate("prepz", 5);
    k.gate("prepz", 6);

    k.gate("h", 0);	// qubit 0 critical
    k.gate("t", 0);
    k.gate("h", 0);
    k.gate("t", 0);

    k.gate("h", 2);	// qubit 2 loaded
    k.gate("t", 2);

    k.gate("h", 4);	// qubit 4 medium loaded

    for (int j=0; j<7; j++)	// all qubits some load at the end
        k.gate("x", j);

    prog.add(k);

    ql::options::set("scheduler", scheduler);
    ql::options::set("scheduler_uniform", scheduler_uniform);
    prog.compile( );
}

int main(int argc, char ** argv)
{
    ql::utils::logger::set_log_level("LOG_DEBUG");      // LOG_DEBUG, LOG_INFO

    test_classical("ALAP", "no");
    test_qec_pipelined("ALAP", "no");
    test_do_while_nested_for("ALAP", "no");

#if 0
    test_0("ALAP", "no");
    test_0("ALAP", "yes");
    test_1("ALAP", "no");
    test_1("ALAP", "yes");
    test_2("ALAP", "no");
    test_2("ALAP", "yes");
    test_3("ALAP", "no");
    test_3("ALAP", "yes");
    test_4("ALAP", "no");
    test_4("ALAP", "yes");
    test_5("ALAP", "no");
    test_5("ALAP", "yes");
    test_6("ALAP", "no");
    test_6("ALAP", "yes");
    test_7("ALAP", "no");
    test_7("ALAP", "yes");
#endif
    return 0;
}
