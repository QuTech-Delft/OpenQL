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
test_qwg(std::string v, std::string mapopt, std::string mapdecomposeropt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_mapdec=" + mapdecomposeropt;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_mapdec=" + mapdecomposeropt;

    ql::quantum_platform starmon("starmon","test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);

    // no dependency, only a conflict in qwg resource
    k.gate("x", 0);
    k.gate("y", 1);

    prog.add(k);
    ql::options::set("mapper", mapopt);
    ql::options::set("mapdecomposer", mapdecomposeropt);
    prog.compile( );
}

// demo single dimension resource constraint representation simple
void
test_singledim(std::string v, std::string mapopt, std::string mapdecomposeropt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_mapdec=" + mapdecomposeropt;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_mapdec=" + mapdecomposeropt;

    ql::quantum_platform starmon("starmon","test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);

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
    ql::options::set("mapdecomposer", mapdecomposeropt);
    prog.compile( );
}

// test edge resource constraints mapping
void
test_edge(std::string v, std::string mapopt, std::string mapdecomposeropt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_mapdec=" + mapdecomposeropt;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_mapdec=" + mapdecomposeropt;

    ql::quantum_platform starmon("starmon","test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);

    // no dependency, only a conflict in edge resource
    k.gate("cz_virt", 1,4);
    k.gate("cz_virt", 0,3);

    prog.add(k);
    ql::options::set("mapper", mapopt);
    ql::options::set("mapdecomposer", mapdecomposeropt);
    prog.compile( );
}

// test detuned_qubits resource constraints mapping
// no swaps generated
void
test_detuned(std::string v, std::string mapopt, std::string mapdecomposeropt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_mapdec=" + mapdecomposeropt;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_mapdec=" + mapdecomposeropt;

    ql::quantum_platform starmon("starmon","test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);

    // preferably cz's parallel, but not with x 3
    k.gate("cz_virt", 0,2);
    k.gate("cz_virt", 1,4);
    k.gate("x", 3);

    // likewise, while y 3, no cz on 0,2 or 1,4
    k.gate("y", 3);
    k.gate("cz_virt", 0,2);
    k.gate("cz_virt", 1,4);

    prog.add(k);
    ql::options::set("mapper", mapopt);
    ql::options::set("mapdecomposer", mapdecomposeropt);
    prog.compile( );
}

// one cnot with operands that are neighbors in s7
void
test_0(std::string v, std::string mapopt, std::string mapdecomposeropt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_mapdec=" + mapdecomposeropt;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_mapdec=" + mapdecomposeropt;

    ql::quantum_platform starmon("starmon","test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);

    k.gate("x", 0);
    k.gate("x", 2);

    // one cnot that is ok in trivial mapping
    k.gate("cnot_virt", 0,2);

    k.gate("x", 0);
    k.gate("x", 2);

    prog.add(k);

    ql::options::set("mapper", mapopt);
    ql::options::set("mapdecomposer", mapdecomposeropt);
    prog.compile( );
}

// all cnots with operands that are neighbors in s7
void
test_1(std::string v, std::string mapopt, std::string mapdecomposeropt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_mapdec=" + mapdecomposeropt;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_mapdec=" + mapdecomposeropt;

    ql::quantum_platform starmon("starmon","test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);

    for (int j=0; j<7; j++) { k.gate("x", j); }

    // a list of all cnots that are ok in trivial mapping
    k.gate("cnot_virt", 0,2);
    k.gate("cnot_virt", 0,3);
    k.gate("cnot_virt", 1,3);
    k.gate("cnot_virt", 1,4);
    k.gate("cnot_virt", 2,0);
    k.gate("cnot_virt", 2,5);
    k.gate("cnot_virt", 3,0);
    k.gate("cnot_virt", 3,1);
    k.gate("cnot_virt", 3,5);
    k.gate("cnot_virt", 3,6);
    k.gate("cnot_virt", 4,1);
    k.gate("cnot_virt", 4,6);
    k.gate("cnot_virt", 5,2);
    k.gate("cnot_virt", 5,3);
    k.gate("cnot_virt", 6,3);
    k.gate("cnot_virt", 6,4);

    for (int j=0; j<7; j++) { k.gate("x", j); }

    prog.add(k);

    ql::options::set("mapper", mapopt);
    ql::options::set("mapdecomposer", mapdecomposeropt);
    prog.compile( );
}

// one cnot with operands that are at distance 2 in s7
void
test_2(std::string v, std::string mapopt, std::string mapdecomposeropt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_mapdec=" + mapdecomposeropt;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_mapdec=" + mapdecomposeropt;

    ql::quantum_platform starmon("starmon","test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);

    k.gate("x", 2);
    k.gate("x", 3);

    // one cnot, but needs one swap
    k.gate("cnot_virt", 2,3);

    k.gate("x", 2);
    k.gate("x", 3);

    prog.add(k);

    ql::options::set("mapper", mapopt);
    ql::options::set("mapdecomposer", mapdecomposeropt);
    prog.compile( );
}

// one cnot with operands that are at distance 4 in s7
void
test_3(std::string v, std::string mapopt, std::string mapdecomposeropt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_mapdec=" + mapdecomposeropt;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_mapdec=" + mapdecomposeropt;

    ql::quantum_platform starmon("starmon","test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);

    k.gate("x", 2);
    k.gate("x", 4);

    // one cnot, but needs several swaps
    k.gate("cnot_virt", 2,4);

    k.gate("x", 2);
    k.gate("x", 4);

    prog.add(k);

    ql::options::set("mapper", mapopt);
    ql::options::set("mapdecomposer", mapdecomposeropt);
    prog.compile( );
}

// all possible cnots in s7, in lexicographic order
// requires many swaps
void
test_4(std::string v, std::string mapopt, std::string mapdecomposeropt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_mapdec=" + mapdecomposeropt;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_mapdec=" + mapdecomposeropt;

    ql::quantum_platform starmon("starmon","test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);

    for (int j=0; j<n; j++) { k.gate("x", j); }

    for (int i=0; i<n; i++) { for (int j=0; j<n; j++) { if (i != j) { k.gate("cnot_virt", i,j); } } }

    for (int j=0; j<n; j++) { k.gate("x", j); }

    prog.add(k);

    ql::options::set("mapper", mapopt);
    ql::options::set("mapdecomposer", mapdecomposeropt);
    prog.compile( );
}

// all possible cnots in s7, avoiding collisions:
// - pairs in both directions together
// - from low distance to high distance
// - each time as much as possible in opposite sides of the circuit
void
test_5(std::string v, std::string mapopt, std::string mapdecomposeropt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_mapdec=" + mapdecomposeropt;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_mapdec=" + mapdecomposeropt;

    ql::quantum_platform starmon("starmon","test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);

    for (int j=0; j<n; j++) { k.gate("x", j); }

	k.gate("cnot_virt", 0,3);
	k.gate("cnot_virt", 3,0);

	k.gate("cnot_virt", 6,4);
	k.gate("cnot_virt", 4,6);

	k.gate("cnot_virt", 3,1);
	k.gate("cnot_virt", 1,3);

	k.gate("cnot_virt", 5,2);
	k.gate("cnot_virt", 2,5);

	k.gate("cnot_virt", 1,4);
	k.gate("cnot_virt", 4,1);

	k.gate("cnot_virt", 3,5);
	k.gate("cnot_virt", 5,3);

	k.gate("cnot_virt", 6,3);
	k.gate("cnot_virt", 3,6);

	k.gate("cnot_virt", 2,0);
	k.gate("cnot_virt", 0,2);

	k.gate("cnot_virt", 0,1);
	k.gate("cnot_virt", 1,0);

	k.gate("cnot_virt", 3,4);
	k.gate("cnot_virt", 4,3);

	k.gate("cnot_virt", 1,6);
	k.gate("cnot_virt", 6,1);

	k.gate("cnot_virt", 6,5);
	k.gate("cnot_virt", 5,6);

	k.gate("cnot_virt", 3,2);
	k.gate("cnot_virt", 2,3);

	k.gate("cnot_virt", 5,0);
	k.gate("cnot_virt", 0,5);

	k.gate("cnot_virt", 0,6);
	k.gate("cnot_virt", 6,0);

	k.gate("cnot_virt", 1,5);
	k.gate("cnot_virt", 5,1);

	k.gate("cnot_virt", 0,4);
	k.gate("cnot_virt", 4,0);

	k.gate("cnot_virt", 6,2);
	k.gate("cnot_virt", 2,6);

	k.gate("cnot_virt", 2,1);
	k.gate("cnot_virt", 1,2);

	k.gate("cnot_virt", 5,4);
	k.gate("cnot_virt", 4,5);

	k.gate("cnot_virt", 2,4);
	k.gate("cnot_virt", 4,2);

    for (int j=0; j<n; j++) { k.gate("x", j); }

    prog.add(k);

    ql::options::set("mapper", mapopt);
    ql::options::set("mapdecomposer", mapdecomposeropt);
    prog.compile( );
}

int main(int argc, char ** argv)
{
    ql::utils::logger::set_log_level("LOG_DEBUG");

    ql::options::set("scheduler", "no");        // still run rc cc_light scheduler afterwards!
    ql::options::set("mapinitialplace", "yes");  // testing initial placement

//    test_singledim("singledim", "minextendrc", "yes");

//    test_qwg("qwg", "minextendrc", "yes");
//    test_edge("edge", "minextendrc", "yes");
//    test_detuned("detuned", "minextendrc", "yes");

//    test_0("0", "base", "yes");
//    test_0("0", "minextend", "yes");
//    test_0("0", "minextendrc", "yes");

//    test_1("1", "base", "yes");
//    test_1("1", "minextend", "yes");
//    test_1("1", "minextendrc", "yes");

    test_2("2", "base", "yes");
//    test_2("2", "minextend", "yes");
//    test_2("2", "minextendrc", "yes");

//    test_3("3", "base", "yes");
//    test_3("3", "minextend", "yes");

//    test_3("3", "base", "yes");
//    test_3("3", "minextendrc", "yes");

//    test_4("4", "base", "yes");
//    test_4("4", "minextend", "yes");
//    test_4("4", "minextendrc", "yes");

//    test_5("5", "base", "yes");
//    test_5("5", "minextend", "yes");
//    test_5("5", "minextendrc", "yes");

    return 0;
}
