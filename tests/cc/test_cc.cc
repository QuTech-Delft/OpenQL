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
//    std::vector<size_t> qubits;
    for (int j=6; j<19; j++) {
        k.gate("x", j);
//        qubits.push_back(j);
    }
//    k.wait(qubits, 0);      // FIXME: try to help scheduler

    k.gate("cnot", 6, 7);
    k.gate("cnot", 12, 13);
    k.gate("cnot", 10, 15);

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
    std::vector<size_t> qops, cops;
    qops.push_back(7);
    cops.push_back(0);      // FIXME: use a ql::creg
    k.gate("measure", qops , cops);


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
