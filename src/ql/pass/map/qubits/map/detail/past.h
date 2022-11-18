/** \file
 * Past implementation.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/list.h"
#include "ql/utils/map.h"
#include "ql/utils/vec.h"
#include "ql/ir/compat/compat.h"
#include "ql/com/map/qubit_mapping.h"
#include "options.h"
#include "free_cycle.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

/**
 * Past: state of the mapper while somewhere in the mapping process.
 *
 * There is a Past attached to the output stream, that is a kind of window with
 * a list of gates in it, to which gates are added after mapping. This is called
 * the 'main' Past. While mapping, several alternatives are evaluated, each of
 * which also has a Past attached, and each of which for most of the parts
 * start off as a copy of the 'main' Past. But it is in fact a temporary
 * extension of this main Past.
 *
 * Past contains gates of which the schedule might influence a future path
 * selected for mapping binary gates. It maintains for each qubit from which
 * cycle on it is free, so that swap insertion can exploit this to hide its
 * overall circuit latency overhead by increasing ILP. Also it maintains the 1
 * to 1 (reversible) virtual to real qubit map: all gates in past and beyond are
 * mapped and have real qubits as operands. While experimenting with path
 * alternatives, a clone is made of the main past, to insert swaps and evaluate
 * the latency effects; note that inserting swaps changes the mapping.
 *
 * On arrival of a quantum gate(s):
 *  - [isempty(waiting_gates)]
 *  - if 2q nonNN clone mult. pasts, in each clone add swap/move gates,
 *    schedule, evaluate clones, select, add swaps to mainPast
 *  - add(), add(), ...: add quantum gates to waiting_gates, waiting to be
 *    scheduled in [!isempty(waiting_gates)]
 *  - schedule(): schedules all quantum gates of waiting_gates into gates
 *    [isempty(waiting_gates) && !isempty(gates)]
 *
 * On arrival of a classical gate:
 *  - flush_all: gates flushed to output_gates
 *    [isempty(waiting_gates) && isempty(gates) && !isempty(output_gates)]
 *  - bypass: classical gate added to output_gates
 *    [isempty(waiting_gates) && isempty(gates) && !isempty(output_gates)]
 *
 * On no gates:
 *  - [isempty(waiting_gates)]
 *  - flush_all: lg flushed to output_gates
 *    [isempty(waiting_gates) && isempty(gates) && !isempty(output_gates)]
 *
 * On end:
 *  - flush_to_circuit: output_gates flushed to output circuit
 *    [isempty(waiting_gates) && isempty(gates) && isempty(output_gates)]
 */
class Past {
public:
    Past(ir::PlatformRef p, const OptionsRef &opt);

    /**
     * Copies the given qubit mapping into our mapping.
     */
    void import_mapping(const com::map::QubitMapping &v2r_value);

    /**
     * Copies our qubit mapping into the given mapping.
     */
    void export_mapping(com::map::QubitMapping &v2r_destination) const;

    /**
     * Prints the state of the embedded FreeCycle object.
     */
    void print_fc() const;

    /**
     * Prints the state of the embedded FreeCycle object only when verbosity
     * is at least debug.
     */
    void debug_print_fc() const;

    /**
     * Prints the state of this object along with the given string.
     */
    void print(const utils::Str &s) const;

    /**
     * Schedules all waiting gates into the main gates list. Note that these
     * gates all are mapped and so have real operand qubit indices. The
     * FreeCycle map reflects for each qubit the first free cycle. All new
     * gates, now in waitinglist, get such a cycle assigned below, increased
     * gradually, until definitive.
     */
    void schedule();

    /**
     * Adds the given mapped gate to the current past. This means adding it to
     * the current past's waiting list, waiting for it to be scheduled later.
     */
    void add(const ir::CustomInstructionRef &gate);

    /**
     * Creates a new gate with given name and qubits, returning whether this was
     * successful. Return the created gate(s) in circ, which is supposed to be
     * empty on entry.
     *
     * Since kernel.h only provides a gate interface as method of class
     * Kernel, that adds the gate to kernel.c, and we want the gate (or its
     * decomposed sequence) here to be added to circ, the kludge is implemented
     * to make sure that kernel.c (the current kernel's mapper input/output
     * circuit) is available for this. In class Future, kernel.c is copied into
     * the dependence graph or copied to a local circuit, and in
     * Mapper::route, a temporary local output circuit is used, which is
     * written to kernel.c only at the very end.
     */
    ir::Maybe<ir::CustomInstruction> new_gate(
        const utils::Str &gname,
        const utils::Vec<utils::UInt> &qubits
    ) const;

    /**
     * Returns the number of swaps added to this past.
     */
    utils::UInt get_num_swaps_added() const;

    /**
     * Returns the number of moves added to this past.
     */
    utils::UInt get_num_moves_added() const;

    /**
     * Returns whether swap(fr0,fr1) starts earlier than swap(sr0,sr1). This is
     * really a short-cut ignoring config file and perhaps several other
     * details.
     */
    utils::Bool is_first_swap_earliest(
        utils::UInt fr0,
        utils::UInt fr1,
        utils::UInt sr0,
        utils::UInt sr1
    ) const;

    /**
     * Generates a move into circ with parameters r0 and r1 (which
     * generate_move() may reverse). Whether this was successfully done can be
     * seen from whether circ was extended. Please note that the reversal of
     * operands may have been done also when generate_move() was not successful.
     */
    void add_move(utils::UInt &r0, utils::UInt &r1);

    /**
     * Generates a single swap/move with real operands and adds it to the
     * current past's waiting list. Note that the swap/move may be implemented
     * by a series of gates (circuit circ below), and that a swap/move
     * essentially is a commutative operation, interchanging the states of the
     * two qubits.
     *
     * A move is implemented by 2 CNOTs, while a swap is 3 CNOTs, provided the
     * target qubit is in |0> (inited) state. So, when one of the operands is
     * the current location of an unused virtual qubit, use a move with that
     * location as 2nd operand, after first having initialized the target qubit
     * in |0> (inited) state when that has not been done already. However, this
     * initialization must not extend the depth (beyond the configured limit),
     * so this can only be done when cycles for it are for free.
     */
    void add_swap(utils::UInt r0, utils::UInt r1);

    /**
     * Adds the mapped gate (with real qubit indices as operands) to the past
     * by adding it to the waiting list and scheduling it into the past.
     */
    void add_and_schedule(const ir::CustomInstructionRef &gate);

    /**
     * Returns the real qubit index implementing virtual qubit index.
     */
    utils::UInt get_real_qubit(utils::UInt virt);

    /**
     * Turns the given gate into a "real" gate.
     *
     * This assumes that the given gate is a virtual gate with virtual qubit
     * indices as operands. When a gate can be created with the same name but
     * with "_real" appended, with the real qubits as operands, then create that
     * gate, otherwise keep the old gate, replacing the virtual qubit operands
     * by the real qubit indices. Since creating a new gate may result in a
     * decomposition to several gates, the result is returned as a circuit
     * vector.
     *
     * So each gate in the circuit (optionally) passes through the following
     * phases.
     *
     *   1. It is created. When it maps to a decomposition in the config file,
     *      it is decomposed immediately, otherwise the gate is created normally
     *      (k.gate). So we expect gates like x, cz, cnot to be specified in the
     *      config file; on the resulting (decomposed) gates, the routing is
     *      done, including depth/cost estimation.
     *
     *  2a. If needed for mapping, a swap/move is created. First try creating
     *      swap_real/move_real as above, otherwise just swap/real (AddSwap).
     *      So we expect gates like swap_real and move_real to be specified in
     *      the config file. swap_real/move_real, unlike swap/real, allow
     *      immediate decomposition; when no swap_real/move_real are specified,
     *      just swap/move must be present and swap/move are created, usually
     *      without decomposition. The routing is done on the resulting
     *      (decomposed) gates, including depth/cost estimation; when the
     *      resulting gates end in _prim, see step 3.
     *
     *  2b. The resulting gates of step 1 have their operands/gate mapped. First
     *      try creating gate_real as above, otherwise just gate (make_real()).
     *      gate_real, unlike gate, allows immediate decomposition; when the
     *      resulting gates end in _prim, see step 3.
     *
     *   3. Make primitive gates. For each gate try recreating it with _prim
     *      appended to its name, otherwise keep it; this decomposes those with
     *      corresponding _prim entries.
     *
     *   4. Final schedule: the resulting gates are subject to final scheduling
     *      (the original resource-constrained scheduler).
     */
    void make_real(const ir::CustomInstructionRef &gate);

    /**
     * Returns the first completely free cycle.
     */
    utils::UInt get_max_free_cycle() const;

    /**
     * Add the given non-qubit gate directly to the output list.
     */
    void bypass(const ir::CustomInstructionRef &gate);

    /**
     * Flushes the output gate list to the given circuit.
     */
    utils::Any<ir::Statement> flush_to_circuit();

private:
    ir::PlatformRef platform;
    OptionsRef options;

    /**
     * State: current virtual to real qubit map, imported/exported to kernel.
     */
    com::map::QubitMapping v2r;

    /**
     * State: FreeCycle map (including resource_manager) of this Past.
     */
    FreeCycle fc;

    /**
     * List of quantum gates in this Past, topological order, waiting to be
     * scheduled. This only contains gates from add() and the final schedule()
     * call. When evaluating alternatives, it is empty when Past is cloned; so
     * no state.
     */
    utils::List<ir::CustomInstructionRef> waiting_gates;

    /**
     * List of gates flushed out of this Past, not yet put in outCirc when
     * evaluating alternatives. output_gates stays constant; so no state.
     */
    utils::List<ir::CustomInstructionRef> output_gates;

    utils::Map<ir::CustomInstructionRef, utils::UInt> cycle;

    utils::UInt num_swaps_added = 0;
    utils::UInt num_moves_added = 0;
};

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
