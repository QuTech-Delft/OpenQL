

#include <openql_i.h>

namespace ql {
    double quick_fidelity_circuit(ql::circuit);
}

void
test_recursion(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper_s7.json");
    //ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    for (int j=0; j<n; j++) { k.gate("x", j); }

    for (int i=0; i<n; i++) { for (int j=0; j<n; j++) { if (i != j) { k.gate("cnot", i,j); } } }

    for (int j=0; j<n; j++) { k.gate("x", j); }

    prog.add(k);

    ql::options::set("maplookahead", param1);
    ql::options::set("maprecNN2q", param2);
    ql::options::set("mapselectmaxlevel", param3);
    ql::options::set("mapselectmaxwidth", param4);

    prog.compile( );
}

void
test_diogo(std::string v, std::string param1, std::string param2, std::string param3)
{
    int n = 17;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapper=" + param3;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapper=" + param3;

    float sweep_points[] = { 1 };


    ql::quantum_platform starmon("starmon", "test_mapper_s17.json");
    //ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    k.gate("x", 1);
    k.gate("cz", 6,7);
    k.gate("cz", 5,6);
    k.gate("cz", 1,5);

    prog.add(k);

    ql::options::set("maplookahead", param1);
    ql::options::set("maprecNN2q", param2);
    ql::options::set("mapper", param3);

    ql::options::set("clifford_premapper", "no");
    ql::options::set("clifford_postmapper", "no");
    ql::options::set("scheduler_post179", "yes");
    ql::options::set("scheduler", "ALAP");

    ql::options::set("quantumsim", "yes");

    prog.compile( );

    double qf = ql::quick_fidelity_circuit(k.c);

    IOUT("Final Fidelity: " << qf);
    IOUT("THE END!");
}

void
test_diogo2(std::string v, std::string param1, std::string param2, std::string param3)
{
    int n = 17;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapper=" + param3;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapper=" + param3;

    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper_s17.json");
    //ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    k.gate("x", 1);
    k.gate("cz", 6,7);
    k.gate("cz", 5,6);
    k.gate("cz", 1,5);
    // k.gate("y", 2);
    // k.gate("h", 3);
    // k.gate("measure", 2);
    // k.gate("measure", 3);


    prog.add(k);

    ql::options::set("maplookahead", param1);
    ql::options::set("maprecNN2q", param2);
    ql::options::set("mapper", param3);

    ql::options::set("clifford_premapper", "no");
    ql::options::set("clifford_postmapper", "no");
    ql::options::set("scheduler_post179", "yes");
    ql::options::set("scheduler", "ALAP");

    ql::options::set("quantumsim", "qsoverlay");

    prog.compile( );

    double qf = ql::quick_fidelity_circuit(k.c);

    IOUT("Final Fidelity: " << qf);
    IOUT("THE END!");
}

// simple program to test (post179) dot printing by the scheduler
// excludes mapper
void
test_dot(std::string v, std::string param1, std::string param2)
{
    int n = 4;
    std::string prog_name = "test_" + v + "_scheduler_post179=" + param1 + "_scheduler=" + param2;
    std::string kernel_name = "test_" + v + "_scheduler_post179=" + param1 + "_scheduler=" + param2;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper_s7.json");
    //ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    k.gate("x", 0);
    k.gate("x", 3);

    // one cnot, no swap
    k.gate("cnot", 0,3);

    k.gate("x", 0);
    k.gate("x", 3);

    prog.add(k);

    ql::options::set("mapper", "no");

    ql::options::set("scheduler_post179", param1);
    ql::options::set("scheduler", param2);

    prog.compile( );
}

// resource constraint presence test
// the resource constraints of qwg prohibit both gates to execute in a single cycle
// no non-NN two-qubit gates so mapper neutral
void
test_rc(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper_s7.json");
    //ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    // no dependency, only a conflict in qwg resource
    k.gate("x", 0);
    k.gate("y", 1);

    prog.add(k);

    ql::options::set("maplookahead", param1);
    ql::options::set("maprecNN2q", param2);
    ql::options::set("mapselectmaxlevel", param3);
    ql::options::set("mapselectmaxwidth", param4);

    prog.compile( );
}

// all cnots (in both directions) with operands that are neighbors in s7
// no non-NN two-qubit gates so mapper neutral
void
test_someNN(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper_s7.json");
    //ql::set_platform(starmon);
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
    ql::options::set("maprecNN2q", param2);
    ql::options::set("mapselectmaxlevel", param3);
    ql::options::set("mapselectmaxwidth", param4);

    prog.compile( );
}

// one cnot with operands that are at distance 2 in s7
// just one two-qubit gate at the smallest non-NN distance so needs mapper;
// initial placement will find a solution, otherwise ...
// with distance 2 there are already 4 variations to map; each generates just one swap
// so it basically tests path finding, placing a cnot in a path,
// generating swap code into each alternative, and comparing the alternatives;
// but these are all equally optimal so it at most tests the tiebreak to force a selection at the end
void
test_oneD2(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 4;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper_s7.json");
    //ql::set_platform(starmon);
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
    ql::options::set("maprecNN2q", param2);
    ql::options::set("mapselectmaxlevel", param3);
    ql::options::set("mapselectmaxwidth", param4);

    prog.compile( );
}

// one cnot with operands that are at distance 4 in s7
// just one two-qubit gate at some bigger non-NN distance so needs mapper;
// initial placement will find a solution, otherwise ...
// with distance 4 there are already 12 variations to map; each generates 3 swaps;
// with multiple swaps to insert, it will find a meet-in-the-middle solution as optimal one,
// but there are several of these, and the combination of path finding and tiebreak will decide which
void
test_oneD4(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 5;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper_s7.json");
    //ql::set_platform(starmon);
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
    ql::options::set("maprecNN2q", param2);
    ql::options::set("mapselectmaxlevel", param3);
    ql::options::set("mapselectmaxwidth", param4);

    prog.compile( );
}

// longest string of cnots with operands that could be at distance 1 in s7 when initially placed correctly
// matches intel NISQ application
// tests initial placement
// when initial placement is not done, the mapper heuristic just sees a string of dependent cnots
// and will map them one by one; since it will start from a trivial mapping
// in which virtual qubit 0/1/2..6 will be mapped to real qubit 0/1/2..6,
// it will probably leave 0 or 1 where it is and move the other one,
// which already precludes the most optimal solution;
// lookahead, minextend and recursion (selectmaxlevel, selectmaxwidth and recNN2q) influence the result
void
test_string(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper_s7.json");
    //ql::set_platform(starmon);
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
    ql::options::set("maprecNN2q", param2);
    ql::options::set("mapselectmaxlevel", param3);
    ql::options::set("mapselectmaxwidth", param4);

    prog.compile( );
}

// all possible cnots in s7, avoiding collisions:
// - pairs in both directions together
// - from low distance to high distance (minimizing disturbance)
// - each time as much as possible in opposite sides of the circuit (maximizing ILP)
// the original order in the circuit seems to be an optimal one to do the mapping,
// but lookahead and minextend try to find an optimal solution;
// still the result of allDopt will be better dan of allD
void
test_allDopt(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper_s7.json");
    //ql::set_platform(starmon);
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
    ql::options::set("maprecNN2q", param2);
    ql::options::set("mapselectmaxlevel", param3);
    ql::options::set("mapselectmaxwidth", param4);

    prog.compile( );
}

// all possible cnots in s7, in lexicographic order
// requires many, many swaps
// the many cnots allow commutation, the big ILP generates many alternatives,
// so critical path selection and/or recursion really pay off;
// nevertheless, this is artifical code, the worst to map,
// so what does being able to map it optimally say about mapping real circuits?
void
test_allD(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper_s7.json");
    //ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    for (int j=0; j<n; j++) { k.gate("x", j); }

    for (int i=0; i<n; i++) { for (int j=0; j<n; j++) { if (i != j) { k.gate("cnot", i,j); } } }

    for (int j=0; j<n; j++) { k.gate("x", j); }

    prog.add(k);

    ql::options::set("maplookahead", param1);
    ql::options::set("maprecNN2q", param2);
    ql::options::set("mapselectmaxlevel", param3);
    ql::options::set("mapselectmaxwidth", param4);

    prog.compile( );
}

void
test_allD2(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper_s7.json");
    //ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    for (int j=0; j<n; j++) { k.gate("x", j); }

    //for (int i=0; i<n; i++) { for (int j=0; j<n; j++) { if (i < j) { k.gate("cnot", i,j); } } }

    //k.gate("cnot", 0,0);
    k.gate("cnot", 0,1);
    k.gate("cnot", 0,2);
    k.gate("cnot", 0,3);
    k.gate("cnot", 0,4);
    k.gate("cnot", 0,5);
    k.gate("cnot", 0,6);
    k.gate("cnot", 1,0);
    //k.gate("cnot", 1,1);
    k.gate("cnot", 1,2);
    k.gate("cnot", 1,3);
    k.gate("cnot", 1,4);
    k.gate("cnot", 1,5);
    k.gate("cnot", 1,6);
    k.gate("cnot", 2,0);
    k.gate("cnot", 2,1);
    //k.gate("cnot", 2,2);
    k.gate("cnot", 2,3);
    k.gate("cnot", 2,4);
    k.gate("cnot", 2,5);
    k.gate("cnot", 2,6);
    k.gate("cnot", 3,0);
    k.gate("cnot", 3,1);
    k.gate("cnot", 3,2);
    //k.gate("cnot", 3,3);
    k.gate("cnot", 3,4);
    k.gate("cnot", 3,5);
    k.gate("cnot", 3,6);
    k.gate("cnot", 4,0);
    k.gate("cnot", 4,1);
    k.gate("cnot", 4,2);
    k.gate("cnot", 4,3);
    //k.gate("cnot", 4,4);
    k.gate("cnot", 4,5);
    k.gate("cnot", 4,6);
    k.gate("cnot", 5,0);
    k.gate("cnot", 5,1);
    k.gate("cnot", 5,2);
    k.gate("cnot", 5,3);
    k.gate("cnot", 5,4);
    //k.gate("cnot", 5,5);
    k.gate("cnot", 5,6);
    k.gate("cnot", 6,0);
    k.gate("cnot", 6,1);
    k.gate("cnot", 6,2);
    k.gate("cnot", 6,3);
    k.gate("cnot", 6,4);
    k.gate("cnot", 6,5);
    //k.gate("cnot", 6,6);

    for (int j=0; j<n; j++) { k.gate("x", j); }

    prog.add(k);

    ql::options::set("maplookahead", param1);
    ql::options::set("maprecNN2q", param2);
    ql::options::set("mapselectmaxlevel", param3);
    ql::options::set("mapselectmaxwidth", param4);

    prog.compile( );
}

// actual test kernel of daniel that failed once
// because it caused use of a location that, before mapping heuristic was started,
// wasn't assigned to a used virtual qubit; i.e. a location that didn't appear in the v2r map as location
// where the v2r is the initial map of the heuristic;
// so this tests moves, qubit initialization, qubit states, adding ancilla's;
// also the circuit has more gates (around 220) than those above (around 50);
// and it executes on s17 (although it should also run on s7)
void
test_daniel2(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 6;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    float sweep_points[] = { 1, 2 };

    ql::quantum_platform starmon("starmon", "test_mapper_s17.json");
    //ql::set_platform(starmon);
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
    ql::options::set("maprecNN2q", param2);
    ql::options::set("mapselectmaxlevel", param3);
    ql::options::set("mapselectmaxwidth", param4);

    prog.compile( );
}

// real code with 5-qubit short error code checkers in 4 variations next to eachother
// must fit somehow in s17
void
test_lingling5esm(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper_s17.json");
    //ql::set_platform(starmon);
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
    ql::options::set("maprecNN2q", param2);
    ql::options::set("mapselectmaxlevel", param3);
    ql::options::set("mapselectmaxwidth", param4);

    prog.compile( );
}

// real code with 7-qubit short error code checkers in 3 variations next to eachother
// must fit somehow in s17
void
test_lingling7esm(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 9;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper_s17.json");
    //ql::set_platform(starmon);
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
    ql::options::set("maprecNN2q", param2);
    ql::options::set("mapselectmaxlevel", param3);
    ql::options::set("mapselectmaxwidth", param4);

    prog.compile( );
}

// real code with 7-qubit short error code checkers in 3 variations next to eachother
// must fit somehow in s17
void
test_lingling7sub(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 9;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper_s17.json");
    //ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

#define SUB1    1

#ifdef SUB1
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
#endif

#ifdef SUB2
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
#endif

#ifdef SUB3
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
#endif

    prog.add(k);

    ql::options::set("maplookahead", param1);
    ql::options::set("maprecNN2q", param2);
    ql::options::set("mapselectmaxlevel", param3);
    ql::options::set("mapselectmaxwidth", param4);

    prog.compile( );
}

// a maxcut QAOA algorithm inspired by the one in Venturelli et al [2017]'s paper
// Temporal planning for compilation of quantum approximate optimization circuits
// meant to run on an architecture inspired by an 8 bit Rigetti prototype from that paper;
// the topology has 'holes' so there are less alternatives and using a longer path than the minimal
// one might pay off in finding an optimal minimal latency;
// and the swaps take only 2 cycles, where a cz takes 3 or 4 cycles,
// so there is a different balance during evaluation of alternatives
void
test_maxcut(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 8;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper_rig.json");
    //ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    k.gate("cz", 1,4);
    k.gate("cz", 1,3);
    k.gate("cz", 3,4);
    k.gate("cz", 3,7);
    k.gate("cz", 4,7);
    k.gate("cz", 6,7);
    k.gate("cz", 5,6);
    k.gate("cz", 1,5);

    k.gate("x", 1);
    k.gate("x", 3);
    k.gate("x", 4);
    k.gate("x", 5);
    k.gate("x", 6);
    k.gate("x", 7);

    /*
    k.gate("cz", 1,4);
    k.gate("cz", 1,3);
    k.gate("cz", 3,4);
    k.gate("cz", 3,7);
    k.gate("cz", 4,7);
    k.gate("cz", 6,7);
    k.gate("cz", 5,6);
    k.gate("cz", 1,5);
    */

    prog.add(k);

    ql::options::set("maplookahead", param1);
    ql::options::set("maprecNN2q", param2);
    ql::options::set("mapselectmaxlevel", param3);
    ql::options::set("mapselectmaxwidth", param4);

    prog.compile( );
}


int main(int argc, char ** argv)
{
    ql::utils::logger::set_log_level("LOG_DEBUG");
    // ql::utils::logger::set_log_level("LOG_NOTHING");
    ql::options::set("unique_output", "yes");

    ql::options::set("write_qasm_files", "yes");
    ql::options::set("write_report_files", "yes");
    ql::options::set("print_dot_graphs", "yes");
    ql::options::set("use_default_gates", "no");

    ql::options::set("clifford_premapper", "yes");
    ql::options::set("mapper", "minextendrc");
    ql::options::set("mapinitone2one", "yes");
//parameter1  ql::options::set("maplookahead", "noroutingfirst");
    ql::options::set("mapselectswaps", "all");
    ql::options::set("initialplace", "no");
    ql::options::set("initialplace2qhorizon", "10");
    ql::options::set("mappathselect", "all");
    ql::options::set("mapusemoves", "yes");
    ql::options::set("mapreverseswap", "yes");
//parameter3  ql::options::set("mapselectmaxlevel", "0");
//parameter2  ql::options::set("maprecNN2q", "no");
//parameter4  ql::options::set("mapselectmaxwidth", "min");
    ql::options::set("maptiebreak", "random");

    ql::options::set("clifford_postmapper", "yes");
    ql::options::set("scheduler_post179", "yes");
    ql::options::set("scheduler", "ALAP");
    ql::options::set("scheduler_commute", "yes");
    ql::options::set("prescheduler", "yes");

    test_recursion("recursion", "noroutingfirst", "no", "0", "min");
#ifdef  RUNALL
    test_recursion("recursion", "all", "no", std::getenv("mapselectmaxlevel"), "min");
    test_recursion("recursion", "all", "no", std::getenv("mapselectmaxlevel"), "minplusone");
    test_recursion("recursion", "all", "no", std::getenv("mapselectmaxlevel"), "minplushalfmin");
    test_recursion("recursion", "all", "no", std::getenv("mapselectmaxlevel"), "minplusmin");

	test_diogo2("diogo2", "noroutingfirst", "yes", "minextendrc");
    test_diogo2("diogo2", "noroutingfirst", "yes", "maxfidelity");

    test_dot("dot", "no", "ASAP");
    test_dot("dot", "no", "ALAP");
    test_dot("dot", "yes", "ASAP");
    test_dot("dot", "yes", "ALAP");

//  NN:

    test_rc("rc", "noroutingfirst", "no", "0", "min");

    test_someNN("someNN", "noroutingfirst", "no", "0", "min");

//  nonNN but solvable by Initial Placement:
    test_oneD2("oneD2", "noroutingfirst", "no", "0", "min");
    test_oneD4("oneD4", "noroutingfirst", "no", "0", "min");

    test_string("string", "noroutingfirst", "no", "0", "min");
    test_string("string", "all", "no", "0", "min");
    test_string("string", "all", "no", "1", "min");
    test_string("string", "all", "no", "2", "min");
    test_string("string", "all", "no", "3", "min");
    test_string("string", "all", "no", "0", "minplusone");
    test_string("string", "all", "no", "1", "minplusone");
    test_string("string", "all", "no", "2", "minplusone");
    test_string("string", "all", "no", "3", "minplusone");
    test_string("string", "all", "no", "0", "minplushalfmin");
    test_string("string", "all", "no", "1", "minplushalfmin");
    test_string("string", "all", "no", "2", "minplushalfmin");
    test_string("string", "all", "no", "3", "minplushalfmin");
    test_string("string", "all", "no", "0", "minplusmin");
    test_string("string", "all", "no", "1", "minplusmin");
    test_string("string", "all", "no", "2", "minplusmin");
    test_string("string", "all", "no", "3", "minplusmin");
    test_string("string", "all", "yes", "0", "min");
    test_string("string", "all", "yes", "1", "min");
    test_string("string", "all", "yes", "2", "min");
    test_string("string", "all", "yes", "3", "min");
    test_string("string", "all", "yes", "0", "minplusone");
    test_string("string", "all", "yes", "1", "minplusone");
    test_string("string", "all", "yes", "2", "minplusone");
    test_string("string", "all", "yes", "3", "minplusone");
    test_string("string", "all", "yes", "0", "minplushalfmin");
    test_string("string", "all", "yes", "1", "minplushalfmin");
    test_string("string", "all", "yes", "2", "minplushalfmin");
    test_string("string", "all", "yes", "3", "minplushalfmin");
    test_string("string", "all", "yes", "0", "minplusmin");
    test_string("string", "all", "yes", "1", "minplusmin");
    test_string("string", "all", "yes", "2", "minplusmin");
    test_string("string", "all", "yes", "3", "minplusmin");

//  nonNN, still not too large:
    test_maxcut("maxcut", "noroutingfirst", "no", "0", "min");
    test_maxcut("maxcut", "all", "no", "0", "min");
    test_maxcut("maxcut", "all", "no", "1", "min");
    test_maxcut("maxcut", "all", "no", "2", "min");
    test_maxcut("maxcut", "all", "no", "3", "min");
    test_maxcut("maxcut", "all", "no", "0", "minplusone");
    test_maxcut("maxcut", "all", "no", "1", "minplusone");
    test_maxcut("maxcut", "all", "no", "2", "minplusone");
    test_maxcut("maxcut", "all", "no", "3", "minplusone");
    test_maxcut("maxcut", "all", "no", "0", "minplushalfmin");
    test_maxcut("maxcut", "all", "no", "1", "minplushalfmin");
    test_maxcut("maxcut", "all", "no", "2", "minplushalfmin");
    test_maxcut("maxcut", "all", "no", "3", "minplushalfmin");
    test_maxcut("maxcut", "all", "no", "0", "minplusmin");
    test_maxcut("maxcut", "all", "no", "1", "minplusmin");
    test_maxcut("maxcut", "all", "no", "2", "minplusmin");
    test_maxcut("maxcut", "all", "no", "3", "minplusmin");
    test_maxcut("maxcut", "noroutingfirst", "yes", "0", "min");
    test_maxcut("maxcut", "all", "yes", "0", "min");
    test_maxcut("maxcut", "all", "yes", "1", "min");
    test_maxcut("maxcut", "all", "yes", "2", "min");
    test_maxcut("maxcut", "all", "yes", "3", "min");
    test_maxcut("maxcut", "all", "yes", "0", "minplusone");
    test_maxcut("maxcut", "all", "yes", "1", "minplusone");
    test_maxcut("maxcut", "all", "yes", "2", "minplusone");
    test_maxcut("maxcut", "all", "yes", "3", "minplusone");
    test_maxcut("maxcut", "all", "yes", "0", "minplushalfmin");
    test_maxcut("maxcut", "all", "yes", "1", "minplushalfmin");
    test_maxcut("maxcut", "all", "yes", "2", "minplushalfmin");
    test_maxcut("maxcut", "all", "yes", "3", "minplushalfmin");
    test_maxcut("maxcut", "all", "yes", "0", "minplusmin");
    test_maxcut("maxcut", "all", "yes", "1", "minplusmin");
    test_maxcut("maxcut", "all", "yes", "2", "minplusmin");
    test_maxcut("maxcut", "all", "yes", "3", "minplusmin");

    test_allDopt("allDopt", "noroutingfirst", "no", "0", "min");
    test_allDopt("allDopt", "all", "no", "0", "min");
    test_allDopt("allDopt", "all", "no", "1", "min");
    test_allDopt("allDopt", "all", "no", "2", "min");
    test_allDopt("allDopt", "all", "no", "3", "min");
    test_allDopt("allDopt", "all", "no", "0", "minplusone");
    test_allDopt("allDopt", "all", "no", "1", "minplusone");
    test_allDopt("allDopt", "all", "no", "2", "minplusone");
    test_allDopt("allDopt", "all", "no", "3", "minplusone");
    test_allDopt("allDopt", "all", "no", "0", "minplushalfmin");
    test_allDopt("allDopt", "all", "no", "1", "minplushalfmin");
    test_allDopt("allDopt", "all", "no", "2", "minplushalfmin");
    test_allDopt("allDopt", "all", "no", "3", "minplushalfmin");
    test_allDopt("allDopt", "all", "no", "0", "minplusmin");
    test_allDopt("allDopt", "all", "no", "1", "minplusmin");
    test_allDopt("allDopt", "all", "no", "2", "minplusmin");
    test_allDopt("allDopt", "all", "no", "3", "minplusmin");
    test_allDopt("allDopt", "noroutingfirst", "yes", "0", "min");
    test_allDopt("allDopt", "all", "yes", "0", "min");
    test_allDopt("allDopt", "all", "yes", "1", "min");
    test_allDopt("allDopt", "all", "yes", "2", "min");
    test_allDopt("allDopt", "all", "yes", "3", "min");
    test_allDopt("allDopt", "all", "yes", "0", "minplusone");
    test_allDopt("allDopt", "all", "yes", "1", "minplusone");
    test_allDopt("allDopt", "all", "yes", "2", "minplusone");
    test_allDopt("allDopt", "all", "yes", "3", "minplusone");
    test_allDopt("allDopt", "all", "yes", "0", "minplushalfmin");
    test_allDopt("allDopt", "all", "yes", "1", "minplushalfmin");
    test_allDopt("allDopt", "all", "yes", "2", "minplushalfmin");
    test_allDopt("allDopt", "all", "yes", "3", "minplushalfmin");
    test_allDopt("allDopt", "all", "yes", "0", "minplusmin");
    test_allDopt("allDopt", "all", "yes", "1", "minplusmin");
    test_allDopt("allDopt", "all", "yes", "2", "minplusmin");
    test_allDopt("allDopt", "all", "yes", "3", "minplusmin");


    test_allD2("allD2", "all", "no", "2", "min");
    test_allD2("allD2", "all", "no", "3", "min");

    test_allD("allD", "noroutingfirst", "no", "0", "min");
    test_allD("allD", "all", "no", "2", "min");
    test_allD("allD", "all", "no", "3", "min");
    test_allD("allD", "all", "no", "2", "minplusone");
    test_allD("allD", "all", "no", "3", "minplusone");
    test_allD("allD", "all", "no", "2", "minplushalfmin");
    test_allD("allD", "all", "no", "3", "minplushalfmin");
    test_allD("allD", "all", "no", "2", "minplusmin");
    test_allD("allD", "all", "no", "3", "minplusmin");
    test_allD("allD", "all", "no", "0", "min");
    test_allD("allD", "all", "no", "1", "min");
    test_allD("allD", "all", "no", "0", "minplusone");
    test_allD("allD", "all", "no", "1", "minplusone");
    test_allD("allD", "all", "no", "0", "minplushalfmin");
    test_allD("allD", "all", "no", "1", "minplushalfmin");
    test_allD("allD", "all", "no", "0", "minplusmin");
    test_allD("allD", "all", "no", "1", "minplusmin");
    test_allD("allD", "noroutingfirst", "yes", "0", "min");
    test_allD("allD", "all", "yes", "0", "min");
    test_allD("allD", "all", "yes", "1", "min");
    test_allD("allD", "all", "yes", "2", "min");
    test_allD("allD", "all", "yes", "3", "min");
    test_allD("allD", "all", "yes", "0", "minplusone");
    test_allD("allD", "all", "yes", "1", "minplusone");
    test_allD("allD", "all", "yes", "2", "minplusone");
    test_allD("allD", "all", "yes", "3", "minplusone");
    test_allD("allD", "all", "yes", "0", "minplushalfmin");
    test_allD("allD", "all", "yes", "1", "minplushalfmin");
    test_allD("allD", "all", "yes", "2", "minplushalfmin");
    test_allD("allD", "all", "yes", "3", "minplushalfmin");
    test_allD("allD", "all", "yes", "0", "minplusmin");
    test_allD("allD", "all", "yes", "1", "minplusmin");
    test_allD("allD", "all", "yes", "2", "minplusmin");
    test_allD("allD", "all", "yes", "3", "minplusmin");

//  nonNN, realistic:
    test_daniel2("daniel2", "noroutingfirst", "no", "0", "min");
    test_daniel2("daniel2", "all", "no", "0", "min");
    test_daniel2("daniel2", "all", "no", "1", "min");
    test_daniel2("daniel2", "all", "no", "2", "min");
    test_daniel2("daniel2", "all", "no", "3", "min");
    test_daniel2("daniel2", "all", "no", "0", "minplusone");
    test_daniel2("daniel2", "all", "no", "1", "minplusone");
    test_daniel2("daniel2", "all", "no", "2", "minplusone");
    test_daniel2("daniel2", "all", "no", "3", "minplusone");
    test_daniel2("daniel2", "all", "no", "0", "minplushalfmin");
    test_daniel2("daniel2", "all", "no", "1", "minplushalfmin");
    test_daniel2("daniel2", "all", "no", "2", "minplushalfmin");
    test_daniel2("daniel2", "all", "no", "3", "minplushalfmin");
    test_daniel2("daniel2", "all", "no", "0", "minplusmin");
    test_daniel2("daniel2", "all", "no", "1", "minplusmin");
    test_daniel2("daniel2", "all", "no", "2", "minplusmin");
    test_daniel2("daniel2", "all", "no", "3", "minplusmin");
    test_daniel2("daniel2", "noroutingfirst", "yes", "0", "min");
    test_daniel2("daniel2", "all", "yes", "0", "min");
    test_daniel2("daniel2", "all", "yes", "1", "min");
    test_daniel2("daniel2", "all", "yes", "2", "min");
    test_daniel2("daniel2", "all", "yes", "3", "min");
    test_daniel2("daniel2", "all", "yes", "0", "minplusone");
    test_daniel2("daniel2", "all", "yes", "1", "minplusone");
    test_daniel2("daniel2", "all", "yes", "2", "minplusone");
    test_daniel2("daniel2", "all", "yes", "3", "minplusone");
    test_daniel2("daniel2", "all", "yes", "0", "minplushalfmin");
    test_daniel2("daniel2", "all", "yes", "1", "minplushalfmin");
    test_daniel2("daniel2", "all", "yes", "2", "minplushalfmin");
    test_daniel2("daniel2", "all", "yes", "3", "minplushalfmin");
    test_daniel2("daniel2", "all", "yes", "0", "minplusmin");
    test_daniel2("daniel2", "all", "yes", "1", "minplusmin");
    test_daniel2("daniel2", "all", "yes", "2", "minplusmin");
    test_daniel2("daniel2", "all", "yes", "3", "minplusmin");

    test_lingling5esm("lingling5esm", "noroutingfirst", "no", "0", "min");
    test_lingling5esm("lingling5esm", "all", "no", "0", "min");
    test_lingling5esm("lingling5esm", "all", "no", "1", "min");
    test_lingling5esm("lingling5esm", "all", "no", "2", "min");
    test_lingling5esm("lingling5esm", "all", "no", "3", "min");
    test_lingling5esm("lingling5esm", "all", "no", "0", "minplusone");
    test_lingling5esm("lingling5esm", "all", "no", "1", "minplusone");
    test_lingling5esm("lingling5esm", "all", "no", "2", "minplusone");
    test_lingling5esm("lingling5esm", "all", "no", "3", "minplusone");
    test_lingling5esm("lingling5esm", "all", "no", "0", "minplushalfmin");
    test_lingling5esm("lingling5esm", "all", "no", "1", "minplushalfmin");
    test_lingling5esm("lingling5esm", "all", "no", "2", "minplushalfmin");
    test_lingling5esm("lingling5esm", "all", "no", "3", "minplushalfmin");
    test_lingling5esm("lingling5esm", "all", "no", "0", "minplusmin");
    test_lingling5esm("lingling5esm", "all", "no", "1", "minplusmin");
    test_lingling5esm("lingling5esm", "all", "no", "2", "minplusmin");
    test_lingling5esm("lingling5esm", "all", "no", "3", "minplusmin");
    test_lingling5esm("lingling5esm", "noroutingfirst", "yes", "0", "min");
    test_lingling5esm("lingling5esm", "all", "yes", "0", "min");
    test_lingling5esm("lingling5esm", "all", "yes", "1", "min");
    test_lingling5esm("lingling5esm", "all", "yes", "2", "min");
    test_lingling5esm("lingling5esm", "all", "yes", "3", "min");
    test_lingling5esm("lingling5esm", "all", "yes", "0", "minplusone");
    test_lingling5esm("lingling5esm", "all", "yes", "1", "minplusone");
    test_lingling5esm("lingling5esm", "all", "yes", "2", "minplusone");
    test_lingling5esm("lingling5esm", "all", "yes", "3", "minplusone");
    test_lingling5esm("lingling5esm", "all", "yes", "0", "minplushalfmin");
    test_lingling5esm("lingling5esm", "all", "yes", "1", "minplushalfmin");
    test_lingling5esm("lingling5esm", "all", "yes", "2", "minplushalfmin");
    test_lingling5esm("lingling5esm", "all", "yes", "3", "minplushalfmin");
    test_lingling5esm("lingling5esm", "all", "yes", "0", "minplusmin");
    test_lingling5esm("lingling5esm", "all", "yes", "1", "minplusmin");
    test_lingling5esm("lingling5esm", "all", "yes", "2", "minplusmin");
    test_lingling5esm("lingling5esm", "all", "yes", "3", "minplusmin");

    test_lingling7esm("lingling7esm", "noroutingfirst", "no", "0", "min");
    test_lingling7esm("lingling7esm", "all", "no", "0", "min");
    test_lingling7esm("lingling7esm", "all", "no", "1", "min");
    test_lingling7esm("lingling7esm", "all", "no", "2", "min");
    test_lingling7esm("lingling7esm", "all", "no", "3", "min");
    test_lingling7esm("lingling7esm", "all", "no", "0", "minplusone");
    test_lingling7esm("lingling7esm", "all", "no", "1", "minplusone");
    test_lingling7esm("lingling7esm", "all", "no", "2", "minplusone");
    test_lingling7esm("lingling7esm", "all", "no", "3", "minplusone");
    test_lingling7esm("lingling7esm", "all", "no", "0", "minplushalfmin");
    test_lingling7esm("lingling7esm", "all", "no", "1", "minplushalfmin");
    test_lingling7esm("lingling7esm", "all", "no", "2", "minplushalfmin");
    test_lingling7esm("lingling7esm", "all", "no", "3", "minplushalfmin");
    test_lingling7esm("lingling7esm", "all", "no", "0", "minplusmin");
    test_lingling7esm("lingling7esm", "all", "no", "1", "minplusmin");
    test_lingling7esm("lingling7esm", "all", "no", "2", "minplusmin");
    test_lingling7esm("lingling7esm", "all", "no", "3", "minplusmin");
    test_lingling7esm("lingling7esm", "noroutingfirst", "yes", "0", "min");
    test_lingling7esm("lingling7esm", "all", "yes", "0", "min");
    test_lingling7esm("lingling7esm", "all", "yes", "1", "min");
    test_lingling7esm("lingling7esm", "all", "yes", "2", "min");
    test_lingling7esm("lingling7esm", "all", "yes", "3", "min");
    test_lingling7esm("lingling7esm", "all", "yes", "0", "minplusone");
    test_lingling7esm("lingling7esm", "all", "yes", "1", "minplusone");
    test_lingling7esm("lingling7esm", "all", "yes", "2", "minplusone");
    test_lingling7esm("lingling7esm", "all", "yes", "3", "minplusone");
    test_lingling7esm("lingling7esm", "all", "yes", "0", "minplushalfmin");
    test_lingling7esm("lingling7esm", "all", "yes", "1", "minplushalfmin");
    test_lingling7esm("lingling7esm", "all", "yes", "2", "minplushalfmin");
    test_lingling7esm("lingling7esm", "all", "yes", "3", "minplushalfmin");
    test_lingling7esm("lingling7esm", "all", "yes", "0", "minplusmin");
    test_lingling7esm("lingling7esm", "all", "yes", "1", "minplusmin");
    test_lingling7esm("lingling7esm", "all", "yes", "2", "minplusmin");
    test_lingling7esm("lingling7esm", "all", "yes", "3", "minplusmin");

    test_lingling7sub("lingling7sub", "noroutingfirst", "no", "0", "min");
    test_lingling7sub("lingling7sub", "all", "no", "0", "min");
    test_lingling7sub("lingling7sub", "all", "no", "1", "min");
    test_lingling7sub("lingling7sub", "all", "no", "2", "min");
    test_lingling7sub("lingling7sub", "all", "no", "3", "min");
    test_lingling7sub("lingling7sub", "all", "no", "0", "minplusone");
    test_lingling7sub("lingling7sub", "all", "no", "1", "minplusone");
    test_lingling7sub("lingling7sub", "all", "no", "2", "minplusone");
    test_lingling7sub("lingling7sub", "all", "no", "3", "minplusone");
    test_lingling7sub("lingling7sub", "all", "no", "0", "minplushalfmin");
    test_lingling7sub("lingling7sub", "all", "no", "1", "minplushalfmin");
    test_lingling7sub("lingling7sub", "all", "no", "2", "minplushalfmin");
    test_lingling7sub("lingling7sub", "all", "no", "3", "minplushalfmin");
    test_lingling7sub("lingling7sub", "all", "no", "0", "minplusmin");
    test_lingling7sub("lingling7sub", "all", "no", "1", "minplusmin");
    test_lingling7sub("lingling7sub", "all", "no", "2", "minplusmin");
    test_lingling7sub("lingling7sub", "all", "no", "3", "minplusmin");
    test_lingling7sub("lingling7sub", "noroutingfirst", "yes", "0", "min");
    test_lingling7sub("lingling7sub", "all", "yes", "0", "min");
    test_lingling7sub("lingling7sub", "all", "yes", "1", "min");
    test_lingling7sub("lingling7sub", "all", "yes", "2", "min");
    test_lingling7sub("lingling7sub", "all", "yes", "3", "min");
    test_lingling7sub("lingling7sub", "all", "yes", "0", "minplusone");
    test_lingling7sub("lingling7sub", "all", "yes", "1", "minplusone");
    test_lingling7sub("lingling7sub", "all", "yes", "2", "minplusone");
    test_lingling7sub("lingling7sub", "all", "yes", "3", "minplusone");
    test_lingling7sub("lingling7sub", "all", "yes", "0", "minplushalfmin");
    test_lingling7sub("lingling7sub", "all", "yes", "1", "minplushalfmin");
    test_lingling7sub("lingling7sub", "all", "yes", "2", "minplushalfmin");
    test_lingling7sub("lingling7sub", "all", "yes", "3", "minplushalfmin");
    test_lingling7sub("lingling7sub", "all", "yes", "0", "minplusmin");
    test_lingling7sub("lingling7sub", "all", "yes", "1", "minplusmin");
    test_lingling7sub("lingling7sub", "all", "yes", "2", "minplusmin");
    test_lingling7sub("lingling7sub", "all", "yes", "3", "minplusmin");
#endif

    return 0;
}
