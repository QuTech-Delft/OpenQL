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
test_qwg(std::string v, std::string mapopt, std::string schedopt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_schedopt=" + schedopt;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_schedopt=" + schedopt;

    ql::quantum_platform starmon("starmon","test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);

    // no dependency, only a conflict in qwg resource
    k.gate("x", 0);
    k.gate("y", 1);

    prog.add(k);
    ql::options::set("mapper", mapopt);
    ql::options::set("scheduler", schedopt);
    prog.compile( );
}

// demo single dimension resource constraint representation simple
void
test_singledim(std::string v, std::string mapopt, std::string schedopt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_schedopt=" + schedopt;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_schedopt=" + schedopt;

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
    ql::options::set("scheduler", schedopt);
    prog.compile( );
}

// test edge resource constraints mapping
void
test_edge(std::string v, std::string mapopt, std::string schedopt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_schedopt=" + schedopt;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_schedopt=" + schedopt;

    ql::quantum_platform starmon("starmon","test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);

    // no dependency, only a conflict in edge resource
    k.gate("cz_v", 1,4);
    k.gate("cz_v", 0,3);

    prog.add(k);
    ql::options::set("mapper", mapopt);
    ql::options::set("scheduler", schedopt);
    prog.compile( );
}

// one cnot with operands that are neighbors in s7
void
test_0(std::string v, std::string mapopt, std::string schedopt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_schedopt=" + schedopt;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_schedopt=" + schedopt;

    ql::quantum_platform starmon("starmon","test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);

    k.gate("x", 0);
    k.gate("x", 2);

    // one cnot that is ok in trivial mapping
    k.gate("cnot_v", 0,2);

    k.gate("x", 0);
    k.gate("x", 2);

    prog.add(k);

    ql::options::set("mapper", mapopt);
    ql::options::set("scheduler", schedopt);
    prog.compile( );
}

// all cnots with operands that are neighbors in s7
void
test_1(std::string v, std::string mapopt, std::string schedopt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_schedopt=" + schedopt;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_schedopt=" + schedopt;

    ql::quantum_platform starmon("starmon","test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);

    for (int j=0; j<7; j++)
        k.gate("x", j);

    // a list of all cnots that are ok in trivial mapping
    k.gate("cnot_v", 0,2);
    k.gate("cnot_v", 0,3);
    k.gate("cnot_v", 1,3);
    k.gate("cnot_v", 1,4);
    k.gate("cnot_v", 2,0);
    k.gate("cnot_v", 2,5);
    k.gate("cnot_v", 3,0);
    k.gate("cnot_v", 3,1);
    k.gate("cnot_v", 3,5);
    k.gate("cnot_v", 3,6);
    k.gate("cnot_v", 4,1);
    k.gate("cnot_v", 4,6);
    k.gate("cnot_v", 5,2);
    k.gate("cnot_v", 5,3);
    k.gate("cnot_v", 6,3);
    k.gate("cnot_v", 6,4);

    for (int j=0; j<7; j++)
        k.gate("x", j);

    prog.add(k);

    ql::options::set("mapper", mapopt);
    ql::options::set("scheduler", schedopt);
    prog.compile( );
}

// one cnot with operands that are at distance 2 in s7
void
test_2(std::string v, std::string mapopt, std::string schedopt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_schedopt=" + schedopt;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_schedopt=" + schedopt;

    ql::quantum_platform starmon("starmon","test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);

    for (int j=0; j<n; j++)
        k.gate("x", j);

    // one cnot, but needs one swap
    k.gate("cnot_v", 2,3);

    for (int j=0; j<n; j++)
        k.gate("x", j);

    prog.add(k);

    ql::options::set("mapper", mapopt);
    ql::options::set("scheduler", schedopt);
    prog.compile( );
}

// one cnot with operands that are at distance 4 in s7
void
test_3(std::string v, std::string mapopt, std::string schedopt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_schedopt=" + schedopt;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_schedopt=" + schedopt;

    ql::quantum_platform starmon("starmon","test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);

    for (int j=0; j<n; j++)
        k.gate("x", j);

    // one cnot, but needs several swaps
    k.gate("cnot_v", 2,4);

    for (int j=0; j<n; j++)
        k.gate("x", j);

    prog.add(k);

    ql::options::set("mapper", mapopt);
    ql::options::set("scheduler", schedopt);
    prog.compile( );
}

// all possible cnots in s7, in lexicographic order
// requires many swaps
void
test_4(std::string v, std::string mapopt, std::string schedopt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_schedopt=" + schedopt;
    std::string kernel_name = "kernel_" + v + "_mapopt=" + mapopt + "_schedopt=" + schedopt;

    ql::quantum_platform starmon("starmon","test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);

    for (int j=0; j<n; j++)
        k.gate("x", j);

    for (int i=0; i<n; i++)
        for (int j=0; j<n; j++)
	    if (i != j)
		k.gate("cnot_v", i,j);

    for (int j=0; j<n; j++)
        k.gate("x", j);

    prog.add(k);

    ql::options::set("mapper", mapopt);
    ql::options::set("scheduler", schedopt);
    prog.compile( );
}

int main(int argc, char ** argv)
{
    ql::utils::logger::set_log_level("LOG_DEBUG");

//    test_singledim("singledim", "minextendrc", "no");

//    test_qwg("qwg", "minextendrc", "no");
//    test_edge("edge", "minextendrc", "no");

//    test_0("0", "base", "ASAP");
//    test_0("0", "minextend", "ASAP");
//    test_0("0", "minextend", "no");
//    test_0("0", "minextendrc", "no");

//    test_1("1", "base", "ASAP");
//    test_1("1", "minextend", "ASAP");
//    test_1("1", "minextend", "no");
//    test_1("1", "minextendrc", "no");

//    test_2("2", "base", "ASAP");
//    test_2("2", "minextend", "ASAP");
//    test_2("2", "minextend", "no");
//    test_2("2", "minextendrc", "no");

//    test_3("3", "base", "ASAP");
//    test_3("3", "minextend", "ASAP");
//    test_3("3", "minextend", "no");
//    test_3("3", "minextendrc", "no");

//    test_4("4", "base", "ASAP");
//    test_4("4", "minextend", "ASAP");

    test_4("4", "base", "no");
    test_4("4", "minextend", "no");
//    test_4("4", "baserc", "no");
    test_4("4", "minextendrc", "no");

    return 0;
}
