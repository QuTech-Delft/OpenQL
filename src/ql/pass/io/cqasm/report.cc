/** \file
 * Defines the cQASM writer pass.
 */

#include "ql/pass/io/cqasm/report.h"

#include "ql/utils/filesystem.h"
#include "ql/ir/cqasm/write.h"
#include "ql/pass/ana/statistics/report.h"
#include "ql/pmgr/factory.h"

namespace ql {
namespace pass {
namespace io {
namespace cqasm {
namespace report {

bool ReportCQasmPass::is_pass_registered = pmgr::Factory::register_pass<ReportCQasmPass>("io.cqasm.Report");

/**
 * Dumps docs for the cQASM writer.
 */
void ReportCQasmPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    This pass writes the current program out as a cQASM file, targeting the
    given cQASM version. The writer supports cQASM versions 1.0, 1.1, and 1.2,
    but note that older cQASM versions do not support everything that OpenQL
    supports.

    Several options are provided to control how the cQASM file is written.
    These are necessary because, even within a particular cQASM version, various
    dialects exist, based on instruction set, implicit register definitions,
    function definitions, and so on.

    Regardless of configuration, the written file assumes that the target cQASM
    reader/interpreter supports the instruction- and function set as defined
    (or derived from) the platform JSON description. This means that if you want
    to target a cQASM reader/interpreter that only supports a subset of this
    instruction/function set, or one that supports a different instruction set
    entirely, you will have to ensure that all instructions have been decomposed
    to the instruction set supported by the target prior to printing the cQASM
    file, or write the program such that the unsupported instructions/functions
    aren't used in the first place. It is also possible to embed the platform
    description into the cQASM file in JSON form via a pragma instruction, but
    of course the target cQASM reader/interpreter would then have to support
    that instead.

    The only instructions that the cQASM writer can print that are not part of
    the instruction set as defined in the JSON file are pragmas, barriers, wait,
    and skip instructions, but they can be disabled via options.

     - `pragma` instructions are no-op placeholder instructions with no operands
       that are used to convey metadata via annotations within the context of
       a statement. If the `with_metadata` and `with_platform` options are
       disabled, no pragmas will be printed.

     - `barrier` and `wait` instructions are used for the builtin wait
       instruction. If the `with_barriers` option is disabled, they will not be
       printed. If the option is set to `simple`, the printed syntax and
       semantics are:

        - `wait <int>`: wait for all previous instructions to complete, then
          wait `<int>` cycles, where `<int>` may be zero;
        - `barrier q[...]`: wait for all instructions operating on the qubits
          in the single-gate-multiple-qubit list to complete.

       Note that this syntax only supports barriers acting on qubits, and
       doesn't support wait instructions depending on a subset of objects.
       However, it conforms with the default cQASM 1.0 gateset as of libqasm
       0.3.1 (in 0.3 and before, `barrier` did not exist in the default
       gateset). If the option is instead set to `extended`, the syntax is:

        - `wait <int>`: wait for all previous instructions to complete, then
          wait `<int>` >= 1 cycles;
        - `wait <int>, [...]`: wait for all previous instructions operating on
          the given objects to complete, when wait `<int>` >= 1 cycles;
        - `barrier`: wait for all previous instructions to complete;
        - `barrier [...]`: wait for all previous instructions operating on the
          given objects to complete.

       This encompasses all wait instructions possible within OpenQL's IR.
       OpenQL's cQASM reader supports both notations equally.

     - `skip <int>` instructions are printed in addition to the `{}` multiline
       bundle notation to convey scheduling information: all instructions in a
       bundle start in the same cycle, the subsequent bundle or instruction
       starts in the next cycle (regardless of the duration of the instructions
       in the former bundle), and a `skip <int>` instruction may be used in
       place of `<int>` empty bundles, thus skipping `<int>` cycles. `skip`
       instructions and bundles are not printed when the `with_timing` option
       is disabled.

    None of the supported cQASM versions support non-scalar variables or
    registers, aside from the special-cased main qubit register and
    corresponding bit register. Therefore, some tricks are needed.

     - For non-scalar registers that are expected to be implicitly defined by
       the target cQASM reader/interpreter, references are printed as a function
       call, for example `creg(2)` for the integer control register 2.

     - For non-scalar variables (including registers when
       `registers_as_variables` is set), an independent cQASM variable will be
       printed for every element of the non-scalar OpenQL object, using the name
       format `<name>_<major>_[...]_<minor>`. For example, the `creg(2)` example
       above would be printed as `creg_2` if `registers_as_variables` is set.
       Note that this notation obviously only supports literal indices, and also
       note that name conflicts may arise in contrived cases (for example, when
       a scalar variable named `creg_2` was defined in addition to a
       one-dimensional `creg` variable).

    Indices start from 0 in both cases.
    )");
}

/**
 * Returns a user-friendly type name for this pass.
 */
utils::Str ReportCQasmPass::get_friendly_type() const {
    return "cQASM writer";
}

/**
 * Constructs a cQASM writer.
 */
ReportCQasmPass::ReportCQasmPass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::Analysis(pass_factory, instance_name, type_name) {
    options.add_str(
        "output_suffix",
        "Suffix to use for the output filename.",
        ".cq"
    );
    options.add_enum(
        "cqasm_version",
        "The cQASM version to target.",
        "1.2",
        {"1.0", "1.1", "1.2", "3.0"}
    );
    options.add_bool(
        "with_platform",
        "Whether to include an annotation that includes the (preprocessed) JSON "
        "description of the platform.",
        false
    );
    options.add_bool(
        "registers_as_variables",
        "Whether to include variable declarations for registers. This must be "
        "enabled if the cQASM file is to be passed to a target that doesn't "
        "implicitly define the registers. Note that the size of the main "
        "qubit register is always printed for version 1.0, because it can't "
        "legally be omitted for that version. Also note that this is a lossy "
        "operation if the file is later read by OpenQL again, because register "
        "indices are lost (since only scalar variables are supported by cQASM).",
        false
    );
    options.add_bool(
        "with_statistics",
        "Whether to include the current statistics for each kernel and the "
        "complete program in the generated comments.",
        false
    );
    options.add_bool(
        "with_metadata",
        "Whether to include metadata supported by the IR but not by cQASM as "
        "annotations, to allow the IR to be more accurately reproduced when "
        "read again via the cQASM reader pass.",
        true
    );
    options.add_enum(
        "with_barriers",
        "Whether to include wait and barrier instructions, and if so, using "
        "which syntax (see pass description). These are only needed when the "
        "program will be fed to another compiler later on.",
        "extended",
        {"no", "simple", "extended"}
    );
    options.add_bool(
        "with_timing",
        "Whether to include scheduling/timing information via bundle-and-skip "
        "notation.",
        true
    );
}

/**
 * Runs the cQASM writer.
 */
utils::Int ReportCQasmPass::run(
    const ir::Ref &ir,
    const pmgr::pass_types::Context &context
) const {
    utils::OutFile file{context.output_prefix + options["output_suffix"].as_str()};

    ir::cqasm::WriteOptions write_options;

    // This should probably be parsed rather than be an if-else chain, but
    // code-fast approach.
    if (options["cqasm_version"].as_str() == "1.0") {
        write_options.version = {1, 0};
    } else if (options["cqasm_version"].as_str() == "1.1") {
        write_options.version = {1, 1};
    } else if (options["cqasm_version"].as_str() == "1.2") {
        write_options.version = {1, 2};
    } else if (options["cqasm_version"].as_str() == "3.0") {
        write_options.version = {3, 0};
    } else {
        QL_ASSERT(false);
    }

    write_options.include_platform = options["with_platform"].as_bool();
    write_options.registers_as_variables = options["registers_as_variables"].as_bool();
    write_options.include_statistics = options["with_statistics"].as_bool();
    write_options.include_metadata = options["with_metadata"].as_bool();

    if (options["with_barriers"].as_str() == "no") {
        write_options.include_wait_instructions = ir::cqasm::WaitStyle::DISABLED;
    } else if (options["with_barriers"].as_str() == "simple") {
        write_options.include_wait_instructions = ir::cqasm::WaitStyle::SIMPLE;
    } else if (options["with_barriers"].as_str() == "extended") {
        write_options.include_wait_instructions = ir::cqasm::WaitStyle::EXTENDED;
    } else {
        QL_ASSERT(false);
    }

    write_options.include_timing = options["with_timing"].as_bool();

    ir::cqasm::write(ir, write_options, file.unwrap());

    return 0;
}

} // namespace report
} // namespace cqasm
} // namespace io
} // namespace pass
} // namespace ql
