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

// test qwg resource constraints mapping
void
test_qwg(std::string v, std::string scheduler)
{
    // create and set platform
    std::string prog_name = "test_" + v + "_scheduler=" + scheduler;
    std::string kernel_name = "kernel_" + v + "_scheduler=" + scheduler;

    ql::quantum_platform starmon("starmon","test_alap_rc_schedule.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, 7, 0);
    ql::quantum_kernel k(kernel_name, starmon, 7, 0);

    // no dependency, only a conflict in qwg resource
    k.gate("x", 0);
    k.gate("y", 1);

    prog.add(k);
    ql::options::set("scheduler", scheduler);
    prog.compile( );
}

// test qwg concurrency
void
test_qwg2(std::string v, std::string scheduler)
{
    // create and set platform
    std::string prog_name = "test_" + v + "_scheduler=" + scheduler;
    std::string kernel_name = "kernel_" + v + "_scheduler=" + scheduler;

    ql::quantum_platform starmon("starmon","test_alap_rc_schedule.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, 7, 0);
    ql::quantum_kernel k(kernel_name, starmon, 7, 0);

    for (int j=0; j<7; j++) { k.gate("x", j); }
    k.gate("x", 0);
    k.gate("y", 1);
    k.gate("y", 2);
    k.gate("x", 3);
    k.gate("y", 4);
    k.gate("x", 5);
    k.gate("y", 6);
    for (int j=0; j<7; j++) { k.gate("y", j); }

    prog.add(k);
    ql::options::set("scheduler", scheduler);
    prog.compile( );
}

// demo single dimension resource constraint representation simple
// this is a suboptimality that can be improved
void
test_singledim(std::string v, std::string scheduler)
{
    // create and set platform
    std::string prog_name = "test_" + v + "_scheduler=" + scheduler;
    std::string kernel_name = "kernel_" + v + "_scheduler=" + scheduler;

    ql::quantum_platform starmon("starmon","test_alap_rc_schedule.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, 7, 0);
    ql::quantum_kernel k(kernel_name, starmon, 7, 0);

    // independent gates but stacking qwg unit use
    // in s7, q2, q3 and q4 all use qwg1
    // the y q3 must be in an other cycle than the x's because x conflicts with y in qwg1
    // the x q2 and x q4 can be in parallel but the y q3 in between prohibits this
    // because the qwg1 resource in single dimensional:
    // after x q2 it is busy on x in cycle 0,
    // then it only looks at the y q3, which requires to go to cycle 1,
    // and then the x q4 only looks at the current cycle (cycle 1),
    // in which qwg1 is busy with the y, so for the x it is busy,
    // and the only option is to go for cycle 2
    k.gate("x", 2);
    k.gate("y", 3);
    k.gate("x", 4);

    prog.add(k);
    ql::options::set("scheduler", scheduler);
    prog.compile( );
}

// test edge resource constraints mapping
void
test_edge(std::string v, std::string scheduler)
{
    // create and set platform
    std::string prog_name = "test_" + v + "_scheduler=" + scheduler;
    std::string kernel_name = "kernel_" + v + "_scheduler=" + scheduler;

    ql::quantum_platform starmon("starmon","test_alap_rc_schedule.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, 7, 0);
    ql::quantum_kernel k(kernel_name, starmon, 7, 0);

    // no dependency, only a conflict in edge resource between the first two czs
    k.gate("cz", 1,4);
    k.gate("cz", 0,3);
    k.gate("cz", 2,5);

    prog.add(k);
    ql::options::set("scheduler", scheduler);
    prog.compile( );
}

// test detuned_qubits resource constraints mapping
void
test_detuned(std::string v, std::string scheduler)
{
    // create and set platform
    std::string prog_name = "test_" + v + "_scheduler=" + scheduler;
    std::string kernel_name = "kernel_" + v + "_scheduler=" + scheduler;

    ql::quantum_platform starmon("starmon","test_alap_rc_schedule.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, 7, 0);
    ql::quantum_kernel k(kernel_name, starmon, 7, 0);

    // preferably cz's parallel, but not with x 3
    k.gate("cz", 0,2);
    k.gate("cz", 1,4);
    k.gate("x", 3);

    // likewise, while y 3, no cz on 0,2 or 1,4
    k.gate("y", 3);
    k.gate("cz", 0,2);
    k.gate("cz", 1,4);

    prog.add(k);
    ql::options::set("scheduler", scheduler);
    prog.compile( );
}

// test detuned_qubits resource constraints mapping
void
test_detuned2(std::string v, std::string scheduler)
{
    // create and set platform
    std::string prog_name = "test_" + v + "_scheduler=" + scheduler;
    std::string kernel_name = "kernel_" + v + "_scheduler=" + scheduler;

    ql::quantum_platform starmon("starmon","test_alap_rc_schedule.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, 7, 0);
    ql::quantum_kernel k(kernel_name, starmon, 7, 0);

    // preferably cz's parallel, but not with x 3
    k.gate("cz", 0,2);
    k.gate("cz", 1,4);
    k.gate("x", 3);

    // likewise, while y 3, no cz on 0,2 or 1,4
    k.gate("y", 3);
    k.gate("cz", 2,5);
    k.gate("cz", 4,6);

    prog.add(k);
    ql::options::set("scheduler", scheduler);
    prog.compile( );
}

// one cnot with operands that are neighbors in s7
void
test_0(std::string v, std::string scheduler)
{
    // create and set platform
    std::string prog_name = "test_" + v + "_scheduler=" + scheduler;
    std::string kernel_name = "kernel_" + v + "_scheduler=" + scheduler;

    ql::quantum_platform starmon("starmon","test_alap_rc_schedule.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, 7, 0);
    ql::quantum_kernel k(kernel_name, starmon, 7, 0);

    k.gate("x", 0);
    k.gate("x", 2);

    // one cnot that is ok in trivial mapping
    k.gate("cnot", 0,2);

    k.gate("x", 0);
    k.gate("x", 2);

    prog.add(k);

    ql::options::set("scheduler", scheduler);
    prog.compile( );
}

// all cnots with operands that are neighbors in s7
void
test_1(std::string v, std::string scheduler)
{
    // create and set platform
    std::string prog_name = "test_" + v + "_scheduler=" + scheduler;
    std::string kernel_name = "kernel_" + v + "_scheduler=" + scheduler;

    ql::quantum_platform starmon("starmon","test_alap_rc_schedule.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, 7, 0);
    ql::quantum_kernel k(kernel_name, starmon, 7, 0);

    for (int j=0; j<7; j++) { k.gate("x", j); }

    // a list of all cnots that are ok in trivial mapping
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

    // for (int j=0; j<7; j++) { k.gate("x", j); }

    prog.add(k);

    ql::options::set("scheduler", scheduler);
    prog.compile( );
}

// code with a lot of preps at the start
// significant difference between ASAP and ALAP
void
test_7( std::string v, std::string scheduler)
{
    // create and set platform
    std::string prog_name = "test_" + v + "_scheduler=" + scheduler;
    std::string kernel_name = "kernel_" + v + "_scheduler=" + scheduler;

    ql::quantum_platform starmon("starmon","test_alap_rc_schedule.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, 7, 0);
    ql::quantum_kernel k(kernel_name, starmon, 7, 0);

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
    prog.compile( );
}

int main(int argc, char ** argv)
{
    // ql::utils::logger::set_log_level("LOG_INFO");
    ql::utils::logger::set_log_level("LOG_DEBUG");

    test_qwg("qwg", "ASAP");
    test_qwg("qwg", "ALAP");
    test_qwg2("qwg2", "ASAP");
    test_qwg2("qwg2", "ALAP");
    test_singledim("singledim", "ASAP");
    test_singledim("singledim", "ALAP");
    test_edge("edge", "ASAP");
    test_edge("edge", "ALAP");
    test_detuned("detuned", "ASAP");
    test_detuned("detuned", "ALAP");
    test_detuned2("detuned2", "ASAP");
    test_detuned2("detuned2", "ALAP");
    test_0("0", "ASAP");
    test_0("0", "ALAP");
    test_1("1", "ASAP");
    test_1("1", "ALAP");
    test_7("7", "ASAP");
    test_7("7", "ALAP");

    return 0;
}
