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

    //========================================================================//
    // Logging behavior                                                       //
    //========================================================================//

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

    //========================================================================//
    // Kernel/gate and other global behavior not related to passes            //
    //========================================================================//

    options.add_bool(
        "use_default_gates",
        "Use default gates or not. TODO: document better, and work out what to"
        "do with default gates to begin with.",
        true
    );

    options.add_enum(
        "decompose_toffoli",
        "Controls the behavior of Kernel.toffoli(); either decompose "
        "immediately via the given substitution, or insert the Toffoli gate "
        "into the circuit as-is if \"no\" or unspecified.",
        "no",
        {"no", "NC", "AM"}
    );

    options.add_bool(
        "issue_skip_319",
        "Issue skip instead of wait in bundles. TODO: document better, and "
        "actually fix skip vs. wait/barrier properly once and for all."
    );

    options.add_bool(
        "unique_output",
        "Uniquify the program name as used for constructing output filenames, "
        "such that compiling the same program multiple times yields a different "
        "name each time. When this option is set during the first construction "
        "of a program with a particular name, the program name is used as-is, "
        "and a <program>.unique file is generated in the output directory to "
        "track how many times a program with this name has been constructed. "
        "When a program with the same name is constructed again later, again "
        "with this option set, a numeric suffix will be automatically added to "
        "the program name, starting from 2. The generated suffix can be reset "
        "by simply removing the .unique file. Note that the uniquified name is "
        "only used when %N is used in the \"output_prefix\" common pass option."
    );

    //========================================================================//
    // Default pass order                                                     //
    //========================================================================//

    options.add_bool(
        "prescheduler",
        "When no compiler configuration file is specified, this controls "
        "whether a basic ASAP/ALAP scheduler without resource constraints "
        "should be run before mapping.",
        true
    );

    options.add_bool(
        "clifford_prescheduler",
        "When no compiler configuration file is specified, this controls "
        "whether to run the Clifford optimizer before the prescheduler."
    );

    options.add_bool(
        "clifford_postscheduler",
        "When no compiler configuration file is specified, this controls "
        "whether to run the Clifford optimizer after the prescheduler."
    );

    options.add_bool(
        "clifford_premapper",
        "When no compiler configuration file is specified, this controls "
        "whether to run the Clifford optimizer before the mapper."
    );

    options.add_bool(
        "clifford_postmapper",
        "When no compiler configuration file is specified, this controls "
        "whether to run the Clifford optimizer after the mapper."
    );

    //========================================================================//
    // Behavior for all default-inserted passes                               //
    //========================================================================//

    options.add_str(
        "output_dir",
        "When no compiler configuration file is specified, this controls "
        "the \"output_prefix\" option for all passes; it will be set to "
        "\"<output_dir>/%N_%P\". Defaults to \"test_output\" for  compatibility "
        "reasons. The directory will automatically be created if it does not "
        "already exist when the first output file is written.",
        "test_output"
    );

    options.add_bool(
        "write_qasm_files",
        "When no compiler configuration file is specified, this enables "
        "writing cQASM files before and after each default pass. When a "
        "compiler configuration file is specified, use the \"debug\" pass "
        "option common to all passes instead."
        "write_report_files."
    );

    options.add_bool(
        "write_report_files",
        "When no compiler configuration file is specified, this enables "
        "writing statistics report files before and after each default pass. "
        "When a compiler configuration file is specified, use the \"debug\" "
        "pass option common to all passes instead."
    );

    //========================================================================//
    // Default-inserted scheduler behavior                                    //
    //========================================================================//

    options.add_enum(
        "scheduler",
        "When no compiler configuration file is specified, this controls "
        "whether ALAP or ASAP scheduling is to be used for the default-inserted "
        "scheduler passes. Both the pre-mapping and post-mapping schedulers are "
        "affected.",
        "ALAP",
        {"ASAP", "ALAP"}
    );

    options.add_bool(
        "scheduler_uniform",
        "When no compiler configuration file is specified, this controls "
        "whether uniform scheduling should be done instead of ASAP/ALAP (i.e. "
        "the \"scheduler\" option will be ignored). Both the pre-mapping and "
        "post-mapping schedulers are affected."
    );

    options.add_bool(
        "scheduler_commute",
        "When no compiler configuration file is specified, this controls "
        "whether the default-inserted scheduler passes are allowed to commute "
        "CZ and CNOT gates. This also affects the mapper."
    );

    options.add_bool(
        "scheduler_commute_rotations",
        "When no compiler configuration file is specified, this controls "
        "whether the default-inserted scheduler passes are allowed to commute "
        "single-qubit X and Z rotations. This also affects the mapper."
    );

    options.add_bool(
        "print_dot_graphs",
        "When no compiler configuration file is specified, this controls "
        "whether data dependency/schedule graphs should be written by "
        "default-inserted scheduler passes. The DOT file format is used as "
        "output format."
    );

    //========================================================================//
    // Default-inserted initial/MIP placement pass behavior                   //
    //========================================================================//

    // NOTE: actually still part of the router (i.e. the complete mapper
    // implementation right now)

    options.add_enum(
        "initialplace",
        "When no compiler configuration file is specified, this controls "
        "whether the MIP-based initial placement algorithm should be run "
        "before running the heuristic mapper. A timeout can be specified, as "
        "listed in the allowable values. If the timeout value ends in an 'x', "
        "compilation fails if the timeout is hit; otherwise, heuristic mapping "
        "is performed instead.",
        "no",
        {"no", "yes", "1s", "10s", "1m", "10m", "1h", "1sx", "10sx", "1mx", "10mx", "1hx"}
    );

    options.add_int(
        "initialplace2qhorizon",
        "When no compiler configuration file is specified, this controls "
        "how many two-qubit gates the MIP-based initial placement pass (if any) "
        "considers for each kernel. If 0 or unspecified, all gates are "
        "considered.",
        "0", 0, 100
    );

    //========================================================================//
    // Default-inserted heuristic router pass behavior                        //
    //========================================================================//

    options.add_enum(
        "mapper",
        "When no compiler configuration file is specified, this controls "
        "whether the heuristic mapper will be run, and if so, which heuristic "
        "it should use. When \"no\", MIP-based placement is also disabled.",
        "no",
        {"no", "base", "baserc", "minextend", "minextendrc", "maxfidelity"}
    );

    options.add_bool(
        "mapinitone2one",
        "When no compiler configuration file is specified, and the mapper is "
        "enabled, this controls whether the mapper should assume that each "
        "kernel starts with a one-to-one mapping between virtual and real "
        "qubits. When disabled, the initial mapping is treated as undefined.",
        true
    );

    options.add_bool(
        "mapassumezeroinitstate",
        "When no compiler configuration file is specified, and the mapper is "
        "enabled, this controls whether the mapper should assume that each "
        "qubit starts out as zero at the start of each kernel, rather than "
        "with an undefined state."
    );

    options.add_bool(
        "mapprepinitsstate",
        "When no compiler configuration file is specified, and the mapper is "
        "enabled, this controls whether the mapper may assume that a "
        "user-written prepz gate actually leaves the qubit in the zero state, "
        "rather than any other quantum state. This allows it to make some "
        "optimizations."
    );

    options.add_enum(
        "maplookahead",
        "When no compiler configuration file is specified, and the mapper is "
        "enabled, this controls the lookahead option for the mapper, "
        "controlling the strategy for selecting the next gate(s) to map. "
        "TODO: document better.",
        "noroutingfirst",
        {"no", "1qfirst", "noroutingfirst", "all"}
    );

    options.add_enum(
        "mappathselect",
        "When no compiler configuration file is specified, and the mapper is "
        "enabled, this controls whether to consider all paths from a source "
        "to destination qubit while routing, or to favor routing along the "
        "borders of the chip. The latter is only supported when the qubits "
        "are given coordinates in the topology section of the platform "
        "configuration file.",
        "all",
        {"all", "borders"}
    );

    options.add_enum(
        "mapselectswaps",
        "When no compiler configuration file is specified, and the mapper is "
        "enabled, this controls how routing interacts with speculation. When"
        "\"all\", all swaps for a particular routing option are committed "
        "immediately, before trying anything else. When \"one\", only the "
        "first swap in the route from source to target qubit is committed. When "
        "\"earliest\", the swap that can be done at the earliest point is "
        "selected, which might be the one swapping the source or target qubit.",
        "all",
        {"one", "all", "earliest"}
    );

    options.add_bool(
        "maprecNN2q",
        "When no compiler configuration file is specified, and the mapper is "
        "enabled, this controls the recurse_on_nn_two_qubit option for the "
        "mapper; i.e. whether to \"recurse\" on nearest-neighbor two-qubit gates."
    );

    options.add_int(
        "mapselectmaxlevel",
        "When no compiler configuration file is specified, and the mapper is "
        "enabled, this controls the maximum recursion depth while searching "
        "for alternative mapping solutions.",
        "0",
        0, 10, {"inf"}
    );

    options.add_enum(
        "mapselectmaxwidth",
        "When no compiler configuration file is specified, and the mapper is "
        "enabled, this limits how many alternative mapping solutions are "
        "considered. \"min\" means only the best-scoring alternatives are "
        "considered, \"minplusone\" means the best scoring alternatives plus "
        "one more are considered, \"minplushalfmin\" means 1.5x the number of "
        "best-scoring alternatives are considered, \"minplusmin\" means 2x, "
        "and \"all\" means they are all considered.",
        "min",
        {"min", "minplusone", "minplushalfmin", "minplusmin", "all"}
    );

    options.add_enum(
        "maptiebreak",
        "When no compiler configuration file is specified, and the mapper is "
        "enabled, this controls how to tie-break equally-scoring alternative "
        "mapping solutions. \"first\" and \"last\" choose respectively the "
        "first and last solution in the list (TODO: does this mean anything or "
        "is this essentially random?), \"random\" uses random number generation "
        "to select an alternative, and \"critical\" favors the alternative that "
        "maps the most critical gate as determined by the scheduler (if any).",
        "random",
        {"first", "last", "random", "critical"}
    );

    options.add_int(
        "mapusemoves",
        "When no compiler configuration file is specified, and the mapper is "
        "enabled, this controls if/when the mapper inserts move gates rather "
        "than swap gates to perform routing. If \"no\", swap gates are always "
        "used. Otherwise, a move gate is used if the other qubit has been "
        "initialized, or if initializing it only extends the circuit by the "
        "given number of cycles. \"yes\" implies this limit is 0 cycles.",
        "yes",
        0, 20, {"no", "yes"}
    );

    options.add_bool(
        "mapreverseswap",
        "When no compiler configuration file is specified, and the mapper is "
        "enabled, this controls whether the mapper will reverse the operands "
        "for a swap gate when reversal improves the schedule. This assumes that "
        "the second operand is used earlier than the first operand.",
        true
    );

    //========================================================================//
    // Default-inserted CC code generation pass behavior                      //
    //========================================================================//

    options.add_str(
        "backend_cc_map_input_file",
        "When no compiler configuration file is specified, and the CC backend "
        "pass is inserted automatically, this controls its \"map_input_file\" "
        "option, which specifies the input map filename."
    );

    options.add_bool(
        "backend_cc_verbose",
        "When no compiler configuration file is specified, and the CC backend "
        "pass is inserted automatically, this controls its \"verbose\" option, "
        "which selects whether verbose comments should be added to the "
        "generated .vq1asm file.",
        true
    );

    options.add_bool(
        "backend_cc_run_once",
        "When no compiler configuration file is specified, and the CC backend "
        "pass is inserted automatically, this controls its \"run_once\" option, "
        "which creates a .vq1asm program that runs once instead of repeating "
        "indefinitely."
    );

    //========================================================================//
    // Defunct options                                                        //
    //========================================================================//

    // These options are no longer used anywhere! They're only still here to
    // prevent "unknown option" errors for old programs.

    options.add_enum(
        "quantumsim",
        "Quantumsim output is no longer supported by OpenQL. This option only "
        "exists to not break existing code that sets the option to \"no\".",
        "no", {"no"}
    );

    options.add_enum(
        "cz_mode",
        "This option is no longer used by OpenQL. It's just there to not break "
        "existing code that sets the option.",
        "manual", {"manual", "auto"}
    );

    options.add_bool(
        "scheduler_post179",
        "This option is no longer used by OpenQL. It's just there to not break "
        "existing code that sets the option.",
        true
    );

    options.add_bool(
        "optimize",
        "This option is no longer used by OpenQL. It's just there to not break "
        "existing code that sets the option."
    );

    options.add_bool(
        "generate_code",
        "This option is no longer used by OpenQL. It's just there to not break "
        "existing code that sets the option.",
        true
    );

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
