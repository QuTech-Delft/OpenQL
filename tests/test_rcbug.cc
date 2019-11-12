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
test_rcbug(std::string v, std::string param1, std::string param2)
{
    int n = 10;
    std::string prog_name = "test_" + v + "_mapper=" + param1 + "_mapreverseswap=" + param2;
    std::string kernel_name = "test_" + v + "_mapper=" + param1 + "_mapreverseswap=" + param2;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper17.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    k.gate("x",9);
    k.gate("cnot",2,9);
    k.gate("x",9);
#ifdef  WITHMEASURE
    for (int i = 0; i < n; i++)
    {
        k.gate("measure", i);
    }
#endif

    prog.add(k);

    ql::options::set("clifford_premapper", "no"); 
    ql::options::set("mapinitone2one", "yes"); 
    ql::options::set("maplookahead", "noroutingfirst");
    ql::options::set("mapselectswaps", "all");
    ql::options::set("initialplace", "no"); 
    ql::options::set("initialplace2qhorizon", "10"); 
    ql::options::set("mappathselect", "all"); 
    ql::options::set("mapusemoves", "no"); 
    ql::options::set("mapselectmaxlevel", "0"); 
    ql::options::set("maprecNN2q", "no"); 
    ql::options::set("mapselectmaxwidth", "min"); 
    ql::options::set("maptiebreak", "random"); 

    ql::options::set("clifford_postmapper", "no"); 
    ql::options::set("scheduler_post179", "yes");   // scheduler_post179 == "no" enables buggy code
    ql::options::set("scheduler", "ASAP");          // ALAP would be better
    ql::options::set("scheduler_commute", "no");    // makes no difference with one CNOT in input
    ql::options::set("prescheduler", "yes");

    ql::options::set("mapper", param1);
    ql::options::set("mapreverseswap", param2);

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

    test_rcbug("rcbug", "minextend", "no");
    test_rcbug("rcbug", "minextend", "yes");
    test_rcbug("rcbug", "minextendrc", "no");
    test_rcbug("rcbug", "minextendrc", "yes");

    return 0;
}
