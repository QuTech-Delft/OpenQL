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
test_rcbug_benstein(std::string v, std::string param1, std::string param2, std::string param3)
{
    int n = 6;
    std::string prog_name = "test_" + v + "_mapper=" + param1 + "_mapreverseswap=" + param2;
    std::string kernel_name = "test_" + v + "_mapper=" + param1 + "_mapreverseswap=" + param2;
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
	
    prog.add(k);

	std::string new_scheduler="yes";
	std::string scheduler="ASAP";
	std::string uniform_sched= "no";
	std::string sched_commute = "yes";
	// std::string mapper="base";
	std::string moves="yes";
	std::string maptiebreak="first";
	std::string initial_placement="no";
	std::string output_dir_name="test_output";
	std::string optimize="no";
	std::string log_level="LOG_DEBUG";

	ql::options::set("optimize", optimize);
	ql::options::set("scheduler", scheduler);
	ql::options::set("scheduler_uniform", uniform_sched);
	ql::options::set("initialplace", initial_placement);
	ql::options::set("log_level", log_level);
	ql::options::set("scheduler_post179", new_scheduler);
	ql::options::set("scheduler_commute", sched_commute);
	ql::options::set("mapusemoves", moves);
	ql::options::set("maptiebreak", maptiebreak);
    ql::options::set("mapper", param1);
    ql::options::set("clifford_premapper", "no");
    ql::options::set("clifford_postmapper", "no");
    ql::options::set("mapassumezeroinitstate", param3);


    prog.compile( );
}

int main(int argc, char ** argv)
{
    ql::options::set("unique_output", "yes");

    ql::options::set("write_qasm_files", "yes"); 
    ql::options::set("write_report_files", "yes"); 
    ql::options::set("print_dot_graphs", "yes"); 

    test_rcbug_benstein("rcbug_benstein", "minextend", "yes", "no");
    // test_rcbug_benstein("rcbug_benstein", "minextendrc", "yes", "no");


    return 0;
}
