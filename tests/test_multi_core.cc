#include <openql_i.h>

void
test_mc(std::string v, std::string param1, std::string param2, std::string param3, std::string param4)
{
    int n = 16;
    // std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    std::string prog_name = "test_" + v;
    // std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    std::string kernel_name = "test_" + v;
    double sweep_points[] = { 1 };

    ql::quantum_platform starmon("mc4x4full", "test_multi_core_4x4_full.json");
    //ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(double));

    int i, j;

    for (i=0; i<4; i++)
    {
        k.gate("x", 4*i);
        k.gate("x", 4*i+1);
    }
    for (i=0; i<4; i++)
    {
        k.gate("cnot", 4*i, 4*i+1);
    }
    for (i=0; i<4; i++)
    {
        for (j=0; j<4; j++)
        {
            if (i!=j) k.gate("cnot", 4*i, 4*j);
        }
    }

    prog.add(k);

    // ql::options::set("maplookahead", param1);
    // ql::options::set("maprecNN2q", param2);
    // ql::options::set("mapselectmaxlevel", param3);
    // ql::options::set("mapselectmaxwidth", param4);

    prog.compile( );
}



int main(int argc, char ** argv)
{
    ql::utils::logger::set_log_level("LOG_DEBUG");
    // ql::utils::logger::set_log_level("LOG_NOTHING");
    ql::options::set("unique_output", "no");

    ql::options::set("write_qasm_files", "yes");
    ql::options::set("write_report_files", "yes");
    // ql::options::set("print_dot_graphs", "yes");
    ql::options::set("use_default_gates", "no");

    ql::options::set("clifford_prescheduler", "no");
    ql::options::set("clifford_postscheduler", "no");

    // ql::options::set("clifford_premapper", "yes");
    ql::options::set("mapper", "minextend");
    ql::options::set("mapinitone2one", "yes");
    ql::options::set("mapassumezeroinitstate", "yes");
//parameter1  ql::options::set("maplookahead", "noroutingfirst");
    ql::options::set("mapselectswaps", "all");
    ql::options::set("initialplace", "no");
    ql::options::set("initialplace2qhorizon", "0");
    ql::options::set("mappathselect", "all");
    ql::options::set("mapusemoves", "yes");
    ql::options::set("mapreverseswap", "yes");
//parameter3  ql::options::set("mapselectmaxlevel", "0");
//parameter2  ql::options::set("maprecNN2q", "no");
//parameter4  ql::options::set("mapselectmaxwidth", "min");
    ql::options::set("maptiebreak", "first");

    ql::options::set("clifford_postmapper", "no");
    ql::options::set("scheduler_post179", "yes");
    ql::options::set("scheduler", "ALAP");
    ql::options::set("scheduler_commute", "yes");
    ql::options::set("prescheduler", "yes");

    test_mc("mc", "noroutingfirst", "no", "0", "min");

    return 0;
}
