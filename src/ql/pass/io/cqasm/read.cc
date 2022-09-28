/** \file
 * Defines the cQASM reader pass.
 */

#include "ql/pass/io/cqasm/read.h"

#include "ql/utils/filesystem.h"
#include "ql/ir/cqasm/read.h"
#include "ql/pmgr/factory.h"

namespace ql {
namespace pass {
namespace io {
namespace cqasm {
namespace read {

bool ReadCQasmPass::is_pass_registered = pmgr::Factory::register_pass<ReadCQasmPass>("io.cqasm.Read");

/**
 * Dumps docs for the cQASM reader.
 */
void ReadCQasmPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    This pass completely discards the incoming program and replaces it with the
    program described by the given cQASM file.

    The reader supports up to cQASM 1.2. However, rather than supporting the
    default cQASM instruction and function set, the instructions defined in the
    platform JSON description are used. In addition, the following special
    instructions are supported.

     - `skip <int>`: used in conjunction with bundle notation to represent a
       scheduled program. The instruction behaves like `<int>` consecutive empty
       bundles.

     - `wait <int>`: used as an input to the scheduler, forcing all instructions
       defined after the `wait` instruction to start at least `<int>` cycles
       after all instructions defined before the `wait` instruction have
       completed.

     - `wait q[...], <int>`: as above, but only affects instructions that
       operate on the specified qubit. Effectively, this means that the
       instruction enforces that the qubit is idled for at least `<int>` cycles.
       If single-gate-multiple-qubit notation is used, for example
       `wait q[0,2], 3`, the *independent* wait blocks are created as per the
       regular single-gate-multiple-qubit rules.
)" R"(
     - `wait <int>, ...`: generalization of the above, supporting any kind of
       object, and any number of them. That is, all preceding instructions
       operating on any object specified in place of the ellipsis must complete
       before the `wait` can be scheduled, and any following instructions
       operating on any object specified in place of the ellipsis must start
       after the `wait` completes. Unlike the above, if single-gate-multiple-
       qubit notation is used for qubit/bit objects, the result is a *single*
       wait instruction that waits for all indexed elements, rather than
       multiple parallel wait instructions (this is semantically different!).

     - `barrier ...`: shorthand for `wait 0, ...`. A `barrier` without arguments
       is also valid.

     - `measure_all`: if the `measure_all_target` option is set, this is
       automatically expanded to single-qubit measurement instructions of the
       name defined by the value of the option for each qubit in the main qubit
       register.

     - `pragma`: supported as a means to place annotations inside statement
       lists. The reader uses this for some annotations of its own, but
       otherwise ignores it.

    Registers as defined by the platform are implicitly defined by the reader,
    and must thus not be redefined as variables. The only exception is the main
    qubit register (`qubits <int>`), which may optionally be defined at the top
    of the file, as this statement is mandatory in the cQASM 1.0 language.
    Non-scalar registers, such as integer control registers (cregs) and bit
    registers beyond the bits associated with the main qubit register (bregs),
    must be referred to using a function call of the same name as the register
    with the index/indices as arguments (for example `creg(2)`), as cQASM
    doesn't natively support non-scalar objects aside from the main qubit
    register and associated bits.
)" R"(
    Various annotations may be used to fine-tune the behavior of the reader.
    Most of these are particularly important for accurate reproduction of
    OpenQL's internal representation of the program after conversion to and from
    cQASM.

     - `pragma @ql.platform(<json>)` may be placed at the top of the program.
       If the `load_platform` option is not enabled, it is ignored. Otherwise,
       the following forms are supported (these mirror the constructors of the
       Platform class in the API for the most part):

        - not specified or `pragma @ql.platform()`: shorthand for
          `...("none", "none")`.
        - `pragma @ql.platform(name: string)`: shorthand for `...(name, name)`.
        - `pragma @ql.platform(name: string, platform_config: string)`: builds a
          platform with the given name (only used for log messages) and platform
          configuration, the latter of which can be either a recognized platform
          name with or without variant suffix (for example `"cc"` or
          `"cc_light.s7"`), or a path to a JSON configuration filename.
        - `pragma @ql.platform(name: string, platform_config: string, compiler_config: string)`:
          as above, but specifies a custom compiler configuration file in
          addition.
        - `pragma @ql.platform(name: string, platform_config: json)`: instead of
          loading the platform JSON data from a file, it is read from the given
          JSON literal directly.
        - `pragma @ql.platform(platform_config: json)`: shorthand for the above,
          using just `"platform"` for the name.

       Note that the loaded compiler configuration is ignored, because we
       already have one by the time this pass is run! Use the `compile_cqasm()`
       API function to load everything from the cQASM file.

     - `pragma @ql.name("<name>")` may be placed at the top of the program to
       set the name of the program, in case no program exists in the IR yet.
       Otherwise it will simply default to `"program"`.

     - Variables may be annotated with `@ql.type("<name>")` to specify the exact
       OpenQL type that should be used. If not specified, the first type defined
       in the platform that matches the primitive cQASM type will be used. You
       should only need this when you're using a platform that, for instance,
       supports multiple types/sizes of integers.
)" R"(
     - Variables may be annotated with `@ql.temp` to specify that they are
       temporary objects that were automatically inferred. This is normally only
       used by generated cQASM code.

     - The first subcircuit may be annotated with `@ql.entry` if it consists of
       only a single, unconditional goto instruction. In that case, the
       subcircuit will be discarded, and the target of the `goto` instruction
       will be marked as the entry point of the program within OpenQL, rather
       than the first subcircuit.

     - The last subcircuit may be annotated with `@ql.exit` if it contains no
       instructions. In that case, the subcircuit will be discarded, but any
       subcircuits that end in an unconditional goto instruction to this
       subcircuit will be marked as ending the program within OpenQL.

    The `schedule` option controls how scheduling information is interpreted.

     - If set to `keep`, `skip` instructions and bundles are used to determine
       the cycle numbers of the instructions within each block, using the
       following rules:
        - single-gate-multiple-qubit notation (for example `x q[0,1]`) is
          expanded to a bundle of instructions that start simultaneously;
        - the first instruction or bundle of instructions is assigned to start
          in cycle 0;
        - each subsequent single instruction or bundle of instructions starts
          one cycle after the previous instruction/bundle started; and
        - `skip <int>` behaves like `<int>` empty bundles.

     - If set to `discard`, `skip` instructions and bundles are ignored, and
       single-gate-multiple-qubit is ignored in terms of its timing implication;
       instead, instructions will be assigned consecutive cycle numbers in the
       order in which they appear in the file.

     - If set to `bundles-as-barriers`, timing information is ignored as for
       `discard`, but barriers are implicitly inserted before and after each
       bundle, sensitive to exactly those objects used as operands by the
       instructions that appear in the bundle. Single-gate-multiple-qubit
       notation expands to a bundle as well.
    )");
}

/**
 * Returns a user-friendly type name for this pass.
 */
utils::Str ReadCQasmPass::get_friendly_type() const {
    return "cQASM reader";
}

/**
 * Constructs a cQASM reader.
 */
ReadCQasmPass::ReadCQasmPass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::Transformation(pass_factory, instance_name, type_name) {
    options.add_str(
        "cqasm_file",
        "cQASM file to read. Mandatory."
    );
    options.add_enum(
        "schedule",
        "Controls how scheduling/timing information (via bundles and skip "
        "instructions) is interpreted. See pass description for more info.",
        "keep",
        {"keep", "discard", "bundles-as-barriers"}
    );
    options.add_str(
        "measure_all_target",
        "Standard cQASM has a measure_all instruction that implicitly measures "
        "all qubits in a certain way, while OpenQL platforms normally lack this "
        "instruction. When this option is set, it is treated as the name of a "
        "single-qubit measurement gate, that will be used to implement "
        "measure_all; i.e. the measure_all instruction will be expanded to a "
        "bundle of <measure_all_target> instructions, for each qubit in the "
        "main qubit register.",
        ""
    );
    options.add_bool(
        "load_platform",
        "When set, the platform is loaded from the cQASM file by means of a "
        "`pragma @ql.platform(...)` statement at the top of the code. See "
        "pass description for more information.",
        false
    );

}

/**
 * Runs the cQASM reader.
 */
utils::Int ReadCQasmPass::run(
    const ir::Ref &ir,
    const pmgr::pass_types::Context &context
) const {

    ir::cqasm::ReadOptions read_options;

    if (options["schedule"].as_str() == "keep") {
        read_options.schedule_mode = ir::cqasm::ScheduleMode::KEEP;
    } else if (options["schedule"].as_str() == "discard") {
        read_options.schedule_mode = ir::cqasm::ScheduleMode::DISCARD;
    } else if (options["schedule"].as_str() == "bundles-as-barriers") {
        read_options.schedule_mode = ir::cqasm::ScheduleMode::BUNDLES_AS_BARRIERS;
    } else {
        QL_ASSERT(false);
    }

    read_options.measure_all_target = options["measure_all_target"].as_str();
    read_options.load_platform = options["load_platform"].as_bool();

    ir::cqasm::read_file(
        ir,
        options["cqasm_file"].as_str(),
        read_options
    );

    return 0;
}

} // namespace reader
} // namespace cqasm
} // namespace io
} // namespace pass
} // namespace ql
