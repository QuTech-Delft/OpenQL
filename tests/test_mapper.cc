#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>

#include <time.h>

#include <openql.h>

// rc test
void
test_rc(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_mapselectswaps=" + param2 + "_mapreverseswap=" + param3;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_mapselectswaps=" + param2 + "_mapreverseswap=" + param3;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    // no dependency, only a conflict in qwg resource
    k.gate("x", 0);
    k.gate("y", 1);

    prog.add(k);

    ql::options::set("maplookahead", param1);
    ql::options::set("mapselectswaps", param2);
    ql::options::set("mapreverseswap", param3);

    prog.compile( );
}

// some cnots with operands that are neighbors in s7
void
test_someNN(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_mapselectswaps=" + param2 + "_mapreverseswap=" + param3;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_mapselectswaps=" + param2 + "_mapreverseswap=" + param3;
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

    ql::options::set("maplookahead", param1);
    ql::options::set("mapselectswaps", param2);
    ql::options::set("mapreverseswap", param3);

    prog.compile( );
}

// all cnots with operands that are neighbors in s7
void
test_manyNN(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_mapselectswaps=" + param2 + "_mapreverseswap=" + param3;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_mapselectswaps=" + param2 + "_mapreverseswap=" + param3;
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

    ql::options::set("maplookahead", param1);
    ql::options::set("mapselectswaps", param2);
    ql::options::set("mapreverseswap", param3);

    prog.compile( );
}

// one cnot with operands that are at distance 2 in s7
void
test_oneD2(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 4;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_mapselectswaps=" + param2 + "_mapreverseswap=" + param3;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_mapselectswaps=" + param2 + "_mapreverseswap=" + param3;
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

    ql::options::set("maplookahead", param1);
    ql::options::set("mapselectswaps", param2);
    ql::options::set("mapreverseswap", param3);

    prog.compile( );
}

// one cnot with operands that are at distance 4 in s7
void
test_oneD4(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 5;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_mapselectswaps=" + param2 + "_mapreverseswap=" + param3;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_mapselectswaps=" + param2 + "_mapreverseswap=" + param3;
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

    ql::options::set("maplookahead", param1);
    ql::options::set("mapselectswaps", param2);
    ql::options::set("mapreverseswap", param3);

    prog.compile( );

#undef DIOGO
#ifdef DIOGO
    IOUT("AFTER test_oneD4Diogo prog.compile()");
    for(auto &kernel : prog.kernels)
    {
        IOUT("... kernel.name:" << kernel.name);
        IOUT("... kernel.c:" << kernel.qasm());
        IOUT("... kernel.bundles:" << ql::ir::qasm(kernel.bundles));
    }

    IOUT(prog.kernels.at(0).qasm());
    for (auto gate : prog.kernels.at(0).c )
        IOUT("Gate " + gate->name + "(" +  to_string(gate->operands.at(0)) + ") at cycle " + to_string(gate->cycle) );
#endif
}

// all possible cnots in s7, in lexicographic order
// requires many swaps
void
test_allD(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_mapselectswaps=" + param2 + "_mapreverseswap=" + param3;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_mapselectswaps=" + param2 + "_mapreverseswap=" + param3;
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

    ql::options::set("maplookahead", param1);
    ql::options::set("mapselectswaps", param2);
    ql::options::set("mapreverseswap", param3);

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
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_mapselectswaps=" + param2 + "_mapreverseswap=" + param3;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_mapselectswaps=" + param2 + "_mapreverseswap=" + param3;
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

    ql::options::set("maplookahead", param1);
    ql::options::set("mapselectswaps", param2);
    ql::options::set("mapreverseswap", param3);

    prog.compile( );
}

// longest string of cnots with operands that could be at distance 1 in s7
// matches intel NISQ application
// tests initial placement
void
test_string(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_mapselectswaps=" + param2 + "_mapreverseswap=" + param3;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_mapselectswaps=" + param2 + "_mapreverseswap=" + param3;
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

    ql::options::set("maplookahead", param1);
    ql::options::set("mapselectswaps", param2);
    ql::options::set("mapreverseswap", param3);

    prog.compile( );
}

// actual test kernel of daniel that failed once
// because it caused use of a location that, before mapping heuristic was started, wasn't assigned to a used virtual qubit
// i.e. a location that didn't appear in the v2r map as location where the v2r is the initial map of the heuristic
void
test_daniel2(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 6;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_mapselectswaps=" + param2 + "_mapreverseswap=" + param3;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_mapselectswaps=" + param2 + "_mapreverseswap=" + param3;
    float sweep_points[] = { 1, 2 };

    ql::quantum_platform starmon("starmon", "test_mapper17.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, n);

    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));


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

    ql::options::set("maplookahead", param1);
    ql::options::set("mapselectswaps", param2);
    ql::options::set("mapreverseswap", param3);

    prog.compile( );
}

void
test_lingling5esm(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_mapselectswaps=" + param2 + "_mapreverseswap=" + param3;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_mapselectswaps=" + param2 + "_mapreverseswap=" + param3;
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

    ql::options::set("maplookahead", param1);
    ql::options::set("mapselectswaps", param2);
    ql::options::set("mapreverseswap", param3);

    prog.compile( );
}

void
test_lingling7esm(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 9;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_mapselectswaps=" + param2 + "_mapreverseswap=" + param3;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_mapselectswaps=" + param2 + "_mapreverseswap=" + param3;
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

    ql::options::set("maplookahead", param1);
    ql::options::set("mapselectswaps", param2);
    ql::options::set("mapreverseswap", param3);

    prog.compile( );
}


int main(int argc, char ** argv)
{
    ql::utils::logger::set_log_level("LOG_DEBUG");
    // ql::utils::logger::set_log_level("LOG_NOTHING");
    ql::options::set("write_qasm_files", "yes"); 
    ql::options::set("write_report_files", "yes"); 
    ql::options::set("print_dot_graphs", "yes"); 

    ql::options::set("clifford_premapper", "yes"); 
    ql::options::set("mapper", "minextendrc"); 
    ql::options::set("mapinitone2one", "yes"); 
//parameter1  ql::options::set("maplookahead", "noroutingfirst");
//parameter2  ql::options::set("mapselectswaps", "all");
    ql::options::set("initialplace", "1m"); 
    ql::options::set("initialplaceprefix", "10"); 
    ql::options::set("mappathselect", "all"); 
    ql::options::set("mapusemoves", "yes"); 
//parameter3  ql::options::set("mapreverseswap", "yes"); 
    ql::options::set("maptiebreak", "first"); 

    ql::options::set("clifford_postmapper", "yes"); 
    ql::options::set("scheduler_post179", "yes");
    ql::options::set("scheduler", "ALAP");
    ql::options::set("scheduler_commute", "yes");
    ql::options::set("prescheduler", "no");

//  NN:
//  test_rc("rc", "no", "no", "yes", "no");
//  test_someNN("someNN", "no", "no", "yes", "yes");

//  nonNN but solvable by Initial Placement:
//  test_oneD2("oneD2", "yes", "yes", "yes", "yes");
//  test_oneD4("oneD4", "yes", "yes", "yes", "yes");

    test_string("string", "all", "all", "no", "yes");
    test_string("string", "all", "all", "yes", "yes");
    test_string("string", "all", "earliest", "no", "yes");
    test_string("string", "all", "earliest", "yes", "yes");
    test_string("string", "all", "one", "no", "yes");
    test_string("string", "all", "one", "yes", "yes");
    test_string("string", "noroutingfirst", "all", "no", "yes");
    test_string("string", "noroutingfirst", "all", "yes", "yes");
    test_string("string", "noroutingfirst", "earliest", "no", "yes");
    test_string("string", "noroutingfirst", "earliest", "yes", "yes");
    test_string("string", "noroutingfirst", "one", "no", "yes");
    test_string("string", "noroutingfirst", "one", "yes", "yes");

//  nonNN, still not too large:
    test_allD("allD", "all", "all", "no", "yes");
    test_allD("allD", "all", "all", "yes", "yes");
    test_allD("allD", "all", "earliest", "no", "yes");
    test_allD("allD", "all", "earliest", "yes", "yes");
    test_allD("allD", "all", "one", "no", "yes");
    test_allD("allD", "all", "one", "yes", "yes");
    test_allD("allD", "noroutingfirst", "all", "no", "yes");
    test_allD("allD", "noroutingfirst", "all", "yes", "yes");
    test_allD("allD", "noroutingfirst", "earliest", "no", "yes");
    test_allD("allD", "noroutingfirst", "earliest", "yes", "yes");
    test_allD("allD", "noroutingfirst", "one", "no", "yes");
    test_allD("allD", "noroutingfirst", "one", "yes", "yes");

    test_allDopt("allDopt", "all", "all", "no", "yes");
    test_allDopt("allDopt", "all", "all", "yes", "yes");
    test_allDopt("allDopt", "all", "earliest", "no", "yes");
    test_allDopt("allDopt", "all", "earliest", "yes", "yes");
    test_allDopt("allDopt", "all", "one", "no", "yes");
    test_allDopt("allDopt", "all", "one", "yes", "yes");
    test_allDopt("allDopt", "noroutingfirst", "all", "no", "yes");
    test_allDopt("allDopt", "noroutingfirst", "all", "yes", "yes");
    test_allDopt("allDopt", "noroutingfirst", "earliest", "no", "yes");
    test_allDopt("allDopt", "noroutingfirst", "earliest", "yes", "yes");
    test_allDopt("allDopt", "noroutingfirst", "one", "no", "yes");
    test_allDopt("allDopt", "noroutingfirst", "one", "yes", "yes");

//  nonNN, realistic:
    test_daniel2("daniel2", "all", "all", "no", "yes");
    test_daniel2("daniel2", "all", "all", "yes", "yes");
    test_daniel2("daniel2", "all", "earliest", "no", "yes");
    test_daniel2("daniel2", "all", "earliest", "yes", "yes");
    test_daniel2("daniel2", "all", "one", "no", "yes");
    test_daniel2("daniel2", "all", "one", "yes", "yes");
    test_daniel2("daniel2", "noroutingfirst", "all", "no", "yes");
    test_daniel2("daniel2", "noroutingfirst", "all", "yes", "yes");
    test_daniel2("daniel2", "noroutingfirst", "earliest", "no", "yes");
    test_daniel2("daniel2", "noroutingfirst", "earliest", "yes", "yes");
    test_daniel2("daniel2", "noroutingfirst", "one", "no", "yes");
    test_daniel2("daniel2", "noroutingfirst", "one", "yes", "yes");

    test_lingling5esm("lingling5esm", "all", "all", "no", "yes");
    test_lingling5esm("lingling5esm", "all", "all", "yes", "yes");
    test_lingling5esm("lingling5esm", "all", "earliest", "no", "yes");
    test_lingling5esm("lingling5esm", "all", "earliest", "yes", "yes");
    test_lingling5esm("lingling5esm", "all", "one", "no", "yes");
    test_lingling5esm("lingling5esm", "all", "one", "yes", "yes");
    test_lingling5esm("lingling5esm", "noroutingfirst", "all", "no", "yes");
    test_lingling5esm("lingling5esm", "noroutingfirst", "all", "yes", "yes");
    test_lingling5esm("lingling5esm", "noroutingfirst", "earliest", "no", "yes");
    test_lingling5esm("lingling5esm", "noroutingfirst", "earliest", "yes", "yes");
    test_lingling5esm("lingling5esm", "noroutingfirst", "one", "no", "yes");
    test_lingling5esm("lingling5esm", "noroutingfirst", "one", "yes", "yes");

    test_lingling7esm("lingling7esm", "all", "all", "no", "yes");
    test_lingling7esm("lingling7esm", "all", "all", "yes", "yes");
    test_lingling7esm("lingling7esm", "all", "earliest", "no", "yes");
    test_lingling7esm("lingling7esm", "all", "earliest", "yes", "yes");
    test_lingling7esm("lingling7esm", "all", "one", "no", "yes");
    test_lingling7esm("lingling7esm", "all", "one", "yes", "yes");
    test_lingling7esm("lingling7esm", "noroutingfirst", "all", "no", "yes");
    test_lingling7esm("lingling7esm", "noroutingfirst", "all", "yes", "yes");
    test_lingling7esm("lingling7esm", "noroutingfirst", "earliest", "no", "yes");
    test_lingling7esm("lingling7esm", "noroutingfirst", "earliest", "yes", "yes");
    test_lingling7esm("lingling7esm", "noroutingfirst", "one", "no", "yes");
    test_lingling7esm("lingling7esm", "noroutingfirst", "one", "yes", "yes");

    return 0;
}
