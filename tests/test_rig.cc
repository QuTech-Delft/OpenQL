#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>

#include <time.h>

#include <openql.h>

void
test_maxcut(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 8;
    std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_mapselectswaps=" + param2 + "_mapreverseswap=" + param3;
    std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_mapselectswaps=" + param2 + "_mapreverseswap=" + param3;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_rig.json");
    ql::set_platform(starmon);
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

    k.gate("cz", 1,4);
    k.gate("cz", 1,3);
    k.gate("cz", 3,4);
    k.gate("cz", 3,7);
    k.gate("cz", 4,7);
    k.gate("cz", 6,7);
    k.gate("cz", 5,6);
    k.gate("cz", 1,5);

    prog.add(k);

    ql::options::set("maplookahead", param1);
    ql::options::set("mapselectswaps", param2);
    ql::options::set("mapreverseswap", param3);

    prog.compile( );
}

int main(int argc, char ** argv)
{
    ql::utils::logger::set_log_level("LOG_DEBUG");
    // ql::utils::logger::set_log_level("LOG_NOTHING");
    ql::options::set("write_qasm_files", "yes"); 
    ql::options::set("write_report_files", "yes"); 
    ql::options::set("print_dot_graphs", "no"); 

    ql::options::set("clifford_premapper", "no"); 
    ql::options::set("mapper", "minextendrc"); 
    ql::options::set("mapinitone2one", "yes"); 
//parameter1  ql::options::set("maplookahead", "noroutingfirst");
//parameter2  ql::options::set("mapselectswaps", "all");
    ql::options::set("initialplace", "1m"); 
    ql::options::set("initialplaceprefix", "10"); 
    ql::options::set("mappathselect", "all"); 
    ql::options::set("mapusemoves", "no"); 
//parameter3  ql::options::set("mapreverseswap", "yes"); 
    ql::options::set("maptiebreak", "first"); 

    ql::options::set("clifford_postmapper", "no"); 
    ql::options::set("scheduler_post179", "yes");
    ql::options::set("scheduler", "ALAP");
    ql::options::set("scheduler_commute", "yes");
    ql::options::set("prescheduler", "no");

    test_maxcut("maxcut", "noroutingfirst", "all", "no", "no");

    return 0;
}
