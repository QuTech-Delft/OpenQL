#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>

#include <time.h>

#include <ql/openql.h>

// test qwg resource constraints mapping
void
test_qwg(std::string v, std::string schedopt)
{
    int n = 2;
    std::string prog_name = "test_" + v + "_schedopt=" + schedopt;
    std::string kernel_name = "test_" + v + "_schedopt=" + schedopt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_179.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    // no dependency, only a conflict in qwg resource
    k.gate("x", 0);
    k.gate("y", 1);

    prog.add(k);

    ql::options::set("scheduler", schedopt);
    prog.compile( );
}

// demo single dimension resource constraint representation simple
void
test_singledim(std::string v, std::string schedopt)
{
    int n = 5;
    std::string prog_name = "test_" + v + "_schedopt=" + schedopt;
    std::string kernel_name = "test_" + v + "_schedopt=" + schedopt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_179.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

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

    ql::options::set("scheduler", schedopt);
    prog.compile( );
}

// test edge resource constraints mapping
void
test_edge(std::string v, std::string schedopt)
{
    int n = 5;
    std::string prog_name = "test_" + v + "_schedopt=" + schedopt;
    std::string kernel_name = "test_" + v + "_schedopt=" + schedopt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_179.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    // no dependency, only a conflict in edge resource
    k.gate("cz", 1,4);
    k.gate("cz", 0,3);

    prog.add(k);

    ql::options::set("scheduler", schedopt);
    prog.compile( );
}

// test detuned_qubits resource constraints mapping
// no swaps generated
void
test_detuned(std::string v, std::string schedopt)
{
    int n = 5;
    std::string prog_name = "test_" + v + "_schedopt=" + schedopt;
    std::string kernel_name = "test_" + v + "_schedopt=" + schedopt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_179.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    // preferably cz's parallel, but not with x 3
    k.gate("cz", 0,2);
    k.gate("cz", 1,4);
    k.gate("x", 3);

    // likewise, while y 3, no cz on 0,2 or 1,4
    k.gate("y", 3);
    k.gate("cz", 0,2);
    k.gate("cz", 1,4);

    prog.add(k);

    ql::options::set("scheduler", schedopt);
    prog.compile( );
}

// one cnot with operands that are neighbors in s7
void
test_oneNN(std::string v, std::string schedopt)
{
    int n = 3;
    std::string prog_name = "test_" + v + "_schedopt=" + schedopt;
    std::string kernel_name = "test_" + v + "_schedopt=" + schedopt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_179.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    k.gate("x", 0);
    k.gate("x", 2);

    // one cnot that is ok in trivial mapping
    k.gate("cnot", 0,2);

    k.gate("x", 0);
    k.gate("x", 2);

    prog.add(k);

    ql::options::set("scheduler", schedopt);
    prog.compile( );
}

// all cnots with operands that are neighbors in s7
void
test_manyNN(std::string v, std::string schedopt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_schedopt=" + schedopt;
    std::string kernel_name = "test_" + v + "_schedopt=" + schedopt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_179.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

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

    for (int j=0; j<7; j++) { k.gate("x", j); }

    prog.add(k);

    ql::options::set("scheduler", schedopt);
    prog.compile( );
}

int main(int argc, char ** argv)
{
    ql::utils::logger::set_log_level("LOG_DEBUG");

    test_singledim("singledim", "ALAP");
    test_qwg("qwg", "ALAP");
    test_edge("edge", "ALAP");
    test_detuned("detuned", "ALAP");
    test_oneNN("oneNN", "ALAP");
    test_manyNN("manyNN", "ALAP");

    return 0;
}
