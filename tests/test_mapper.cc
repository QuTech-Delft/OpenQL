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
test_manyNN(std::string v, std::string mapopt, std::string scheduler_commuteopt, std::string maplookaheadopt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_scheduler_commute=" + scheduler_commuteopt + "_maplookahead=" + maplookaheadopt;
    std::string kernel_name = "test_" + v + "_mapopt=" + mapopt + "_scheduler_commute=" + scheduler_commuteopt + "_maplookahead=" + maplookaheadopt;
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

    ql::options::set("mapper", mapopt);
    ql::options::set("scheduler_commute", scheduler_commuteopt);
    ql::options::set("maplookahead", maplookaheadopt);
    prog.compile( );
}

// one cnot with operands that are at distance 2 in s7
void
test_oneD2(std::string v, std::string mapopt, std::string scheduler_commuteopt, std::string maplookaheadopt)
{
    int n = 4;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_scheduler_commute=" + scheduler_commuteopt + "_maplookahead=" + maplookaheadopt;
    std::string kernel_name = "test_" + v + "_mapopt=" + mapopt + "_scheduler_commute=" + scheduler_commuteopt + "_maplookahead=" + maplookaheadopt;
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

    ql::options::set("mapper", mapopt);
    ql::options::set("scheduler_commute", scheduler_commuteopt);
    ql::options::set("maplookahead", maplookaheadopt);
    prog.compile( );
}

// one cnot with operands that are at distance 4 in s7
void
test_oneD4(std::string v, std::string mapopt, std::string scheduler_commuteopt, std::string maplookaheadopt)
{
    int n = 5;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_scheduler_commute=" + scheduler_commuteopt + "_maplookahead=" + maplookaheadopt;
    std::string kernel_name = "test_" + v + "_mapopt=" + mapopt + "_scheduler_commute=" + scheduler_commuteopt + "_maplookahead=" + maplookaheadopt;
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

    ql::options::set("mapper", mapopt);
    ql::options::set("scheduler_commute", scheduler_commuteopt);
    ql::options::set("maplookahead", maplookaheadopt);
    prog.compile( );
}

// all possible cnots in s7, in lexicographic order
// requires many swaps
void
test_allD(std::string v, std::string mapopt, std::string scheduler_commuteopt, std::string maplookaheadopt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_scheduler_commute=" + scheduler_commuteopt + "_maplookahead=" + maplookaheadopt;
    std::string kernel_name = "test_" + v + "_mapopt=" + mapopt + "_scheduler_commute=" + scheduler_commuteopt + "_maplookahead=" + maplookaheadopt;
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

    ql::options::set("mapper", mapopt);
    ql::options::set("scheduler_commute", scheduler_commuteopt);
    ql::options::set("maplookahead", maplookaheadopt);
    prog.compile( );
}

// all possible cnots in s7, avoiding collisions:
// - pairs in both directions together
// - from low distance to high distance
// - each time as much as possible in opposite sides of the circuit
void
test_allDopt(std::string v, std::string mapopt, std::string scheduler_commuteopt, std::string maplookaheadopt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_scheduler_commute=" + scheduler_commuteopt + "_maplookahead=" + maplookaheadopt;
    std::string kernel_name = "test_" + v + "_mapopt=" + mapopt + "_scheduler_commute=" + scheduler_commuteopt + "_maplookahead=" + maplookaheadopt;
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

    ql::options::set("mapper", mapopt);
    ql::options::set("scheduler_commute", scheduler_commuteopt);
    ql::options::set("maplookahead", maplookaheadopt);
    prog.compile( );
}

// longest string of cnots with operands that could be at distance 1 in s7
// matches intel NISQ application
// tests initial placement
void
test_string(std::string v, std::string mapopt, std::string scheduler_commuteopt, std::string maplookaheadopt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_scheduler_commute=" + scheduler_commuteopt + "_maplookahead=" + maplookaheadopt;
    std::string kernel_name = "test_" + v + "_mapopt=" + mapopt + "_scheduler_commute=" + scheduler_commuteopt + "_maplookahead=" + maplookaheadopt;
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

    ql::options::set("mapper", mapopt);
    ql::options::set("scheduler_commute", scheduler_commuteopt);
    ql::options::set("maplookahead", maplookaheadopt);
    prog.compile( );
}

// actual test kernel of daniel that failed once
// because it caused use of a location that, before mapping heuristic was started, wasn't assigned to a used virtual qubit
// i.e. a location that didn't appear in the v2r map as location where the v2r is the initial map of the heuristic
void
test_daniel2(std::string v, std::string mapopt, std::string scheduler_commuteopt, std::string maplookaheadopt)
{
    int n = 6;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_scheduler_commute=" + scheduler_commuteopt + "_maplookahead=" + maplookaheadopt;
    std::string kernel_name = "test_" + v + "_mapopt=" + mapopt + "_scheduler_commute=" + scheduler_commuteopt + "_maplookahead=" + maplookaheadopt;
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

    ql::options::set("mapper", mapopt);
    ql::options::set("scheduler_commute", scheduler_commuteopt);
    ql::options::set("maplookahead", maplookaheadopt);
    prog.compile( );
}

void
test_lingling_5_esm(std::string v, std::string mapopt, std::string scheduler_commuteopt, std::string maplookaheadopt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_scheduler_commute=" + scheduler_commuteopt + "_maplookahead=" + maplookaheadopt;
    std::string kernel_name = "test_" + v + "_mapopt=" + mapopt + "_scheduler_commute=" + scheduler_commuteopt + "_maplookahead=" + maplookaheadopt;
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

    ql::options::set("mapper", mapopt);
    ql::options::set("scheduler_commute", scheduler_commuteopt);
    ql::options::set("maplookahead", maplookaheadopt);
    prog.compile( );
}

void
test_lingling_7_esm(std::string v, std::string mapopt, std::string scheduler_commuteopt, std::string maplookaheadopt)
{
    int n = 9;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_scheduler_commute=" + scheduler_commuteopt + "_maplookahead=" + maplookaheadopt;
    std::string kernel_name = "test_" + v + "_mapopt=" + mapopt + "_scheduler_commute=" + scheduler_commuteopt + "_maplookahead=" + maplookaheadopt;
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

    ql::options::set("mapper", mapopt);
    ql::options::set("scheduler_commute", scheduler_commuteopt);
    ql::options::set("maplookahead", maplookaheadopt);
    prog.compile( );
}


int main(int argc, char ** argv)
{
    ql::utils::logger::set_log_level("LOG_DEBUG");
    ql::options::set("scheduler", "ALAP");
    ql::options::set("mapinitone2one", "yes"); 
    ql::options::set("initialplace", "no"); 
    ql::options::set("mapusemoves", "yes"); 
    ql::options::set("maptiebreak", "first"); 
    ql::options::set("mappathselect", "all"); 
    ql::options::set("mapdecomposer", "no");

//  test_daniel2("daniel2", "base", "yes", "critical");
//  test_daniel2("daniel2", "base", "yes", "critical");
//  test_daniel2("daniel2", "minextend", "yes", "critical");
    test_daniel2("daniel2", "minextendrc", "yes", "no");
    test_daniel2("daniel2", "minextendrc", "yes", "critical");
    test_daniel2("daniel2", "minextendrc", "yes", "noroutingfirst");
    test_daniel2("daniel2", "minextendrc", "yes", "all");

//  test_oneD2("oneD2", "base", "yes", "critical");
//  test_oneD2("oneD2", "base", "yes", "critical");
//  test_oneD2("oneD2", "minextend", "yes", "critical");
//  test_oneD2("oneD2", "minextendrc", "yes", "critical");

//  test_oneD4("oneD4", "base", "yes", "critical");
//  test_oneD4("oneD4", "minextend", "yes", "critical");
//  test_oneD4("oneD4", "minextendrc", "yes", "critical");

//  test_string("string", "base", "yes", "critical");
//  test_string("string", "base", "yes", "critical");
//  test_string("string", "minextend", "yes", "critical");
    test_string("string", "minextendrc", "yes", "no");
    test_string("string", "minextendrc", "yes", "critical");
    test_string("string", "minextendrc", "yes", "noroutingfirst");
    test_string("string", "minextendrc", "yes", "all");

//  test_allD("allD", "base", "yes", "no");
//  test_allD("allD", "base", "yes", "critical");
//  test_allD("allD", "base", "yes", "noroutingfirst");
//  test_allD("allD", "minextend", "yes", "no");
//  test_allD("allD", "minextend", "yes", "critical");
//  test_allD("allD", "minextend", "yes", "noroutingfirst");
    test_allD("allD", "minextendrc", "yes", "no");
    test_allD("allD", "minextendrc", "yes", "critical");
    test_allD("allD", "minextendrc", "yes", "noroutingfirst");
    test_allD("allD", "minextendrc", "yes", "all");

//  test_allDopt("allDopt", "base", "yes", "critical");
//  test_allDopt("allDopt", "base", "yes", "critical");
//  test_allDopt("allDopt", "minextend", "yes", "critical");
    test_allDopt("allDopt", "minextendrc", "yes", "no");
    test_allDopt("allDopt", "minextendrc", "yes", "critical");
    test_allDopt("allDopt", "minextendrc", "yes", "noroutingfirst");
    test_allDopt("allDopt", "minextendrc", "yes", "all");

//  test_lingling_5_esm("lingling_5_esm", "base", "yes", "critical");
//  test_lingling_5_esm("lingling_5_esm", "base", "yes", "critical");
//  test_lingling_5_esm("lingling_5_esm", "minextend", "yes", "critical");
    test_lingling_5_esm("lingling_5_esm", "minextendrc", "yes", "no");
    test_lingling_5_esm("lingling_5_esm", "minextendrc", "yes", "critical");
    test_lingling_5_esm("lingling_5_esm", "minextendrc", "yes", "noroutingfirst");
    test_lingling_5_esm("lingling_5_esm", "minextendrc", "yes", "all");

//  test_lingling_7_esm("lingling_7_esm", "base", "yes", "critical");
//  test_lingling_7_esm("lingling_7_esm", "base", "yes", "critical");
//  test_lingling_7_esm("lingling_7_esm", "minextend", "yes", "critical");
    test_lingling_7_esm("lingling_7_esm", "minextendrc", "yes", "no");
    test_lingling_7_esm("lingling_7_esm", "minextendrc", "yes", "critical");
    test_lingling_7_esm("lingling_7_esm", "minextendrc", "yes", "noroutingfirst");
    test_lingling_7_esm("lingling_7_esm", "minextendrc", "yes", "all");

    return 0;
}
