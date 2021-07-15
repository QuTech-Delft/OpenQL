/** \file
 * Provides the conversion from the old IR (still used for the API for backward
 * compatibility) to the new one.
 */

#pragma once

#include "ql/ir/compat/compat.h"
#include "ql/ir/ir.h"

namespace ql {
namespace ir {

/**
 * Annotation placed on the Program node to indicate how many qubits, cregs, and
 * bregs were used in the original program. This information would otherwise be
 * lost.
 */
struct ObjectUsage {

    /**
     * The number of qubits used by the program.
     */
    utils::UInt num_qubits;

    /**
     * The number of cregs used by the program.
     */
    utils::UInt num_cregs;

    /**
     * The number of bregs used by the program.
     */
    utils::UInt num_bregs;

};

/**
 * Annotation placed on SubBlock nodes with the original name of the kernel.
 */
struct KernelName {

    /**
     * The original name of the kernel.
     */
    utils::Str name;

};

/**
 * Annotation placed on BlockBase nodes to indicate cycle validity of the
 * original kernel.
 */
struct KernelCyclesValid {

    /**
     * Whether the cycle numbers were valid when the kernel was converted to a
     * block. Note that cycle numbers in the new IR must always be valid; this
     * is only used by the conversion back to the old IR to invalidate them
     * again for compatibility purposes.
     */
    utils::Bool valid;

};

/**
 * Annotation placed on InstructionType nodes to indicate that the prototype has
 * been inferred, and thus that we need to be lenient if we encounter gates that
 * don't match it.
 */
struct PrototypeInferred {};

/**
 * Converts the old platform to the new IR structure.
 *
 * See convert_old_to_new(const compat::ProgramRef&) for details.
 */
Ref convert_old_to_new(const compat::PlatformRef &old);

/**
 * Converts the old IR (program and platform) to the new one.
 *
 * The old IR has a somewhat different and in some cases lesser feature set.
 * Therefore, some conversion rules are needed.
 *
 * Data types didn't really exist in the old IR. Therefore, the following are
 * generated:
 *
 *  - `qubit`, the main qubit data type;
 *  - `bit`, the type used for bregs and conditions;
 *  - `int` (signed, 32-bit), the type used for cregs and indices; and
 *  - `real`, the type used for gate angles.
 *
 * Objects are converted as per the following rules.
 *
 *  - All qubits in the old IR are mapped to the main qubit register (named `q`)
 *    in the new one. Qubit variables are not used at all.
 *  - Bit references from the old IR are mapped as follows:
 *     - the first num_qubits bits are mapped to the implicit bit registers of
 *       the main qubit register; and
 *     - any additional bits are mapped to a bit vector register, named `breg`.
 *  - Cregs are mapped to an int32 vector register, named `creg`.
 *
 * The old IR does not support instruction prototypes; any instruction could
 * have any set of parameter types, and heuristics/special cases were applied
 * all over the place to make that work sufficiently well. Conversely, the
 * new IR needs a separate instruction overload in the instruction set for
 * an instruction to even exist within the program. Therefore, this information
 * either needs to be added to the platform configuration file using a new key,
 * or it needs to be inferred for backward compatibility. Explicit specification
 * can be done by adding a "parameters" key to the instruction definition, which
 * must then be an array of strings. The strings must either match a type name
 * exactly, or be of the form `<type>:<mode>`, where `<mode>` must then be one
 * of the following:
 *
 *  - `W`: the operand is written (classical) or otherwise operated on in a way
 *    that does not commute (qubit);
 *  - `R`: the operand is read (classical only);
 *  - `L`: the operand is read and must be known at compile-time;
 *  - `X`: the (qubit) operand behaves like an X rotation, commuting with other
 *    gates that use the qubit in X mode;
 *  - `Y` and `Z`: same as `X`, but for the other axes;
 *  - `M`: the (qubit) operand is measured, with the result of the measurement
 *    going to the implicit bit register associated with the qubit (this is
 *    identical to using both the qubit and the implicit bit in `W` mode using
 *    a single operand).
 *
 * If the "parameters" key does not exist, the following rules are applied (if
 * multiple name-based rules match, the first is used):
 *
 *  - if the name matches `h|i|move_init|prep_?[xyz]`, the parameters default
 *    to `qubit:W`;
 *  - if the name equals `rx`, the parameters default to `qubit:X, real:L`;
 *  - if the name matches `(m|mr|r)?xm?[0-9]*`, the parameters default to
 *    `qubit:X`;
 *  - if the name equals `ry`, the parameters default to `qubit:Y, real:L`;
 *  - if the name matches `(m|mr|r)?ym?[0-9]*`, the parameters default to
 *    `qubit:Y`;
 *  - if the name equals `rz`, the parameters default to `qubit:Z, real:L`;
 *  - if the name equals `crz` or `cr`, the parameters default to
 *    `qubit:Z, qubit:Z, real:L`;
 *  - if the name equals `crk`, the parameters default to
 *    `qubit:Z, qubit:Z, int:L`;
 *  - if the name matches `[st](dag)?|(m|mr|r)?zm?[0-9]*`, the parameters
 *    default to `qubit:Z`;
 *  - if the name matches `meas(ure)?(_?[xyz])?(_keep)?`, two overloads are
 *    generated, namely `qubit:M` for using an implicit bit result register and
 *    `qubit:W, bit:W` for making the return register explicit;
 *  - if the name matches `(teleport)?(move|swap)`, the parameters default to
 *    `qubit:W, qubit:W`;
 *  - if the name equals `cnot` or `cx`, the parameters default to
 *    `qubit:Z, qubit:X`;
 *  - if the name equals `cz` or `cphase`, the parameters default to
 *    `qubit:Z, qubit:Z`;
 *  - if the name equals `cz_park`, the parameters default to
 *    `qubit:Z, qubit:Z, qubit:W` (i.e. only a single parking qubit is assumed);
 *  - if the name equals `toffoli`, the parameters default to
 *    `qubit:Z, qubit:Z, qubit:X`;
 *  - if the instruction name is not matched by any of the above, it is assumed
 *    to have no operands;
 *  - if the instruction is specialized, and the above heuristics result in too
 *    few or differently-typed operands compared to the template/specialization
 *    parameters, the above is thrown out and substituted with a `qubit:W` for
 *    each template parameter;
 *  - if, at any time during the conversion process (either within the
 *    decomposition rules or within the program), an instruction instance is
 *    scanned for which no overload exists, an overload is inferred for it,
 *    using `W` mode for all operands, but note that this will fail for
 *    instructions using literal operands.
 *
 * Once the instruction set has been processed, the legacy decomposition rules
 * (`gate_decomposition` section) are scanned. While the old platform didn't
 * really support this, it's possible to specify an instruction both in the
 * `instructions` and `gate_decomposition` sections: the instruction metadata
 * will then be taken from the instruction section. If an instruction is
 * implicitly defined by the `gate_decomposition` section, as was the norm, an
 * instruction type is inferred for it, using the following rules:
 *
 *  - a `qubit:W` operand is added for each `q<i>` `%<i>` operand in the "name"
 *    of the decomposition;
 *  - the duration is trivially and pessimistically inferred as the sum of the
 *    durations of the instructions it decomposes into.
 *
 * Instruction types in the new IR make a distinction between cQASM and
 * OpenQL-internal gate names, because these tend to use different naming
 * conventions. If an instruction definition has a `cqasm_name` key, its value
 * will be used as this name. In all other cases, the instruction name will be
 * the same in cQASM and OpenQL.
 *
 * Instruction durations are specified in cycles in the new IR. Any duration
 * that is not a multiple of the cycle time will be rounded up. If no duration
 * is specified, a single cycle is assumed. Note that a duration of 0 is legal
 * in the new IR (used for classical instructions, since the duration refers to
 * quantum cycles), but because it isn't the default, this has to be explicitly
 * specified when needed.
 *
 * Default instructions (i.e. instructions not explicitly defined in the
 * platform) no longer exist, with the exception of classical assignments (`set`
 * instructions), barriers (`wait` and `barrier`), the special source and sink
 * nodes for the scheduler, and (new) `goto` instructions. The following
 * conversion behavior is used to accommodate (the first matching rule is used):
 *
 *  - if the gate is named `wait` or `barrier`, or has type `WAIT`, a wait
 *    instruction is used, using the instruction's duration rounded up to the
 *    nearest cycle, waiting on all qubits, cregs, and bregs referred to;
 *  - if the gate name is `SOURCE`, a source instruction is used;
 *  - if the gate name is `SINK`, a sink instruction is used;
 *  - if the gate type is CLASSICAL and the name is `add`, a set instruction
 *    is emitted using the `operator+(int, int) -> int` function on the RHS;
 *  - if the gate type is CLASSICAL and the name is `sub`, a set instruction
 *    is emitted using the `operator-(int, int) -> int` function on the RHS;
 *  - if the gate type is CLASSICAL and the name is `and`, a set instruction
 *    is emitted using the `operator&(int, int) -> int` function on the RHS;
 *  - if the gate type is CLASSICAL and the name is `or`, a set instruction
 *    is emitted using the `operator|(int, int) -> int` function on the RHS;
 *  - if the gate type is CLASSICAL and the name is `xor`, a set instruction
 *    is emitted using the `operator^(int, int) -> int` function on the RHS;
 *  - if the gate type is CLASSICAL and the name is `eq`, a set instruction
 *    is emitted using the `operator==(int, int) -> bool` function wrapped
 *    inside a `int(bool) -> int` function on the RHS;
 *  - if the gate type is CLASSICAL and the name is `ne`, a set instruction
 *    is emitted using the `operator!=(int, int) -> bool` function wrapped
 *    inside a `int(bool) -> int` function on the RHS;
 *  - if the gate type is CLASSICAL and the name is `lt`, a set instruction
 *    is emitted using the `operator<(int, int) -> bool` function wrapped
 *    inside a `int(bool) -> int` function on the RHS;
 *  - if the gate type is CLASSICAL and the name is `gt`, a set instruction
 *    is emitted using the `operator>(int, int) -> bool` function wrapped
 *    inside a `int(bool) -> int` function on the RHS;
 *  - if the gate type is CLASSICAL and the name is `le`, a set instruction
 *    is emitted using the `operator<=(int, int) -> bool` function wrapped
 *    inside a `int(bool) -> int` function on the RHS;
 *  - if the gate type is CLASSICAL and the name is `ge`, a set instruction
 *    is emitted using the `operator>=(int, int) -> bool` function wrapped
 *    inside a `int(bool) -> int` function on the RHS;
 *  - if the gate type is CLASSICAL and the name is `ldi` or `mov`, a set
 *    instruction is emitted using the operands directly;
 *  - if the gate type is CLASSICAL and the name is `not`, a set instruction
 *    is emitted using the `operator~(int) -> int` function on the RHS;
 *  - if an instruction type already exists for the gate name and the operand
 *    list deduced as follows, it is used:
 *     - any qubit operands;
 *     - any creg operands;
 *     - any breg operands;
 *     - the angle operand, if the gate is named `rx`, `ry`, `rz`, `crz`, or
 *       `cr`;
 *     - the integer operand, if the gate is named `crk`.
 *  - if the gate type is IDENTITY, an instruction type is inferred using a
 *    `qubit:W` operand for each qubit used (other operands are ignored);
 *  - if the gate type is HADAMARD, an instruction type is inferred using a
 *    single `qubit:W` operand for the first qubit used (other operands are
 *    ignored, and an error is emitted if there are no qubit operands);
 *  - if the gate type is PAULI_X, RX90, MRX90, or RX180, an instruction type is
 *    inferred using a single `qubit:X` operand for the first qubit used (other
 *    operands are ignored, and an error is emitted if there are no qubit
 *    operands);
 *  - if the gate type is PAULI_Y, RY90, MRY90, or RY180, an instruction type is
 *    inferred using a single `qubit:Y` operand for the first qubit used (other
 *    operands are ignored, and an error is emitted if there are no qubit
 *    operands);
 *  - if the gate type is PAULI_Z, PHASE, PHASE_DAG, T, or T_DAG, an instruction
 *    type is inferred using a single `qubit:Z` operand for the first qubit used
 *    (other operands are ignored, and an error is emitted if there are no qubit
 *    operands);
 *  - if the gate type is RX, RY, or RZ, an instruction type is inferred using
 *    `qubit:#, real:L` operands, where # is the axis (taken from the first
 *    qubit operand and the angle operand; other operands are ignored, and an
 *    error is emitted if there are no qubit operands);
 *  - if the gate type is PREP_Z, an instruction type is inferred using
 *    `qubit:W` for each qubit operand (other operands are ignored, and an error
 *    is emitted if there are no qubit operands);
 *  - if the gate type is CNOT, an instruction type is inferred using
 *    `qubit:Z, qubit:X` operands (taken from the first and second qubit
 *    operands; other operands are ignored, and an error is emitted if there are
 *    too few qubit operands);
 *  - if the gate type is CPHASE, an instruction type is inferred using
 *    `qubit:Z, qubit:Z` operands (taken from the first and second qubit
 *    operands; other operands are ignored, and an error is emitted if there are
 *    too few qubit operands);
 *  - if the gate type is SWAP, an instruction type is inferred using
 *    `qubit:W, qubit:W` operands (taken from the first and second qubit
 *    operands; other operands are ignored, and an error is emitted if there are
 *    too few qubit operands);
 *  - if the gate type is TOFFOLI, an instruction type is inferred using
 *    `qubit:Z, qubit:Z, qubit:X` operands (taken from the first, second, and
 *    third qubit operands; other operands are ignored, and an error is emitted
 *    if there are too few qubit operands);
 *  - if the gate type is MEASURE, DISPLAY, or DISPLAY_BINARY, an instruction
 *    type is inferred using `qubit:M` for each qubit operand (other operands
 *    are ignored, and an error is emitted if there are no qubit operands);
 *  - if the gate type is NOP, an instruction type with no operands is inferred
 *    (all operands are ignored);
 *  - if all of the above fails, an error is emitted.
 *
 * The old IR had no concept of (builtin) functions. Nevertheless, functions are
 * needed in the new IR to represent all functionality. The following functions
 * are always added:
 *
 *  - `operator!(bit) -> bit` for NOT, NAND, and NOR, conditions;
 *  - `operator&&(bit, bit) -> bit` for AND and NAND conditions;
 *  - `operator||(bit, bit) -> bit` for OR and NOR conditions;
 *  - `operator==(bit, bit) -> bit` for NXOR conditions;
 *  - `operator!=(bit, bit) -> bit` for XOR conditions;
 *  - `operator+(int, int) -> int` for `add` instructions;
 *  - `operator-(int, int) -> int` for `sub` instructions;
 *  - `operator&(int, int) -> int` for `and` instructions;
 *  - `operator|(int, int) -> int` for `or` instructions;
 *  - `operator^(int, int) -> int` for `xor` instructions;
 *  - `operator==(int, int) -> bool` for `eq` instructions;
 *  - `operator!=(int, int) -> bool` for `ne` instructions;
 *  - `operator<(int, int) -> bool` for `lt` instructions;
 *  - `operator>(int, int) -> bool` for `gt` instructions;
 *  - `operator<=(int, int) -> bool` for `le` instructions;
 *  - `operator>=(int, int) -> bool` for `ge` instructions;
 *  - `operator~(int) -> int` for `not` instructions;
 *  - `int(bit) -> int` for comparison instructions with creg target.
 *
 * The structured control-flow constructs of the old IR are mapped to the new
 * one trivially. Recursive structure is supported.
 */
Ref convert_old_to_new(const compat::ProgramRef &old);

} // namespace ir
} // namespace ql
