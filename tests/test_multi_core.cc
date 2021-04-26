#include <openql>

void test_mc(std::string v, std::string param1, std::string param2, std::string param3, std::string param4) {
    int n = 16;
    // std::string prog_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    std::string prog_name = "test_" + v;
    // std::string kernel_name = "test_" + v + "_maplookahead=" + param1 + "_maprecNN2q=" + param2 + "_mapselectmaxlevel=" + param3 + "_mapselectmaxwidth=" + param4;
    std::string kernel_name = "test_" + v;
    std::vector<double> sweep_points = { 1 };

    auto starmon = ql::Platform("mc4x4full", "test_multi_core_4x4_full.json");
    //ql::set_platform(starmon);
    auto prog = ql::Program(prog_name, starmon, n, 0);
    auto k = ql::Kernel(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points);

    int i, j;

    for (i=0; i<4; i++) {
        k.gate("x", 4*i);
        k.gate("x", 4*i+1);
    }
    for (i=0; i<4; i++) {
        k.gate("cnot", 4*i, 4*i+1);
    }
    for (i=0; i<4; i++) {
        for (j=0; j<4; j++) {
            if (i!=j) k.gate("cnot", 4*i, 4*j);
        }
    }

    prog.add_kernel(k);

    // ql::set_option("maplookahead", param1);
    // ql::set_option("maprecNN2q", param2);
    // ql::set_option("mapselectmaxlevel", param3);
    // ql::set_option("mapselectmaxwidth", param4);

    prog.compile();
}



int main(int argc, char **argv) {
    ql::utils::logger::set_log_level("LOG_DEBUG");
    // ql::utils::logger::set_log_level("LOG_NOTHING");
    ql::set_option("unique_output", "no");

    ql::set_option("write_qasm_files", "yes");
    ql::set_option("write_report_files", "yes");
    // ql::set_option("print_dot_graphs", "yes");
    ql::set_option("use_default_gates", "no");
    ql::set_option("generate_code", "no");

    ql::set_option("clifford_prescheduler", "no");
    ql::set_option("clifford_postscheduler", "no");

    // ql::set_option("clifford_premapper", "yes");
    ql::set_option("mapper", "minextend");
    ql::set_option("mapinitone2one", "yes");
    ql::set_option("mapassumezeroinitstate", "yes");
//parameter1  ql::set_option("maplookahead", "noroutingfirst");
    ql::set_option("mapselectswaps", "all");
    ql::set_option("initialplace", "no");
    ql::set_option("initialplace2qhorizon", "0");
    ql::set_option("mappathselect", "all");
    ql::set_option("mapusemoves", "yes");
    ql::set_option("mapreverseswap", "yes");
//parameter3  ql::set_option("mapselectmaxlevel", "0");
//parameter2  ql::set_option("maprecNN2q", "no");
//parameter4  ql::set_option("mapselectmaxwidth", "min");
    ql::set_option("maptiebreak", "first");

    ql::set_option("clifford_postmapper", "no");
    ql::set_option("scheduler_post179", "yes");
    ql::set_option("scheduler", "ALAP");
    ql::set_option("scheduler_commute", "yes");
    ql::set_option("prescheduler", "yes");

    test_mc("mc", "noroutingfirst", "no", "0", "min");

    return 0;
}
