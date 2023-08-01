#include <openql>

int main(int, char **) {
    ql::initialize();
    ql::utils::logger::set_log_level("LOG_INFO");
    ql::set_option("log_level", "LOG_INFO");
    
    ql::set_option("maptiebreak", "random");
    
    ql::set_option("use_default_gates", "no");
    
    ql::set_option("decompose_toffoli", "no");
    ql::set_option("scheduler", "ASAP");
    ql::set_option("scheduler_heuristic", "random");
    ql::set_option("scheduler_uniform", "no");
    ql::set_option("scheduler_commute", "yes");
    ql::set_option("scheduler_commute_rotations", "yes");
    ql::set_option("prescheduler", "yes");
    ql::set_option("print_dot_graphs", "no");
    
    ql::set_option("clifford_premapper", "yes");
    ql::set_option("clifford_postmapper", "no");
    ql::set_option("mapper", "base");
    ql::set_option("mapassumezeroinitstate", "yes");
    ql::set_option("mapusemoves", "no");
    ql::set_option("mapreverseswap", "yes");
    ql::set_option("mapmaxalters", "10");
    // ql::set_option("mappathselect", "random");
    ql::set_option("maplookahead", "noroutingfirst");
    ql::set_option("maprecNN2q", "no");
    ql::set_option("mapselectmaxlevel", "0");
    ql::set_option("mapselectmaxwidth", "min");
    
    ql::set_option("write_qasm_files", "yes");
    ql::set_option("write_report_files", "yes");

    auto platform  = ql::Platform("mctests", "test_multi_core_64x16_full.json");
    auto num_qubits = 18;
    auto p = ql::Program("qaoa_q1024", platform, num_qubits);
    auto k = ql::Kernel("qaoa_q1024", platform, num_qubits);
    k.gate("cnot", 0, 17);
    p.add_kernel(k);
    p.compile();
}