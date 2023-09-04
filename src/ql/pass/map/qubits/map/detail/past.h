#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/list.h"
#include "ql/utils/map.h"
#include "ql/utils/vec.h"
#include "ql/com/map/qubit_mapping.h"
#include "ql/ir/swap_parameters.h"
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
 * the 'main' Past. While mapping, several alternatives might be evaluated, each of
 * which also has a Past attached, and each of which for most of the parts
 * start off as a copy of the 'main' Past. But it is in fact a temporary
 * extension of this main Past.
 *
 * Past contains:
 * - the list of gates that are already mapped (this should be at all times a valid circuit
 *   with routed gates),
 * - the virtual to real qubit mapping after execution of above gates (i.e. swaps and moves added for routing),
 * - the free cycle map, which is a scheduling heuristic telling which qubits/references are free
 *   at which cycle. This allows routing to use paths that extend the overall circuit depth
 *   as little as possible.
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
     * Adds the given mapped gate to this past's FreeCycle map.
     * If output_circuit is provided, add the gate to it.
     */
    void add(const ir::CustomInstructionRef &gate, utils::Any<ir::Statement> *output_circuit = nullptr);

    /**
     * Returns a new gate with given name and qubits, throwing if
     * it was not successful.
     */
    ir::Maybe<ir::CustomInstruction> new_gate(
        const utils::Str &gname,
        const utils::Vec<utils::UInt> &qubits
    ) const;

    /**
     * Returns the number of swaps added to this past for routing.
     */
    utils::UInt get_num_swaps_added() const;

    /**
     * Returns the number of moves added to this past for routing.
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

    bool add_move(utils::UInt &r0, utils::UInt &r1, const ir::SwapParameters& swap_params, utils::Any<ir::Statement> *output_circuit = nullptr);

    /**
     * Generates a single swap/move with real operands and adds it to the
     * current past's waiting list. Note that the swap/move may be implemented
     * by a series of gates, and that a swap/move
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
     *
     * If output_circuit is provided, add the swap to it.
     */
    void add_swap(utils::UInt r0, utils::UInt r1, utils::Any<ir::Statement> *output_circuit = nullptr);

    /**
     * Returns the real qubit index implementing the given virtual qubit index.
     */
    utils::UInt get_real_qubit(utils::UInt virt);

    /**
     * Turns the given gate into a "real" gate, that is, maps its virtual qubit operands
     * to real qubit operands as described by v2r.
     */
    void make_real(const ir::CustomInstructionRef &gate);

    /**
     * Returns the first completely free cycle.
     */
    utils::UInt get_max_free_cycle() const;

private:
    ir::PlatformRef platform;
    OptionsRef options;

    /**
     * Current virtual to real qubit map.
     */
    com::map::QubitMapping v2r;

    /**
     * FreeCycle map of this Past.
     */
    FreeCycle fc;

    utils::UInt num_swaps_added = 0;
    utils::UInt num_moves_added = 0;
};

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
