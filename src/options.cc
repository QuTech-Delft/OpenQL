/** \file
 * Option-parsing and storage implementation.
 */

#include "options.h"

#include "utils/exception.h"
#include "utils/logger.h"
#include "utils/filesystem.h"
#include "utils/map.h"
#include "utils/vec.h"
#include <CLI/CLI.hpp>

namespace ql {
namespace options {

using namespace utils;

class Options {
private:
    CLI::App *app;
    Map<Str, Str> opt_name2opt_val;

    void set_defaults() {
        // default values
        opt_name2opt_val.set("log_level") = "LOG_NOTHING";
        opt_name2opt_val.set("output_dir") = "test_output";
        opt_name2opt_val.set("unique_output") = "no";
        opt_name2opt_val.set("write_qasm_files") = "no";
        opt_name2opt_val.set("write_report_files") = "no";

        opt_name2opt_val.set("optimize") = "no";
        opt_name2opt_val.set("use_default_gates") = "yes";
        opt_name2opt_val.set("decompose_toffoli") = "no";
        opt_name2opt_val.set("quantumsim") = "no";
        opt_name2opt_val.set("issue_skip_319") = "no";

        opt_name2opt_val.set("scheduler") = "ALAP";
        opt_name2opt_val.set("scheduler_uniform") = "no";
        opt_name2opt_val.set("scheduler_commute") = "no";
        opt_name2opt_val.set("vary_commutations") = "no";
        opt_name2opt_val.set("prescheduler") = "yes";
        opt_name2opt_val.set("scheduler_post179") = "yes";
        opt_name2opt_val.set("backend_cc_map_input_file") = "";

        opt_name2opt_val.set("cz_mode") = "manual";
        opt_name2opt_val.set("print_dot_graphs") = "no";

        opt_name2opt_val.set("clifford_prescheduler") = "no";
        opt_name2opt_val.set("clifford_postscheduler") = "no";
        opt_name2opt_val.set("clifford_premapper") = "no";
        opt_name2opt_val.set("clifford_postmapper") = "no";

        opt_name2opt_val.set("mapper") = "no";
        opt_name2opt_val.set("mapassumezeroinitstate") = "no";
        opt_name2opt_val.set("mapinitone2one") = "yes";
        opt_name2opt_val.set("mapprepinitsstate") = "no";
        opt_name2opt_val.set("initialplace") = "no";
        opt_name2opt_val.set("initialplace2qhorizon") = "0";
        opt_name2opt_val.set("maplookahead") = "noroutingfirst";
        opt_name2opt_val.set("mappathselect") = "all";
        opt_name2opt_val.set("maprecNN2q") = "no";
        opt_name2opt_val.set("mapselectmaxlevel") = "0";
        opt_name2opt_val.set("mapselectmaxwidth") = "min";
        opt_name2opt_val.set("mapselectswaps") = "all";
        opt_name2opt_val.set("maptiebreak") = "random";
        opt_name2opt_val.set("mapusemoves") = "yes";
        opt_name2opt_val.set("mapreverseswap") = "yes";

        // add options with default values and list of possible values
        app->add_set_ignore_case("--log_level", opt_name2opt_val.at("log_level"),
                                 {"LOG_NOTHING", "LOG_CRITICAL", "LOG_ERROR", "LOG_WARNING", "LOG_INFO", "LOG_DEBUG"}, "Log levels", true);
        app->add_option("--output_dir", opt_name2opt_val.at("output_dir"), "Name of output directory", true);
        app->add_set_ignore_case("--unique_output", opt_name2opt_val.at("unique_output"), {"no", "yes"}, "Make output files unique", true);
        app->add_set_ignore_case("--prescheduler", opt_name2opt_val.at("prescheduler"), {"no", "yes"}, "Run qasm (first) scheduler?", true);
        app->add_set_ignore_case("--scheduler_post179", opt_name2opt_val.at("scheduler_post179"), {"no", "yes"}, "Issue 179 solution included", true);
        app->add_set_ignore_case("--print_dot_graphs", opt_name2opt_val.at("print_dot_graphs"), {"no", "yes"}, "Print (un-)scheduled graphs in DOT format", true);
        app->add_set_ignore_case("--scheduler", opt_name2opt_val.at("scheduler"), {"ASAP", "ALAP"}, "scheduler type", true);
        app->add_set_ignore_case("--scheduler_uniform", opt_name2opt_val.at("scheduler_uniform"), {"yes", "no"}, "Do uniform scheduling or not", true);
        app->add_set_ignore_case("--scheduler_commute", opt_name2opt_val.at("scheduler_commute"), {"yes", "no"}, "Commute gates when possible, or not", true);
        app->add_set_ignore_case("--vary_commutations", opt_name2opt_val.at("vary_commutations"), {"no", "yes"}, "Circuit-wide exploit commutation", true);
        app->add_set_ignore_case("--use_default_gates", opt_name2opt_val.at("use_default_gates"), {"yes", "no"}, "Use default gates or not", true);
        app->add_set_ignore_case("--optimize", opt_name2opt_val.at("optimize"), {"yes", "no"}, "optimize or not", true);
        app->add_set_ignore_case("--clifford_prescheduler", opt_name2opt_val.at("clifford_prescheduler"), {"yes", "no"}, "clifford optimize before prescheduler yes or not", true);
        app->add_set_ignore_case("--clifford_postscheduler", opt_name2opt_val.at("clifford_postscheduler"), {"yes", "no"}, "clifford optimize after prescheduler yes or not", true);
        app->add_set_ignore_case("--clifford_premapper", opt_name2opt_val.at("clifford_premapper"), {"yes", "no"}, "clifford optimize before mapping yes or not", true);
        app->add_set_ignore_case("--clifford_postmapper", opt_name2opt_val.at("clifford_postmapper"), {"yes", "no"}, "clifford optimize after mapping yes or not", true);
        app->add_set_ignore_case("--decompose_toffoli", opt_name2opt_val.at("decompose_toffoli"), {"no", "NC", "AM"}, "Type of decomposition used for toffoli", true);
        app->add_set_ignore_case("--quantumsim", opt_name2opt_val.at("quantumsim"), {"no", "yes", "qsoverlay"}, "Produce quantumsim output, and of which kind", true);
        app->add_set_ignore_case("--issue_skip_319", opt_name2opt_val.at("issue_skip_319"), {"no", "yes"}, "Issue skip instead of wait in bundles", true);
        app->add_option("--backend_cc_map_input_file", opt_name2opt_val.at("backend_cc_map_input_file"), "Name of CC input map file", true);
        app->add_set_ignore_case("--cz_mode", opt_name2opt_val.at("cz_mode"), {"manual", "auto"}, "CZ mode", true);

        app->add_set_ignore_case("--mapper", opt_name2opt_val.at("mapper"), {"no", "base", "baserc", "minextend", "minextendrc", "maxfidelity"}, "Mapper heuristic", true);
        app->add_set_ignore_case("--mapinitone2one", opt_name2opt_val.at("mapinitone2one"), {"no", "yes"}, "Initialize mapping of virtual qubits one to one to real qubits", true);
        app->add_set_ignore_case("--mapprepinitsstate", opt_name2opt_val.at("mapprepinitsstate"), {"no", "yes"}, "Prep gate leaves qubit in zero state", true);
        app->add_set_ignore_case("--mapassumezeroinitstate", opt_name2opt_val.at("mapassumezeroinitstate"), {"no", "yes"}, "Assume that qubits are initialized to zero state", true);
        app->add_set_ignore_case("--initialplace", opt_name2opt_val.at("initialplace"), {"no","yes","1s","10s","1m","10m","1h","1sx","10sx","1mx","10mx","1hx"}, "Initialplace qubits before mapping", true);
        app->add_set_ignore_case("--initialplace2qhorizon", opt_name2opt_val.at("initialplace2qhorizon"), {"0","1","2","3","4","5","6","7","8","9", "10","11","12","13","14","15","16","17","18","19","20","30","40","50","60","70","80","90","100"}, "Initialplace considers only this number of initial two-qubit gates", true);
        app->add_set_ignore_case("--maplookahead", opt_name2opt_val.at("maplookahead"), {"no", "1qfirst", "noroutingfirst", "all"}, "Strategy wrt selecting next gate(s) to map", true);
        app->add_set_ignore_case("--mappathselect", opt_name2opt_val.at("mappathselect"), {"all", "borders"}, "Which paths: all or borders", true);
        app->add_set_ignore_case("--mapselectswaps", opt_name2opt_val.at("mapselectswaps"), {"one", "all", "earliest"}, "Select only one swap, or earliest, or all swaps for one alternative", true);
        app->add_set_ignore_case("--maprecNN2q", opt_name2opt_val.at("maprecNN2q"), {"no","yes"}, "Recursing also on NN 2q gate?", true);
        app->add_set_ignore_case("--mapselectmaxlevel", opt_name2opt_val.at("mapselectmaxlevel"), {"0","1","2","3","4","5","6","7","8","9","10","inf"}, "Maximum recursion in selecting alternatives on minimum extension", true);
        app->add_set_ignore_case("--mapselectmaxwidth", opt_name2opt_val.at("mapselectmaxwidth"), {"min","minplusone","minplushalfmin","minplusmin","all"}, "Maximum width number of alternatives to enter recursion with", true);
        app->add_set_ignore_case("--maptiebreak", opt_name2opt_val.at("maptiebreak"), {"first", "last", "random", "critical"}, "Tie break method", true);
        app->add_set_ignore_case("--mapusemoves", opt_name2opt_val.at("mapusemoves"), {"no", "yes", "0","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16","17","18","19","20"}, "Use unused qubit to move thru", true);
        app->add_set_ignore_case("--mapreverseswap", opt_name2opt_val.at("mapreverseswap"), {"no", "yes"}, "Reverse swap operands when better", true);

        app->add_set_ignore_case("--write_qasm_files", opt_name2opt_val.at("write_qasm_files"), {"yes", "no"}, "write (un-)scheduled (with and without resource-constraint) qasm files", true);
        app->add_set_ignore_case("--write_report_files", opt_name2opt_val.at("write_report_files"), {"yes", "no"}, "write report files on circuit characteristics and pass results", true);
    }

public:
    Options(const Str &app_name = "testApp") {
        app = new CLI::App(app_name);
        set_defaults();
    }

    void print_current_values() {
        std::cout << "log_level: " << opt_name2opt_val.at("log_level") << std::endl
                  << "output_dir: " << opt_name2opt_val.at("output_dir") << std::endl
                  << "unique_output: " << opt_name2opt_val.at("unique_output") << std::endl
                  << "optimize: " << opt_name2opt_val.at("optimize") << std::endl
                  << "use_default_gates: " << opt_name2opt_val.at("use_default_gates") << std::endl
                  << "decompose_toffoli: " << opt_name2opt_val.at("decompose_toffoli") << std::endl
                  << "quantumsim: " << opt_name2opt_val.at("quantumsim") << std::endl
                  << "issue_skip_319: " << opt_name2opt_val.at("issue_skip_319") << std::endl
                  << "clifford_prescheduler: " << opt_name2opt_val.at("clifford_prescheduler") << std::endl
                  << "prescheduler: " << opt_name2opt_val.at("prescheduler") << std::endl
                  << "scheduler: " << opt_name2opt_val.at("scheduler") << std::endl
                  << "scheduler_uniform: " << opt_name2opt_val.at("scheduler_uniform") << std::endl
                  << "clifford_postscheduler: " << opt_name2opt_val.at("clifford_postscheduler") << std::endl
                  << "clifford_premapper: " << opt_name2opt_val.at("clifford_premapper") << std::endl
                  << "mapper: "           << opt_name2opt_val.at("mapper") << std::endl
                  << "mapinitone2one: "   << opt_name2opt_val.at("mapinitone2one") << std::endl
                  << "initialplace: "     << opt_name2opt_val.at("initialplace") << std::endl
                  << "initialplace2qhorizon: "<< opt_name2opt_val.at("initialplace2qhorizon") << std::endl
                  << "maplookahead: "     << opt_name2opt_val.at("maplookahead") << std::endl
                  << "mappathselect: "    << opt_name2opt_val.at("mappathselect") << std::endl
                  << "maptiebreak: "      << opt_name2opt_val.at("maptiebreak") << std::endl
                  << "mapusemoves: "      << opt_name2opt_val.at("mapusemoves") << std::endl
                  << "mapreverseswap: "   << opt_name2opt_val.at("mapreverseswap") << std::endl
                  << "mapselectswaps: "   << opt_name2opt_val.at("mapselectswaps") << std::endl
                  << "clifford_postmapper: " << opt_name2opt_val.at("clifford_postmapper") << std::endl
                  << "scheduler_post179: " << opt_name2opt_val.at("scheduler_post179") << std::endl
                  << "scheduler_commute: " << opt_name2opt_val.at("scheduler_commute") << std::endl
                  << "vary_commutations: " << opt_name2opt_val.at("vary_commutations") << std::endl
                  << "cz_mode: " << opt_name2opt_val.at("cz_mode") << std::endl
                  << "write_qasm_files: " << opt_name2opt_val.at("write_qasm_files") << std::endl
                  << "write_report_files: " << opt_name2opt_val.at("write_report_files") << std::endl
                  << "print_dot_graphs: " << opt_name2opt_val.at("print_dot_graphs") << std::endl;
        // FIXME: incomplete, function seems unused
    }

    void reset_options() {
        app = new CLI::App("testApp");
        set_defaults();
    }

    void help() {
        std::cout << app->help() << std::endl;
    }

    void set(const Str &opt_name, const Str &opt_value) {
        try {
            std::vector<Str> opts = {opt_value, "--"+opt_name};
            app->parse(opts);
        } catch (const std::exception &e) {
            app->reset();
            QL_EOUT("Un-known option:" << e.what());
            throw Exception("Error parsing options. " + Str(e.what()) + " !", false);
        }
        app->reset();
    }

    Str get(const Str &opt_name) {
        Str opt_value("UNKNOWN");
        if (opt_name2opt_val.find(opt_name) != opt_name2opt_val.end()) {
            opt_value = opt_name2opt_val.at(opt_name);
        } else {
            QL_EOUT("Un-known option:" << opt_name);
        }
        return opt_value;
    }

};

QL_GLOBAL Options ql_options("OpenQL Options");

void print() {
    ql_options.help();
}

void print_current_values() {
    ql_options.print_current_values();
}

void set(const Str &opt_name, const Str &opt_value) {
    ql_options.set(opt_name, opt_value);

    if (opt_name == "log_level") {
        logger::set_log_level(opt_value);
    } else if (opt_name == "output_dir") {
        make_dirs(opt_value);
    }
}

Str get(const Str &opt_name) {
    return ql_options.get(opt_name);
}

void reset_options() {
    ql_options.reset_options();
}

} // namespace options
} // namespace ql
