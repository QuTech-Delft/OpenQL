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
test_qwg(std::string v, std::string mapopt, std::string initialplaceopt, std::string post179opt)
{
    int n = 2;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
    std::string kernel_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
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
    ql::options::set("mapper", mapopt);
    ql::options::set("initialplace", initialplaceopt);
    ql::options::set("scheduler_post179", post179opt);
    prog.compile( );
}

// demo single dimension resource constraint representation simple
void
test_singledim(std::string v, std::string mapopt, std::string initialplaceopt, std::string post179opt)
{
    int n = 5;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
    std::string kernel_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper.json");
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
    ql::options::set("mapper", mapopt);
    ql::options::set("initialplace", initialplaceopt);
    ql::options::set("scheduler_post179", post179opt);
    prog.compile( );
}

// test edge resource constraints mapping
void
test_edge(std::string v, std::string mapopt, std::string initialplaceopt, std::string post179opt)
{
    int n = 5;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
    std::string kernel_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    // no dependency, only a conflict in edge resource
    k.gate("cz", 1,4);
    k.gate("cz", 0,3);

    prog.add(k);
    ql::options::set("mapper", mapopt);
    ql::options::set("initialplace", initialplaceopt);
    ql::options::set("scheduler_post179", post179opt);
    prog.compile( );
}

// test detuned_qubits resource constraints mapping
// no swaps generated
void
test_detuned(std::string v, std::string mapopt, std::string initialplaceopt, std::string post179opt)
{
    int n = 5;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
    std::string kernel_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper.json");
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
    ql::options::set("mapper", mapopt);
    ql::options::set("initialplace", initialplaceopt);
    ql::options::set("scheduler_post179", post179opt);
    prog.compile( );
}

// one cnot with operands that are neighbors in s7
void
test_oneNN(std::string v, std::string mapopt, std::string initialplaceopt, std::string post179opt)
{
    int n = 3;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
    std::string kernel_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper.json");
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

    ql::options::set("mapper", mapopt);
    ql::options::set("initialplace", initialplaceopt);
    ql::options::set("scheduler_post179", post179opt);
    prog.compile( );
}

// all cnots with operands that are neighbors in s7
void
test_manyNN(std::string v, std::string mapopt, std::string initialplaceopt, std::string post179opt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
    std::string kernel_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
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
    ql::options::set("initialplace", initialplaceopt);
    ql::options::set("scheduler_post179", post179opt);
    prog.compile( );
}

// one cnot with operands that are at distance 2 in s7
void
test_oneD2(std::string v, std::string mapopt, std::string initialplaceopt, std::string post179opt)
{
    int n = 4;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
    std::string kernel_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
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
    ql::options::set("initialplace", initialplaceopt);
    ql::options::set("scheduler_post179", post179opt);
    prog.compile( );
}

// one cnot with operands that are at distance 4 in s7
void
test_oneD4(std::string v, std::string mapopt, std::string initialplaceopt, std::string post179opt)
{
    int n = 5;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
    std::string kernel_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
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
    ql::options::set("initialplace", initialplaceopt);
    ql::options::set("scheduler_post179", post179opt);
    prog.compile( );
}

// all possible cnots in s7, in lexicographic order
// requires many swaps
void
test_allD(std::string v, std::string mapopt, std::string initialplaceopt, std::string post179opt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
    std::string kernel_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
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
    ql::options::set("initialplace", initialplaceopt);
    ql::options::set("scheduler_post179", post179opt);
    prog.compile( );
}

// all possible cnots in s7, avoiding collisions:
// - pairs in both directions together
// - from low distance to high distance
// - each time as much as possible in opposite sides of the circuit
void
test_allDopt(std::string v, std::string mapopt, std::string initialplaceopt, std::string post179opt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
    std::string kernel_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
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
    ql::options::set("initialplace", initialplaceopt);
    ql::options::set("scheduler_post179", post179opt);
    prog.compile( );
}

// longest string of cnots with operands that could be at distance 1 in s7
// matches intel NISQ application
// tests initial placement
void
test_string(std::string v, std::string mapopt, std::string initialplaceopt, std::string post179opt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
    std::string kernel_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
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
    ql::options::set("initialplace", initialplaceopt);
    ql::options::set("scheduler_post179", post179opt);
    prog.compile( );
}

// simple kernel originating from two kernel example of daniel
// will be modified to two kernels when inter kernel control flow is supported
void
test_daniel(std::string v, std::string mapopt, std::string initialplaceopt, std::string post179opt)
{
    int n = 2;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, n);

    ql::quantum_kernel k("entanglement", starmon, n, 0);
    k.gate("h", 0);
    k.gate("cnot", 0,1);
    k.gate("measure", 0);
    k.gate("measure", 1);
    // k.gate("measure", std::vector<size_t>{0}, std::vector<size_t>{0});
    // k.gate("measure", std::vector<size_t>{1}, std::vector<size_t>{1});
    prog.add(k);

    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    ql::options::set("mapper", mapopt);
    ql::options::set("initialplace", initialplaceopt);
    ql::options::set("scheduler_post179", post179opt);
    prog.compile( );
}

// actual test kernel of daniel that failed once
// because it caused use of a location that, before mapping heuristic was started, wasn't assigned to a used virtual qubit
// i.e. a location that didn't appear in the v2r map as location where the v2r is the initial map of the heuristic
void
test_daniel2(std::string v, std::string mapopt, std::string initialplaceopt, std::string post179opt)
{
    int n = 6;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
    std::string kernel_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
    float sweep_points[] = { 1, 2 };

    ql::quantum_platform starmon("starmon", "constraints_configuration_quantumsim_sc17.json");
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
    ql::options::set("initialplace", initialplaceopt);
    ql::options::set("scheduler_post179", post179opt);
    prog.compile( );
}

void
test_lingling_5_esm(std::string v, std::string mapopt, std::string initialplaceopt, std::string post179opt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
    std::string kernel_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "constraints_configuration_quantumsim_sc17.json");
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
    ql::options::set("initialplace", initialplaceopt);
    ql::options::set("scheduler_post179", post179opt);
    prog.compile( );
}

void
test_lingling_7_esm(std::string v, std::string mapopt, std::string initialplaceopt, std::string post179opt)
{
    int n = 9;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
    std::string kernel_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_scheduler_post179=" + post179opt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "constraints_configuration_quantumsim_sc17.json");
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
    ql::options::set("initialplace", initialplaceopt);
    ql::options::set("scheduler_post179", post179opt);
    prog.compile( );
}


int main(int argc, char ** argv)
{
    ql::utils::logger::set_log_level("LOG_DEBUG");
    ql::options::set("scheduler", "ALAP");
    ql::options::set("mapdecomposer", "yes");   // always decompose to primitives
    ql::options::set("mapusemoves", "no"); 
    ql::options::set("maptiebreak", "first"); 

//  test_singledim("singledim", "minextend", "no", "no");
//  test_singledim("singledim", "minextendrc", "yes", "no");
//  test_singledim("singledim", "minextendrc", "no", "no");

//  test_qwg("qwg", "minextendrc", "yes", "no");
//  test_qwg("qwg", "minextend", "no", "no");
//  test_qwg("qwg", "minextendrc", "no", "no");

//  test_edge("edge", "minextendrc", "yes", "no");
//  test_edge("edge", "minextend", "no", "no");
//  test_edge("edge", "minextendrc", "no", "no");

//  test_detuned("detuned", "minextendrc", "yes", "no");
//  test_detuned("detuned", "minextend", "no", "no");
//  test_detuned("detuned", "minextendrc", "no", "no");

//  test_oneNN("oneNN", "base", "yes", "no");
//  test_oneNN("oneNN", "minextend", "yes", "no");
//  test_oneNN("oneNN", "minextendrc", "no", "no");
//  test_oneNN("oneNN", "minextendrc", "yes", "no");

//  test_manyNN("manyNN", "base", "yes", "no");
//  test_manyNN("manyNN", "minextend", "yes", "no");
//  test_manyNN("manyNN", "minextendrc", "no", "no");
//  test_manyNN("manyNN", "minextendrc", "yes", "no");
    
//  test_daniel("daniel", "minextend", "no", "no");
//  test_daniel("daniel", "minextend", "no", "yes");
//  test_daniel("daniel", "minextendrc", "no", "no");
//  test_daniel("daniel", "minextendrc", "no", "yes");

//  test_daniel2("daniel2", "minextend", "no", "no");
//  test_daniel2("daniel2", "minextend", "no", "yes");

//  test_daniel2("daniel2", "minextendrc", "no", "no");
//  test_daniel2("daniel2", "minextendrc", "no", "yes");

//  test_oneD2("oneD2", "base", "no", "no");
//  test_oneD2("oneD2", "base", "no", "yes");
//  test_oneD2("oneD2", "minextend", "no", "no");
//  test_oneD2("oneD2", "minextend", "no", "yes");
//  test_oneD2("oneD2", "minextend", "no", "no");
//  test_oneD2("oneD2", "minextend", "no", "yes");
//  test_oneD2("oneD2", "minextendrc", "no", "no");
//  test_oneD2("oneD2", "minextendrc", "no", "yes");

//  test_oneD4("oneD4", "base", "no", "no");
//  test_oneD4("oneD4", "base", "no", "yes");
//  test_oneD4("oneD4", "minextendrc", "no", "no");
//  test_oneD4("oneD4", "minextendrc", "no", "yes");

//  test_string("string", "base", "no", "no");
//  test_string("string", "base", "no", "yes");
    test_string("string", "minextend", "no", "no");
    test_string("string", "minextend", "no", "yes");
//  test_string("string", "minextendrc", "no", "no");
//  test_string("string", "minextendrc", "no", "yes");

//  test_allD("allD", "base", "no", "no");
//  test_allD("allD", "base", "no", "yes");
//  test_allD("allD", "minextend", "no", "no");
//  test_allD("allD", "minextend", "no", "yes");
//  test_allD("allD", "minextend", "no", "no");
//  test_allD("allD", "minextend", "no", "yes");
//  test_allD("allD", "minextendrc", "no", "no");
//  test_allD("allD", "minextendrc", "no", "yes");

//  test_allDopt("allDopt", "base", "no", "no");
//  test_allDopt("allDopt", "base", "no", "yes");
//  test_allDopt("allDopt", "minextend", "no", "no");
//  test_allDopt("allDopt", "minextend", "no", "yes");
//  test_allDopt("allDopt", "minextend", "no", "no");
//  test_allDopt("allDopt", "minextend", "no", "yes");
//  test_allDopt("allDopt", "minextendrc", "no", "no");
//  test_allDopt("allDopt", "minextendrc", "no", "yes");

//  test_lingling_5_esm("lingling_5_esm", "minextendrc", "no", "no");
//  test_lingling_5_esm("lingling_5_esm", "minextendrc", "no", "yes");
//  test_lingling_7_esm("lingling_7_esm", "minextendrc", "no", "no");
//  test_lingling_7_esm("lingling_7_esm", "minextendrc", "no", "yes");

    return 0;
}
