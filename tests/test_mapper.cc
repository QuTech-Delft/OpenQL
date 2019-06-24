#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>

#include <time.h>

#include <ql/openql.h>

// all cnots with operands that are neighbors in s7
void
test_manyNN(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_swapopt=" + param1 + "_clifford_premapper=" + param2 + "_schedulercommute=" + param3 + "_presched=" + param4;
    std::string kernel_name = "test_" + v + "_swapopt=" + param1 + "_clifford_premapper=" + param2 + "_schedulercommute=" + param3 + "_presched=" + param4;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper.json");
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

    ql::options::set("mapreverseswap", param1);
    ql::options::set("clifford_premapper", param2);
    ql::options::set("clifford_postmapper", param2);
    ql::options::set("scheduler_commute", param3);
    ql::options::set("prescheduler", param4);
    prog.compile( );
}

// one cnot with operands that are at distance 2 in s7
void
test_oneD2(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 4;
    std::string prog_name = "test_" + v + "_swapopt=" + param1 + "_clifford_premapper=" + param2 + "_schedulercommute=" + param3 + "_presched=" + param4;
    std::string kernel_name = "test_" + v + "_swapopt=" + param1 + "_clifford_premapper=" + param2 + "_schedulercommute=" + param3 + "_presched=" + param4;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    k.gate("x", 2);
    k.gate("x", 3);

    // one cnot, but needs one swap
    k.gate("cnot", 2,3);

    k.gate("x", 2);
    k.gate("x", 3);

    prog.add(k);

    ql::options::set("mapreverseswap", param1);
    ql::options::set("clifford_premapper", param2);
    ql::options::set("clifford_postmapper", param2);
    ql::options::set("scheduler_commute", param3);
    ql::options::set("prescheduler", param4);
    prog.compile( );
}

// one cnot with operands that are at distance 4 in s7
void
test_oneD4(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 5;
    std::string prog_name = "test_" + v + "_swapopt=" + param1 + "_clifford_premapper=" + param2 + "_schedulercommute=" + param3 + "_presched=" + param4;
    std::string kernel_name = "test_" + v + "_swapopt=" + param1 + "_clifford_premapper=" + param2 + "_schedulercommute=" + param3 + "_presched=" + param4;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    k.gate("x", 2);
    k.gate("x", 4);

    // one cnot, but needs several swaps
    k.gate("cnot", 2,4);

    k.gate("x", 2);
    k.gate("x", 4);

    prog.add(k);

    ql::options::set("mapreverseswap", param1);
    ql::options::set("clifford_premapper", param2);
    ql::options::set("clifford_postmapper", param2);
    ql::options::set("scheduler_commute", param3);
    ql::options::set("prescheduler", param4);
    prog.compile( );
}

// just test
void
test_oneD4Diogo(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 5;
    std::string prog_name = "test_" + v + "_swapopt=" + param1 + "_clifford_premapper=" + param2 + "_schedulercommute=" + param3 + "_presched=" + param4;
    std::string kernel_name = "test_" + v + "_swapopt=" + param1 + "_clifford_premapper=" + param2 + "_schedulercommute=" + param3 + "_presched=" + param4;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    k.gate("x", 2);
    k.gate("x", 4);

    // one cnot, but needs several swaps
    k.gate("cnot", 2,4);

    k.gate("x", 2);
    k.gate("x", 4);

    prog.add(k);

    k.gate("x", 2);     // demonstrates that prog.add(k) copies k as value, so k cannot be used anymore

    ql::options::set("mapreverseswap", param1);
    ql::options::set("clifford_premapper", param2);
    ql::options::set("clifford_postmapper", param2);
    ql::options::set("scheduler_commute", param3);
    ql::options::set("prescheduler", param4);
    prog.compile( );

    IOUT("AFTER test_oneD4Diogo prog.compile()");
    for(auto &kernel : prog.kernels)
    {
        IOUT("... kernel.name:" << kernel.name);
        IOUT("... kernel.c:" << kernel.qasm());
        IOUT("... kernel.bundles:" << ql::ir::qasm(kernel.bundles));
    }

    IOUT(k.qasm());
    for (auto gate : k.c )
        IOUT("Gate " + gate->name + "(" +  to_string(gate->operands.at(0)) + ") at cycle " + to_string(gate->cycle) );
}

// all possible cnots in s7, in lexicographic order
// requires many swaps
void
test_allD(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_swapopt=" + param1 + "_clifford_premapper=" + param2 + "_schedulercommute=" + param3 + "_presched=" + param4;
    std::string kernel_name = "test_" + v + "_swapopt=" + param1 + "_clifford_premapper=" + param2 + "_schedulercommute=" + param3 + "_presched=" + param4;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    for (int j=0; j<n; j++) { k.gate("x", j); }

    for (int i=0; i<n; i++) { for (int j=0; j<n; j++) { if (i != j) { k.gate("cnot", i,j); } } }

    for (int j=0; j<n; j++) { k.gate("x", j); }

    prog.add(k);

    ql::options::set("mapreverseswap", param1);
    ql::options::set("clifford_premapper", param2);
    ql::options::set("clifford_postmapper", param2);
    ql::options::set("scheduler_commute", param3);
    ql::options::set("prescheduler", param4);
    prog.compile( );
}

// all possible cnots in s7, avoiding collisions:
// - pairs in both directions together
// - from low distance to high distance
// - each time as much as possible in opposite sides of the circuit
void
test_allDopt(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_swapopt=" + param1 + "_clifford_premapper=" + param2 + "_schedulercommute=" + param3 + "_presched=" + param4;
    std::string kernel_name = "test_" + v + "_swapopt=" + param1 + "_clifford_premapper=" + param2 + "_schedulercommute=" + param3 + "_presched=" + param4;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    for (int j=0; j<n; j++) { k.gate("x", j); }

	k.gate("cnot", 0,3);
	k.gate("cnot", 3,0);

	k.gate("cnot", 6,4);
	k.gate("cnot", 4,6);

	k.gate("cnot", 3,1);
	k.gate("cnot", 1,3);

	k.gate("cnot", 5,2);
	k.gate("cnot", 2,5);

	k.gate("cnot", 1,4);
	k.gate("cnot", 4,1);

	k.gate("cnot", 3,5);
	k.gate("cnot", 5,3);

	k.gate("cnot", 6,3);
	k.gate("cnot", 3,6);

	k.gate("cnot", 2,0);
	k.gate("cnot", 0,2);

	k.gate("cnot", 0,1);
	k.gate("cnot", 1,0);

	k.gate("cnot", 3,4);
	k.gate("cnot", 4,3);

	k.gate("cnot", 1,6);
	k.gate("cnot", 6,1);

	k.gate("cnot", 6,5);
	k.gate("cnot", 5,6);

	k.gate("cnot", 3,2);
	k.gate("cnot", 2,3);

	k.gate("cnot", 5,0);
	k.gate("cnot", 0,5);

	k.gate("cnot", 0,6);
	k.gate("cnot", 6,0);

	k.gate("cnot", 1,5);
	k.gate("cnot", 5,1);

	k.gate("cnot", 0,4);
	k.gate("cnot", 4,0);

	k.gate("cnot", 6,2);
	k.gate("cnot", 2,6);

	k.gate("cnot", 2,1);
	k.gate("cnot", 1,2);

	k.gate("cnot", 5,4);
	k.gate("cnot", 4,5);

	k.gate("cnot", 2,4);
	k.gate("cnot", 4,2);

    for (int j=0; j<n; j++) { k.gate("x", j); }

    prog.add(k);

    ql::options::set("mapreverseswap", param1);
    ql::options::set("clifford_premapper", param2);
    ql::options::set("clifford_postmapper", param2);
    ql::options::set("scheduler_commute", param3);
    ql::options::set("prescheduler", param4);
    prog.compile( );
}

// longest string of cnots with operands that could be at distance 1 in s7
// matches intel NISQ application
// tests initial placement
void
test_string(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_swapopt=" + param1 + "_clifford_premapper=" + param2 + "_schedulercommute=" + param3 + "_presched=" + param4;
    std::string kernel_name = "test_" + v + "_swapopt=" + param1 + "_clifford_premapper=" + param2 + "_schedulercommute=" + param3 + "_presched=" + param4;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));


    for (int j=0; j<7; j++) { k.gate("x", j); }

    // string of cnots, a good initial placement prevents any swap
    k.gate("cnot", 0,1);
    k.gate("cnot", 1,2);
    k.gate("cnot", 2,3);
    k.gate("cnot", 3,4);
    k.gate("cnot", 4,5);
    k.gate("cnot", 5,6);

    for (int j=0; j<7; j++) { k.gate("x", j); }

    prog.add(k);

    ql::options::set("mapreverseswap", param1);
    ql::options::set("clifford_premapper", param2);
    ql::options::set("clifford_postmapper", param2);
    ql::options::set("scheduler_commute", param3);
    ql::options::set("prescheduler", param4);
    prog.compile( );
}

// actual test kernel of daniel that failed once
// because it caused use of a location that, before mapping heuristic was started, wasn't assigned to a used virtual qubit
// i.e. a location that didn't appear in the v2r map as location where the v2r is the initial map of the heuristic
void
test_daniel2(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 6;
    std::string prog_name = "test_" + v + "_swapopt=" + param1 + "_clifford_premapper=" + param2 + "_schedulercommute=" + param3 + "_presched=" + param4;
    std::string kernel_name = "test_" + v + "_swapopt=" + param1 + "_clifford_premapper=" + param2 + "_schedulercommute=" + param3 + "_presched=" + param4;
    float sweep_points[] = { 1, 2 };

    ql::quantum_platform starmon("starmon", "test_mapper17.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, n);

    ql::quantum_kernel k(kernel_name, starmon, n, 0);

    k.gate("x",0);
    k.gate("cnot",4,0);
    k.gate("h",0);
    k.gate("t",1);
    k.gate("t",5);
    k.gate("t",0);
    k.gate("cnot",5,1);
    k.gate("cnot",0,5);
    k.gate("cnot",1,0);
    k.gate("tdag",5);
    k.gate("cnot",1,5);
    k.gate("tdag",1);
    k.gate("tdag",5);
    k.gate("t",0);
    k.gate("cnot",0,5);
    k.gate("cnot",1,0);
    k.gate("cnot",5,1);
    k.gate("h",0);
    k.gate("h",5);
    k.gate("t",4);
    k.gate("t",2);
    k.gate("t",5);
    k.gate("cnot",2,4);
    k.gate("cnot",5,2);
    k.gate("cnot",4,5);
    k.gate("tdag",2);
    k.gate("cnot",4,2);
    k.gate("tdag",4);
    k.gate("tdag",2);
    k.gate("t",5);
    k.gate("cnot",5,2);
    k.gate("cnot",4,5);
    k.gate("cnot",2,4);
    k.gate("h",5);
    k.gate("h",0);
    k.gate("t",1);
    k.gate("t",5);
    k.gate("t",0);
    k.gate("cnot",5,1);
    k.gate("cnot",0,5);
    k.gate("cnot",1,0);
    k.gate("tdag",5);
    k.gate("cnot",1,5);
    k.gate("tdag",1);
    k.gate("tdag",5);
    k.gate("t",0);
    k.gate("cnot",0,5);
    k.gate("cnot",1,0);
    k.gate("cnot",5,1);
    k.gate("h",0);
    k.gate("h",5);
    k.gate("t",4);
    k.gate("t",2);
    k.gate("t",5);
    k.gate("cnot",2,4);
    k.gate("cnot",5,2);
    k.gate("cnot",4,5);
    k.gate("tdag",2);
    k.gate("cnot",4,2);
    k.gate("tdag",4);
    k.gate("tdag",2);
    k.gate("t",5);
    k.gate("cnot",5,2);
    k.gate("cnot",4,5);
    k.gate("cnot",2,4);
    k.gate("h",5);
    k.gate("x",4);
    k.gate("h",5);
    k.gate("t",4);
    k.gate("t",3);
    k.gate("t",5);
    k.gate("cnot",3,4);
    k.gate("cnot",5,3);
    k.gate("cnot",4,5);
    k.gate("tdag",3);
    k.gate("cnot",4,3);
    k.gate("tdag",4);
    k.gate("tdag",3);
    k.gate("t",5);
    k.gate("cnot",5,3);
    k.gate("cnot",4,5);
    k.gate("cnot",3,4);
    k.gate("h",5);
    k.gate("h",0);
    k.gate("t",5);
    k.gate("t",4);
    k.gate("t",0);
    k.gate("cnot",4,5);
    k.gate("cnot",0,4);
    k.gate("cnot",5,0);
    k.gate("tdag",4);
    k.gate("cnot",5,4);
    k.gate("tdag",5);
    k.gate("tdag",4);
    k.gate("t",0);
    k.gate("cnot",0,4);
    k.gate("cnot",5,0);
    k.gate("cnot",4,5);
    k.gate("h",0);
    k.gate("h",4);
    k.gate("t",2);
    k.gate("t",1);
    k.gate("t",4);
    k.gate("cnot",1,2);
    k.gate("cnot",4,1);
    k.gate("cnot",2,4);
    k.gate("tdag",1);
    k.gate("cnot",2,1);
    k.gate("tdag",2);
    k.gate("tdag",1);
    k.gate("t",4);
    k.gate("cnot",4,1);
    k.gate("cnot",2,4);
    k.gate("cnot",1,2);
    k.gate("h",4);
    k.gate("h",0);
    k.gate("t",5);
    k.gate("t",4);
    k.gate("t",0);
    k.gate("cnot",4,5);
    k.gate("cnot",0,4);
    k.gate("cnot",5,0);
    k.gate("tdag",4);
    k.gate("cnot",5,4);
    k.gate("tdag",5);
    k.gate("tdag",4);
    k.gate("t",0);
    k.gate("cnot",0,4);
    k.gate("cnot",5,0);
    k.gate("cnot",4,5);
    k.gate("h",0);
    k.gate("h",4);
    k.gate("t",2);
    k.gate("t",1);
    k.gate("t",4);
    k.gate("cnot",1,2);
    k.gate("cnot",4,1);
    k.gate("cnot",2,4);
    k.gate("tdag",1);
    k.gate("cnot",2,1);
    k.gate("tdag",2);
    k.gate("tdag",1);
    k.gate("t",4);
    k.gate("cnot",4,1);
    k.gate("cnot",2,4);
    k.gate("cnot",1,2);
    k.gate("h",4);
    k.gate("h",5);
    k.gate("t",4);
    k.gate("t",3);
    k.gate("t",5);
    k.gate("cnot",3,4);
    k.gate("cnot",5,3);
    k.gate("cnot",4,5);
    k.gate("tdag",3);
    k.gate("cnot",4,3);
    k.gate("tdag",4);
    k.gate("tdag",3);
    k.gate("t",5);
    k.gate("cnot",5,3);
    k.gate("cnot",4,5);
    k.gate("cnot",3,4);
    k.gate("h",5);
    k.gate("h",0);
    k.gate("t",5);
    k.gate("t",4);
    k.gate("t",0);
    k.gate("cnot",4,5);
    k.gate("cnot",0,4);
    k.gate("cnot",5,0);
    k.gate("tdag",4);
    k.gate("cnot",5,4);
    k.gate("tdag",5);
    k.gate("tdag",4);
    k.gate("t",0);
    k.gate("cnot",0,4);
    k.gate("cnot",5,0);
    k.gate("cnot",4,5);
    k.gate("h",0);
    k.gate("h",4);
    k.gate("t",2);
    k.gate("t",1);
    k.gate("t",4);
    k.gate("cnot",1,2);
    k.gate("cnot",4,1);
    k.gate("cnot",2,4);
    k.gate("tdag",1);
    k.gate("cnot",2,1);
    k.gate("tdag",2);
    k.gate("tdag",1);
    k.gate("t",4);
    k.gate("cnot",4,1);
    k.gate("cnot",2,4);
    k.gate("cnot",1,2);
    k.gate("h",4);
    k.gate("h",0);
    k.gate("t",5);
    k.gate("t",4);
    k.gate("t",0);
    k.gate("cnot",4,5);
    k.gate("cnot",0,4);
    k.gate("cnot",5,0);
    k.gate("tdag",4);
    k.gate("cnot",5,4);
    k.gate("tdag",5);
    k.gate("tdag",4);
    k.gate("t",0);
    k.gate("cnot",0,4);
    k.gate("cnot",5,0);
    k.gate("cnot",4,5);
    k.gate("h",0);
    k.gate("h",4);
    k.gate("t",2);
    k.gate("t",1);
    k.gate("t",4);
    k.gate("cnot",1,2);
    k.gate("cnot",4,1);
    k.gate("cnot",2,4);
    k.gate("tdag",1);
    k.gate("cnot",2,1);
    k.gate("tdag",2);
    k.gate("tdag",1);
    k.gate("t",4);
    k.gate("cnot",4,1);
    k.gate("cnot",2,4);
    k.gate("cnot",1,2);
    k.gate("h",4);
    k.gate("cnot",0,4);

    for (int q=0; q<n; q++)
    {
	    k.gate("measure", q);
    }

    prog.add(k);

    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    ql::options::set("mapreverseswap", param1);
    ql::options::set("clifford_premapper", param2);
    ql::options::set("clifford_postmapper", param2);
    ql::options::set("scheduler_commute", param3);
    ql::options::set("prescheduler", param4);
    prog.compile( );
}

void
test_lingling5esm(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_swapopt=" + param1 + "_clifford_premapper=" + param2 + "_schedulercommute=" + param3 + "_presched=" + param4;
    std::string kernel_name = "test_" + v + "_swapopt=" + param1 + "_clifford_premapper=" + param2 + "_schedulercommute=" + param3 + "_presched=" + param4;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper17.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    k.gate("prepz",5);
    k.gate("prepz",6);
    k.gate("x",5);
    k.gate("ym90",5);
    k.gate("x",6);
    k.gate("ym90",6);
    k.gate("ym90",0);
    k.gate("cz",5,0);
    k.gate("ry90",0);
    k.gate("x",5);
    k.gate("ym90",5);
    k.gate("ym90",5);
    k.gate("cz",6,5);
    k.gate("ry90",5);
    k.gate("ym90",5);
    k.gate("cz",1,5);
    k.gate("ry90",5);
    k.gate("ym90",5);
    k.gate("cz",2,5);
    k.gate("ry90",5);
    k.gate("ym90",5);
    k.gate("cz",6,5);
    k.gate("ry90",5);
    k.gate("x",5);
    k.gate("ym90",5);
    k.gate("ym90",3);
    k.gate("cz",5,3);
    k.gate("ry90",3);
    k.gate("x",5);
    k.gate("ym90",5);
    k.gate("measure",5);
    k.gate("measure",6);
    k.gate("prepz",5);
    k.gate("prepz",6);
    k.gate("x",5);
    k.gate("ym90",5);
    k.gate("x",6);
    k.gate("ym90",6);
    k.gate("ym90",1);
    k.gate("cz",5,1);
    k.gate("ry90",1);
    k.gate("x",5);
    k.gate("ym90",5);
    k.gate("ym90",5);
    k.gate("cz",6,5);
    k.gate("ry90",5);
    k.gate("ym90",5);
    k.gate("cz",2,5);
    k.gate("ry90",5);
    k.gate("ym90",5);
    k.gate("cz",3,5);
    k.gate("ry90",5);
    k.gate("ym90",5);
    k.gate("cz",6,5);
    k.gate("ry90",5);
    k.gate("x",5);
    k.gate("ym90",5);
    k.gate("ym90",4);
    k.gate("cz",5,4);
    k.gate("ry90",4);
    k.gate("x",5);
    k.gate("ym90",5);
    k.gate("measure",5);
    k.gate("measure",6);
    k.gate("prepz",5);
    k.gate("prepz",6);
    k.gate("x",5);
    k.gate("ym90",5);
    k.gate("x",6);
    k.gate("ym90",6);
    k.gate("ym90",2);
    k.gate("cz",5,2);
    k.gate("ry90",2);
    k.gate("x",5);
    k.gate("ym90",5);
    k.gate("ym90",5);
    k.gate("cz",6,5);
    k.gate("ry90",5);
    k.gate("ym90",5);
    k.gate("cz",3,5);
    k.gate("ry90",5);
    k.gate("ym90",5);
    k.gate("cz",4,5);
    k.gate("ry90",5);
    k.gate("ym90",5);
    k.gate("cz",6,5);
    k.gate("ry90",5);
    k.gate("x",5);
    k.gate("ym90",5);
    k.gate("ym90",0);
    k.gate("cz",5,0);
    k.gate("ry90",0);
    k.gate("x",5);
    k.gate("ym90",5);
    k.gate("measure",5);
    k.gate("measure",6);
    k.gate("prepz",5);
    k.gate("prepz",6);
    k.gate("x",5);
    k.gate("ym90",5);
    k.gate("x",6);
    k.gate("ym90",6);
    k.gate("ym90",3);
    k.gate("cz",5,3);
    k.gate("ry90",3);
    k.gate("x",5);
    k.gate("ym90",5);
    k.gate("ym90",5);
    k.gate("cz",6,5);
    k.gate("ry90",5);
    k.gate("ym90",5);
    k.gate("cz",4,5);
    k.gate("ry90",5);
    k.gate("ym90",5);
    k.gate("cz",0,5);
    k.gate("ry90",5);
    k.gate("ym90",5);
    k.gate("cz",6,5);
    k.gate("ry90",5);
    k.gate("x",5);
    k.gate("ym90",5);
    k.gate("ym90",1);
    k.gate("cz",5,1);
    k.gate("ry90",1);
    k.gate("x",5);
    k.gate("ym90",5);
    k.gate("measure",5);
    k.gate("measure",6);

    prog.add(k);

    ql::options::set("mapreverseswap", param1);
    ql::options::set("clifford_premapper", param2);
    ql::options::set("clifford_postmapper", param2);
    ql::options::set("scheduler_commute", param3);
    ql::options::set("prescheduler", param4);
    prog.compile( );
}

void
test_lingling7esm(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 9;
    std::string prog_name = "test_" + v + "_swapopt=" + param1 + "_clifford_premapper=" + param2 + "_schedulercommute=" + param3 + "_presched=" + param4;
    std::string kernel_name = "test_" + v + "_swapopt=" + param1 + "_clifford_premapper=" + param2 + "_schedulercommute=" + param3 + "_presched=" + param4;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper17.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    k.gate("prepz",7);
    k.gate("prepz",8);
    k.gate("x",7);
    k.gate("ym90",7);
    k.gate("ym90",4);
    k.gate("cz",7,4);
    k.gate("ry90",4);
    k.gate("ym90",8);
    k.gate("cz",0,8);
    k.gate("ry90",8);
    k.gate("ym90",8);
    k.gate("cz",7,8);
    k.gate("ry90",8);
    k.gate("ym90",6);
    k.gate("cz",7,6);
    k.gate("ry90",6);
    k.gate("ym90",8);
    k.gate("cz",2,8);
    k.gate("ry90",8);
    k.gate("ym90",3);
    k.gate("cz",7,3);
    k.gate("ry90",3);
    k.gate("ym90",8);
    k.gate("cz",4,8);
    k.gate("ry90",8);
    k.gate("ym90",8);
    k.gate("cz",7,8);
    k.gate("ry90",8);
    k.gate("ym90",5);
    k.gate("cz",7,5);
    k.gate("ry90",5);
    k.gate("ym90",8);
    k.gate("cz",6,8);
    k.gate("ry90",8);
    k.gate("x",7);
    k.gate("ym90",7);
    k.gate("measure",7);
    k.gate("measure",8);
    k.gate("prepz",7);
    k.gate("prepz",8);
    k.gate("x",7);
    k.gate("ym90",7);
    k.gate("ym90",5);
    k.gate("cz",7,5);
    k.gate("ry90",5);
    k.gate("ym90",8);
    k.gate("cz",1,8);
    k.gate("ry90",8);
    k.gate("ym90",8);
    k.gate("cz",7,8);
    k.gate("ry90",8);
    k.gate("ym90",6);
    k.gate("cz",7,6);
    k.gate("ry90",6);
    k.gate("ym90",8);
    k.gate("cz",2,8);
    k.gate("ry90",8);
    k.gate("ym90",3);
    k.gate("cz",7,3);
    k.gate("ry90",3);
    k.gate("ym90",8);
    k.gate("cz",5,8);
    k.gate("ry90",8);
    k.gate("ym90",8);
    k.gate("cz",7,8);
    k.gate("ry90",8);
    k.gate("ym90",4);
    k.gate("cz",7,4);
    k.gate("ry90",4);
    k.gate("ym90",8);
    k.gate("cz",6,8);
    k.gate("ry90",8);
    k.gate("x",7);
    k.gate("ym90",7);
    k.gate("measure",7);
    k.gate("measure",8);
    k.gate("prepz",7);
    k.gate("prepz",8);
    k.gate("x",7);
    k.gate("ym90",7);
    k.gate("ym90",1);
    k.gate("cz",7,1);
    k.gate("ry90",1);
    k.gate("ym90",8);
    k.gate("cz",2,8);
    k.gate("ry90",8);
    k.gate("ym90",8);
    k.gate("cz",7,8);
    k.gate("ry90",8);
    k.gate("ym90",5);
    k.gate("cz",7,5);
    k.gate("ry90",5);
    k.gate("ym90",8);
    k.gate("cz",6,8);
    k.gate("ry90",8);
    k.gate("ym90",2);
    k.gate("cz",7,2);
    k.gate("ry90",2);
    k.gate("ym90",8);
    k.gate("cz",0,8);
    k.gate("ry90",8);
    k.gate("ym90",8);
    k.gate("cz",7,8);
    k.gate("ry90",8);
    k.gate("ym90",6);
    k.gate("cz",7,6);
    k.gate("ry90",6);
    k.gate("ym90",8);
    k.gate("cz",4,8);
    k.gate("ry90",8);
    k.gate("x",7);
    k.gate("ym90",7);
    k.gate("measure",7);
    k.gate("measure",8);
    prog.add(k);

    ql::options::set("mapreverseswap", param1);
    ql::options::set("clifford_premapper", param2);
    ql::options::set("clifford_postmapper", param2);
    ql::options::set("scheduler_commute", param3);
    ql::options::set("prescheduler", param4);
    prog.compile( );
}


int main(int argc, char ** argv)
{
    // ql::utils::logger::set_log_level("LOG_DEBUG");
    ql::utils::logger::set_log_level("LOG_NOTHING");

#ifdef DEVELOP
//parameter2    ql::options::set("clifford_premapper", "yes"); 
    ql::options::set("mapper", "minextendrc"); 
    ql::options::set("mapinitone2one", "yes"); 
    ql::options::set("maplookahead", "noroutingfirst");
    ql::options::set("initialplace", "no"); 
    ql::options::set("initialplaceprefix", "10"); 
    ql::options::set("mappathselect", "all"); 
    ql::options::set("mapusemoves", "yes"); 
//parameter1    ql::options::set("mapreverseswap, "yes"); 
    ql::options::set("maptiebreak", "first"); 

//parameter2    ql::options::set("clifford_postmapper", "yes"); 
    ql::options::set("scheduler_post179", "yes");
    ql::options::set("scheduler", "ALAP");
//parameter3    ql::options::set("scheduler_commute", "yes");
//parameter4    ql::options::set("prescheduler", "no");
#endif

    ql::options::set("mapper", "base"); 
    ql::options::set("mapinitone2one", "yes"); 
    ql::options::set("maplookahead", "noroutingfirst");
    ql::options::set("initialplace", "no"); 
    ql::options::set("initialplaceprefix", "10"); 
    ql::options::set("mappathselect", "all"); 
    ql::options::set("mapusemoves", "yes"); 
//parameter1    ql::options::set("mapreverseswap, "yes"); 
    ql::options::set("maptiebreak", "first"); 

//parameter2    ql::options::set("clifford_postmapper", "yes"); 
    ql::options::set("scheduler_post179", "yes");
    ql::options::set("scheduler", "ALAP");
//parameter3    ql::options::set("scheduler_commute", "yes");
//parameter4    ql::options::set("prescheduler", "no");

//  test_danielt("danielt", "minextendrc", "no", "no", "no");
//  test_danielt("danielt", "minextendrc", "yes", "no", "no");
//  test_danielt("danielt", "minextendrc", "no", "yes", "no");
//  test_danielt("danielt", "minextendrc", "yes", "yes", "no");

//  test_daniel2("daniel2", "base", "yes", "critical", "no");
//  test_daniel2("daniel2", "base", "yes", "critical", "no");
//  test_daniel2("daniel2", "minextend", "yes", "critical", "no");
//  test_daniel2("daniel2", "minextendrc", "yes", "no", "no");
//  test_daniel2("daniel2", "minextendrc", "yes", "critical", "no");
//  test_daniel2("daniel2", "yes", "no", "no", "no");
//  test_daniel2("daniel2", "yes", "yes", "no", "no");
//  test_daniel2("daniel2", "yes", "no", "yes", "no");
//  test_daniel2("daniel2", "yes", "yes", "yes", "no");
//  test_daniel2("daniel2", "yes", "yes", "yes", "yes");
//  test_daniel2("daniel2", "no", "no", "no", "no");
//  test_daniel2("daniel2", "no", "yes", "no", "no");
//  test_daniel2("daniel2", "no", "no", "yes", "no");
//  test_daniel2("daniel2", "no", "yes", "yes", "no");

//  test_oneD2("oneD2", "base", "yes", "critical", "no");
//  test_oneD2("oneD2", "base", "yes", "critical", "no");
//  test_oneD2("oneD2", "minextend", "yes", "critical", "no");
//  test_oneD2("oneD2", "minextendrc", "yes", "critical", "no");
//  test_oneD2("oneD2", "yes", "yes", "yes", "no");
//  test_oneD2("oneD2", "yes", "yes", "yes", "yes");

//  test_oneD4("oneD4", "base", "yes", "critical", "no");
//  test_oneD4("oneD4", "minextend", "yes", "critical", "no");
//  test_oneD4("oneD4", "minextendrc", "yes", "critical", "no");

//  test_oneD4Diogo("oneD4Diogo", "base", "yes", "critical", "no");

//  test_string("string", "base", "yes", "critical", "no");
//  test_string("string", "base", "yes", "critical", "no");
//  test_string("string", "minextend", "yes", "critical", "no");
//  test_string("string", "minextendrc", "yes", "no", "no");
//  test_string("string", "minextendrc", "yes", "critical", "no");
//  test_string("string", "yes", "no", "no", "no");
//  test_string("string", "yes", "yes", "no", "no");
//  test_string("string", "yes", "no", "yes", "no");
//  test_string("string", "yes", "yes", "yes", "no");
//  test_string("string", "yes", "yes", "yes", "yes");
//  test_string("string", "no", "no", "no", "no");
//  test_string("string", "no", "yes", "no", "no");
//  test_string("string", "no", "no", "yes", "no");
//  test_string("string", "no", "yes", "yes", "no");

//  test_allD("allD", "base", "yes", "no", "no");
//  test_allD("allD", "base", "yes", "critical", "no");
//  test_allD("allD", "base", "yes", "noroutingfirst", "no");
//  test_allD("allD", "base", "yes", "all", "no");
//  test_allD("allD", "minextend", "yes", "no", "no");
//  test_allD("allD", "minextend", "yes", "critical", "no");
//  test_allD("allD", "minextend", "yes", "noroutingfirst", "no");
//  test_allD("allD", "minextend", "yes", "all", "no");
//  test_allD("allD", "minextendrc", "yes", "no", "no");
//  test_allD("allD", "minextendrc", "yes", "critical", "no");
//  test_allD("allD", "yes", "no", "no", "no");
//  test_allD("allD", "yes", "yes", "no", "no");
//  test_allD("allD", "yes", "no", "yes", "no");
//  test_allD("allD", "yes", "yes", "yes", "no");
//  test_allD("allD", "yes", "yes", "yes", "yes");
    test_allD("allD", "no", "no", "no", "no");
//  test_allD("allD", "no", "yes", "no", "no");
//  test_allD("allD", "no", "no", "yes", "no");
//  test_allD("allD", "no", "yes", "yes", "no");

//  test_allDopt("allDopt", "base", "yes", "critical", "no");
//  test_allDopt("allDopt", "base", "yes", "critical", "no");
//  test_allDopt("allDopt", "minextend", "yes", "critical", "no");
//  test_allDopt("allDopt", "minextendrc", "yes", "no", "no");
//  test_allDopt("allDopt", "minextendrc", "yes", "critical", "no");
//  test_allDopt("allDopt", "yes", "no", "no", "no");
//  test_allDopt("allDopt", "yes", "yes", "no", "no");
//  test_allDopt("allDopt", "yes", "no", "yes", "no");
//  test_allDopt("allDopt", "yes", "yes", "yes", "no");
//  test_allDopt("allDopt", "yes", "yes", "yes", "yes");
//  test_allDopt("allDopt", "no", "no", "no", "no");
//  test_allDopt("allDopt", "no", "yes", "no", "no");
//  test_allDopt("allDopt", "no", "no", "yes", "no");
//  test_allDopt("allDopt", "no", "yes", "yes", "no");

//  test_lingling5esm("lingling5esm", "base", "yes", "critical", "no");
//  test_lingling5esm("lingling5esm", "base", "yes", "critical", "no");
//  test_lingling5esm("lingling5esm", "minextend", "yes", "critical", "no");
//  test_lingling5esm("lingling5esm", "minextendrc", "yes", "no", "no");
//  test_lingling5esm("lingling5esm", "minextendrc", "yes", "critical", "no");
//  test_lingling5esm("lingling5esm", "yes", "no", "no", "no");
//  test_lingling5esm("lingling5esm", "yes", "yes", "no", "no");
//  test_lingling5esm("lingling5esm", "yes", "no", "yes", "no");
//  test_lingling5esm("lingling5esm", "yes", "yes", "yes", "no");
//  test_lingling5esm("lingling5esm", "yes", "yes", "yes", "yes");
//  test_lingling5esm("lingling5esm", "no", "no", "no", "no");
//  test_lingling5esm("lingling5esm", "no", "yes", "no", "no");
//  test_lingling5esm("lingling5esm", "no", "no", "yes", "no");
//  test_lingling5esm("lingling5esm", "no", "yes", "yes", "no");

//  test_lingling7esm("lingling7esm", "base", "yes", "critical", "no");
//  test_lingling7esm("lingling7esm", "base", "yes", "critical", "no");
//  test_lingling7esm("lingling7esm", "minextend", "yes", "critical", "no");
//  test_lingling7esm("lingling7esm", "minextend", "yes", "noroutingfirst", "no");
//  test_lingling7esm("lingling7esm", "minextendrc", "yes", "no", "no");
//  test_lingling7esm("lingling7esm", "minextendrc", "yes", "critical", "no");
//  test_lingling7esm("lingling7esm", "yes", "no", "no", "no");
//  test_lingling7esm("lingling7esm", "yes", "yes", "no", "no");
//  test_lingling7esm("lingling7esm", "yes", "no", "yes", "no");
//  test_lingling7esm("lingling7esm", "yes", "yes", "yes", "no");
//  test_lingling7esm("lingling7esm", "yes", "yes", "yes", "yes");
//  test_lingling7esm("lingling7esm", "no", "no", "no", "no");
//  test_lingling7esm("lingling7esm", "no", "yes", "no", "no");
//  test_lingling7esm("lingling7esm", "no", "no", "yes", "no");
//  test_lingling7esm("lingling7esm", "no", "yes", "yes", "no");

    return 0;
}
