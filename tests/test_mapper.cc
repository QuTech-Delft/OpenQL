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
test_qwg(std::string v, std::string mapopt, std::string initialplaceopt, std::string configfile)
{
    int n = 2;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_json=" + configfile;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_json=" + configfile;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon",configfile);
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    // no dependency, only a conflict in qwg resource
    k.gate("x", 0);
    k.gate("y", 1);

    prog.add(k);
    ql::options::set("mapper", mapopt);
    ql::options::set("initialplace", initialplaceopt);
    prog.compile( );
}

// demo single dimension resource constraint representation simple
void
test_singledim(std::string v, std::string mapopt, std::string initialplaceopt, std::string configfile)
{
    int n = 5;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_json=" + configfile;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_json=" + configfile;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon",configfile);
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);
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
    prog.compile( );
}

// test edge resource constraints mapping
void
test_edge(std::string v, std::string mapopt, std::string initialplaceopt, std::string configfile)
{
    int n = 5;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_json=" + configfile;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_json=" + configfile;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon",configfile);
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    // no dependency, only a conflict in edge resource
    k.gate("cz", 1,4);
    k.gate("cz", 0,3);

    prog.add(k);
    ql::options::set("mapper", mapopt);
    ql::options::set("initialplace", initialplaceopt);
    prog.compile( );
}

// test detuned_qubits resource constraints mapping
// no swaps generated
void
test_detuned(std::string v, std::string mapopt, std::string initialplaceopt, std::string configfile)
{
    int n = 5;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_json=" + configfile;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_json=" + configfile;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon",configfile);
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);
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
    prog.compile( );
}

// one cnot with operands that are neighbors in s7
void
test_oneNN(std::string v, std::string mapopt, std::string initialplaceopt, std::string configfile)
{
    int n = 3;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_json=" + configfile;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_json=" + configfile;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon",configfile);
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);
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
    prog.compile( );
}

// all cnots with operands that are neighbors in s7
void
test_manyNN(std::string v, std::string mapopt, std::string initialplaceopt, std::string configfile)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_json=" + configfile;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_json=" + configfile;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon",configfile);
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);
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
    prog.compile( );
}

// one cnot with operands that are at distance 2 in s7
void
test_oneD2(std::string v, std::string mapopt, std::string initialplaceopt, std::string configfile)
{
    int n = 4;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_json=" + configfile;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_json=" + configfile;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon",configfile);
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);
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
    prog.compile( );
}

// one cnot with operands that are at distance 4 in s7
void
test_oneD4(std::string v, std::string mapopt, std::string initialplaceopt, std::string configfile)
{
    int n = 5;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_json=" + configfile;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_json=" + configfile;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon",configfile);
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);
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
    prog.compile( );
}

// all possible cnots in s7, in lexicographic order
// requires many swaps
void
test_allD(std::string v, std::string mapopt, std::string initialplaceopt, std::string configfile)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_json=" + configfile;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_json=" + configfile;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon",configfile);
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    for (int j=0; j<n; j++) { k.gate("x", j); }

    for (int i=0; i<n; i++) { for (int j=0; j<n; j++) { if (i != j) { k.gate("cnot", i,j); } } }

    for (int j=0; j<n; j++) { k.gate("x", j); }

    prog.add(k);

    ql::options::set("mapper", mapopt);
    ql::options::set("initialplace", initialplaceopt);
    prog.compile( );
}

// all possible cnots in s7, avoiding collisions:
// - pairs in both directions together
// - from low distance to high distance
// - each time as much as possible in opposite sides of the circuit
void
test_allDopt(std::string v, std::string mapopt, std::string initialplaceopt, std::string configfile)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_json=" + configfile;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_json=" + configfile;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon",configfile);
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);
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
    prog.compile( );
}

// longest string of cnots with operands that could be at distance 1 in s7
// matches intel NISQ application
// tests initial placement
void
test_string(std::string v, std::string mapopt, std::string initialplaceopt, std::string configfile)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_json=" + configfile;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_initplace=" + initialplaceopt + "_json=" + configfile;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon",configfile);
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);
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
    prog.compile( );
}


int main(int argc, char ** argv)
{
    auto    configfile = ql::options::get("configuration_file");
    if ("" == configfile)
    {
        // perhaps from Python, the above options work from a command line, from C++ they don't hence the following lines ...
        if (argc >= 2)
        {
            // bluntly assume first argument (if any) is the name of the configuration file
            // don't test, don't take the '/'s out, ...
            configfile = argv[1];
        }
        else
        {
            configfile = "test_mapper.json";
        }
    }

    ql::utils::logger::set_log_level("LOG_DEBUG");
    ql::options::set("scheduler", "no");        // still run rc cc_light scheduler afterwards!
    ql::options::set("mapdecomposer", "yes");   // always decompose to primitives

//    test_singledim("singledim", "minextendrc", "yes", configfile);

//    test_qwg("qwg", "minextendrc", "yes", configfile);
//    test_edge("edge", "minextendrc", "yes", configfile);
//    test_detuned("detuned", "minextendrc", "yes", configfile);

//    test_oneNN("oneNN", "base", "yes", configfile);
//    test_oneNN("oneNN", "minextend", "yes", configfile);
//    test_oneNN("oneNN", "minextendrc", "yes", configfile);

//    test_manyNN("manyNN", "base", "yes", configfile);
//    test_manyNN("manyNN", "minextend", "yes", configfile);
//    test_manyNN("manyNN", "minextendrc", "yes", configfile);
    
//    test_oneD2("oneD2", "base", "yes", configfile);
//    test_oneD2("oneD2", "minextend", "yes", configfile);
//    test_oneD2("oneD2", "minextendrc", "yes", configfile);

//    test_oneD4("oneD4", "base", "yes", configfile);
//    test_oneD4("oneD4", "minextend", "yes", configfile);
//    test_oneD4("oneD4", "minextendrc", "yes", configfile);

//    test_string("string", "base", "yes", configfile);
//    test_string("string", "minextend", "yes", configfile);
//    test_string("string", "minextendrc", "yes", configfile);

    test_allD("allD", "base", "yes", configfile);
    test_allD("allD", "minextend", "yes", configfile);
    test_allD("allD", "minextendrc", "yes", configfile);

//    test_allDopt("allDopt", "base", "yes", configfile);
//    test_allDopt("allDopt", "minextend", "yes", configfile);
//    test_allDopt("allDopt", "minextendrc", "yes", configfile);

    return 0;
}
