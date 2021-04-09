/** \file
 * Implementation for OpenQL's global options.
 */

#include "ql/com/options.h"
#include "ql/utils/logger.h"

namespace ql {
namespace com {
namespace options {

using namespace utils;

/**
 * Makes a new options record for OpenQL.
 */
Options make_ql_options() {
    auto options = Options();

    options.add_enum(
        "log_level",
        "Log levels",
        "LOG_NOTHING",
        {
            "LOG_NOTHING",
            "LOG_CRITICAL",
            "LOG_ERROR",
            "LOG_WARNING",
            "LOG_INFO",
            "LOG_DEBUG"
        }
    ).with_callback([](Option &x){logger::set_log_level(x.as_str());});

    options.add_str(
        "output_dir",
        "Name of the directory in which all output products will be written. "
        "Defaults to \"test_output\" for compatibility reasons. The directory "
        "will automatically be created when the program is compiled.",
        "test_output"
    );

    options.add_bool(
        "unique_output",
        "Uniquify the program name as used for constructing output filenames, "
        "such that compiling the same program multiple times does not overwrite "
        "any previously written output files. When this option is set during the "
        "first construction of a program with a particular name, the program "
        "name is used as-is, and a <program>.unique file is generated in the "
        "output directory to track how many times a program with this name has "
        "been constructed. When a program with the same name is constructed "
        "again later, again with this option set, a numeric suffix will be "
        "automatically added to the program name, starting from 2. The "
        "generated suffix can be reset by simply removing the .unique file."
    );

    options.add_bool("prescheduler", "Run qasm (first) scheduler?", true);
    options.add_bool("scheduler_post179", "Issue 179 solution included", true);
    options.add_bool("print_dot_graphs", "Print (un-)scheduled graphs in DOT format");
    options.add_enum("scheduler", "scheduler type", "ALAP", {"ASAP", "ALAP"});
    options.add_bool("scheduler_uniform", "Do uniform scheduling or not");
    options.add_bool("scheduler_commute", "Commute two-qubit gates when possible, or not");
    options.add_bool("scheduler_commute_rotations", "Commute rotation gates and with two-qubit gates when possible, or not");
    options.add_bool("use_default_gates", "Use default gates or not", "yes");
    options.add_bool("optimize", "optimize or not");
    options.add_bool("clifford_prescheduler", "clifford optimize before prescheduler yes or not");
    options.add_bool("clifford_postscheduler", "clifford optimize after prescheduler yes or not");
    options.add_bool("clifford_premapper", "clifford optimize before mapping yes or not");
    options.add_bool("clifford_postmapper", "clifford optimize after mapping yes or not");
    options.add_enum("decompose_toffoli", "Type of decomposition used for toffoli", "no", {"no", "NC", "AM"});
    options.add_enum("quantumsim", "Produce quantumsim output, and of which kind", "no", {"no", "yes", "qsoverlay"});
    options.add_bool("issue_skip_319", "Issue skip instead of wait in bundles");

    options.add_str ("backend_cc_map_input_file", "Name of CC input map file");
    options.add_bool("backend_cc_verbose", "Add verbose comments to generated .vq1asm file", true);
    options.add_bool("backend_cc_run_once", "Create a .vq1asm program that runs once instead of repeating indefinitely");

    options.add_enum("cz_mode", "CZ mode", "manual", {"manual", "auto"});
    options.add_enum("mapper", "Mapper heuristic", "no", {"no", "base", "baserc", "minextend", "minextendrc", "maxfidelity"});
    options.add_bool("mapinitone2one", "Initialize mapping of virtual qubits one to one to real qubits", true);
    options.add_bool("mapprepinitsstate", "Prep gate leaves qubit in zero state");
    options.add_bool("mapassumezeroinitstate", "Assume that qubits are initialized to zero state");
    options.add_enum("initialplace", "Initialplace qubits before mapping", "no", {"no", "yes", "1s", "10s", "1m", "10m", "1h", "1sx", "10sx", "1mx", "10mx", "1hx"});
    options.add_int ("initialplace2qhorizon", "Initialplace considers only this number of initial two-qubit gates", "0", 0, 100);
    options.add_enum("maplookahead", "Strategy wrt selecting next gate(s) to map", "noroutingfirst", {"no", "1qfirst", "noroutingfirst", "all"});
    options.add_enum("mappathselect", "Which paths: all or borders", "all", {"all", "borders"});
    options.add_enum("mapselectswaps", "Select only one swap, or earliest, or all swaps for one alternative", "all", {"one", "all", "earliest"});
    options.add_bool("maprecNN2q", "Recursing also on NN 2q gate?");
    options.add_int ("mapselectmaxlevel", "Maximum recursion in selecting alternatives on minimum extension", "0", 0, 10, {"inf"});
    options.add_enum("mapselectmaxwidth", "Maximum width number of alternatives to enter recursion with", "min", {"min", "minplusone", "minplushalfmin", "minplusmin", "all"});
    options.add_enum("maptiebreak", "Tie break method", "random", {"first", "last", "random", "critical"});
    options.add_int ("mapusemoves", "Use unused qubit to move thru", "yes", 0, 20, {"no", "yes"});
    options.add_bool("mapreverseswap", "Reverse swap operands when better", true);
    options.add_bool("write_qasm_files", "write (un-)scheduled (with and without resource-constraint) qasm files");
    options.add_bool("write_report_files", "write report files on circuit characteristics and pass results");
    options.add_bool("generate_code", "Generate code for the target; otherwise stop just before doing that", true);

    return options;
}

/**
 * Global options object for all of OpenQL.
 */
Options global = make_ql_options();

/**
 * Convenience function for getting an option value as a string from the global
 * options record.
 */
const Str &get(const Str &key) {
    return global[key].as_str();
}

/**
 * Convenience function for setting an option value for the global options
 * record.
 */
void set(const Str &key, const Str &value) {
    global[key] = value;
}

} // namespace options
} // namespace com
} // namespace ql
