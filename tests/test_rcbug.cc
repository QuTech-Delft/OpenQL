#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>
#include <cstdlib>

#include <time.h>

#include <openql.h>

#include "metrics.h"


void
test_lingling(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 4;
    std::string prog_name = "test_" + v + "_mapper=" + param1 + "_mapusemoves=" + param2 + "_mapassumezeroinitstate=" + param3 + "_mapprepinitsstate=" + param4;
    std::string kernel_name = "test_" + v + "_mapper=" + param1 + "_mapusemoves=" + param2 + "_mapassumezeroinitstate=" + param3 + "_mapprepinitsstate=" + param4;
    float sweep_points[] = { 1,2 };

    ql::quantum_platform starmon("starmon", "test_mapper17.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, 2);
    // prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

	k.gate("h",2);
	k.gate("cnot",0,1);
	k.gate("cnot",2,3);
	k.gate("h",1);
	k.gate("cnot",1,2);
	k.gate("t",0);
	k.gate("cnot",2,0);
	k.gate("cnot",0,1);

	k.gate("measure",0);
	k.gate("measure",1);
	k.gate("measure",2);
	k.gate("measure",3);
	
    prog.add(k);

	ql::options::set("log_level", "LOG_DEBUG");
	ql::options::set("optimize", "no");

	ql::options::set("scheduler", "ASAP");
	ql::options::set("scheduler_uniform", "no");
	ql::options::set("scheduler_post179", "yes");
	ql::options::set("scheduler_commute", "yes");

    ql::options::set("clifford_premapper", "no");
    ql::options::set("clifford_postmapper", "no");

    ql::options::set("mapassumezeroinitstate", param3);
    ql::options::set("mapprepinitsstate", param4);
	ql::options::set("mapinitone2one", "yes");
	ql::options::set("initialplace", "yes");
	ql::options::set("initialplace2qhorizon", "3");
	ql::options::set("mapreverseswap", "yes");
	ql::options::set("mapusemoves", param2);
	ql::options::set("maptiebreak", "random");
    ql::options::set("mapper", param1);


    prog.compile( );
}

void
test_rcbug_benstein(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 6;
    std::string prog_name = "test_" + v + "_mapper=" + param1 + "_mapusemoves=" + param2 + "_mapassumezeroinitstate=" + param3 + "_mapprepinitsstate=" + param4;
    std::string kernel_name = "test_" + v + "_mapper=" + param1 + "_mapusemoves=" + param2 + "_mapassumezeroinitstate=" + param3 + "_mapprepinitsstate=" + param4;
    float sweep_points[] = { 1,2 };

    ql::quantum_platform starmon("starmon", "test_mapper17.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, 2);
    // prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

	k.gate("prepz",0);
	k.gate("prepz",1);
	k.gate("prepz",2);
	k.gate("prepz",5);
	k.gate("x",1);
	k.gate("h",0);
	k.gate("h",1);
	k.gate("cnot",0,1);
	k.gate("h",0);
	k.gate("h",1);
	k.gate("measure",0);
	k.gate("measure",1);
	
    prog.add(k);

	ql::options::set("log_level", "LOG_DEBUG");
	ql::options::set("optimize", "no");

	ql::options::set("scheduler", "ASAP");
	ql::options::set("scheduler_uniform", "no");
	ql::options::set("scheduler_post179", "yes");
	ql::options::set("scheduler_commute", "yes");

    ql::options::set("clifford_premapper", "no");
    ql::options::set("clifford_postmapper", "no");

    ql::options::set("mapassumezeroinitstate", param3);
    ql::options::set("mapprepinitsstate", param4);
	ql::options::set("mapinitone2one", "yes");
	ql::options::set("initialplace", "10s");
	ql::options::set("initialplace2qhorizon", "10");
	ql::options::set("mapreverseswap", "yes");
	ql::options::set("mapusemoves", param2);
	ql::options::set("maptiebreak", "random");
    ql::options::set("mapper", param1);


    prog.compile( );
}

int main(int argc, char ** argv)
{
    ql::options::set("unique_output", "yes");

    ql::options::set("write_qasm_files", "yes"); 
    ql::options::set("write_report_files", "yes"); 
    ql::options::set("print_dot_graphs", "yes"); 

    test_lingling("lingling", "base", "no", "no","no");

//    test_rcbug_benstein("rcbug_benstein", "minextend", "no", "no","no");
//    test_rcbug_benstein("rcbug_benstein", "minextend", "no", "yes","no");
//    test_rcbug_benstein("rcbug_benstein", "minextend", "no", "no","yes");
//    test_rcbug_benstein("rcbug_benstein", "minextend", "no", "yes","yes");
//    test_rcbug_benstein("rcbug_benstein", "minextend", "yes", "no","yes");
//    test_rcbug_benstein("rcbug_benstein", "minextend", "yes", "yes","yes");
//    test_rcbug_benstein("rcbug_benstein", "minextend", "yes", "no","no");
//    test_rcbug_benstein("rcbug_benstein", "minextend", "yes", "yes","no");
//    test_rcbug_benstein("rcbug_benstein", "minextendrc", "no", "no","no");
//    test_rcbug_benstein("rcbug_benstein", "minextendrc", "no", "yes","no");
//    test_rcbug_benstein("rcbug_benstein", "minextendrc", "no", "no","yes");
//    test_rcbug_benstein("rcbug_benstein", "minextendrc", "no", "yes","yes");
//    test_rcbug_benstein("rcbug_benstein", "minextendrc", "yes", "no","yes");
//    test_rcbug_benstein("rcbug_benstein", "minextendrc", "yes", "yes","yes");
//    test_rcbug_benstein("rcbug_benstein", "minextendrc", "yes", "no","no");
//    test_rcbug_benstein("rcbug_benstein", "minextendrc", "yes", "yes","no");

    return 0;
}
