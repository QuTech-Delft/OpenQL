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
test_cucaroo(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
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

	k.gate("cnot",1,2);
	k.gate("cnot",1,0);
	k.gate("toffoli",{0,2,1});
	k.gate("cnot",1,3);
	k.gate("toffoli",{0,2,1});
	k.gate("cnot",1,0);
	k.gate("cnot",0,2);
	
    prog.add(k);

//VARIOUS OPTIONS

	//Measurement
	// measurement = False

	//Some compiler options
	ql::options::set("output_dir", "mapper=maxfidelity");

	ql::options::set("log_level" , "LOG_INFO"         );
	ql::options::set("scheduler" , "ALAP");
	ql::options::set("mapper" , "maxfidelity");
	ql::options::set("optimize" , "no");
	ql::options::set("scheduler_uniform" , "no");
	ql::options::set("initialplace" , "no");
	ql::options::set("scheduler_post179" , "yes");
	ql::options::set("scheduler_commute" , "yes");
	ql::options::set("mapusemoves" , "no");
	ql::options::set("maptiebreak" , "random");

	//add other options here (overring the options above will not work! change the value in the options above instead!)
	ql::options::set("decompose_toffoli", "no");
	ql::options::set("prescheduler", "yes"); 
	ql::options::set("cz_mode", "manual"); 
	ql::options::set("clifford_premapper", "yes"); // = "yes";
	ql::options::set("clifford_postmapper", "yes"); // = "yes";
	ql::options::set("mapinitone2one", "yes"); // = "yes";
	ql::options::set("mapassumezeroinitstate", "no"); // = "no";
	ql::options::set("initialplace", "no"); // = "no";
	ql::options::set("initialplace2qhorizon", "0"); // = "0";
	ql::options::set("maplookahead", "noroutingfirst"); // = "noroutingfirst";
	ql::options::set("mappathselect", "all"); // = "all";
	ql::options::set("maprecNN2q", "no"); // = "no";
	ql::options::set("mapselectmaxlevel", "0"); // = "0";
	ql::options::set("mapselectmaxwidth", "min"); // = "min";
	ql::options::set("mapselectswaps", "all"); // = "all";
	ql::options::set("mapreverseswap", "yes"); // = "yes";


    prog.compile( );
}

int main(int argc, char ** argv)
{
    ql::options::set("unique_output", "yes");

    ql::options::set("write_qasm_files", "yes"); 
    ql::options::set("write_report_files", "yes"); 
    ql::options::set("print_dot_graphs", "yes"); 

    test_cucaroo("cucaroo", "maxfidelity", "no", "no","no");

    return 0;
}
