/** \file
 * Virtual to real qubit mapping state tracker.
 */

#pragma once

#include <iostream>
#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/vec.h"

namespace ql {
namespace com {

/**
 * Value used to specify that a virtual qubit has no real qubit associated with
 * it.
 */
const utils::UInt UNDEFINED_QUBIT = utils::MAX;

/**
 * The state of a real qubit.
 */
enum struct QubitState {

    /**
     * Qubit has no relevant state needing preservation, i.e. is garbage.
     */
    NONE,

    /**
     * Qubit has initialized state suitable for replacing swap by move.
     */
    INITIALIZED,

    /**
     * Qubit has a unique state which must be preserved.
     */
    LIVE

};

/**
 * Converts QubitState to a string.
 */
std::ostream &operator<<(std::ostream &os, QubitState qs);

/**
 * Virtual to real qubit mapping. Maintains the mapping (in both directions), as
 * well as information about whether the state of a real qubit is in use or not.
 */
class QubitMapping {
private:

    /**
     * Size of the map.
     */
    utils::UInt nq;

    /**
     * Maps virtual qubit indices to real qubit indices or UNDEFINED_QUBIT.
     */
    utils::Vec<utils::UInt> virt_to_real;

    /**
     * Maps real qubit indices to their state.
     */
    utils::Vec<QubitState> real_state;

public:

    /**
     * Creates a virtual to real qubit map with the given number of qubits.
     *
     * The mapping starts off undefined for all virtual qubits, unless
     * one_to_one is set, in which case virtual qubit i maps to real qubit i for
     * all qubits. The state of the qubits is initialized as specified.
     */
    explicit QubitMapping(
        utils::UInt num_qubits = 0,
        utils::Bool one_to_one = false,
        QubitState initial_state = QubitState::NONE
    );

    /**
     * Resizes/reinitializes the map.
     *
     * Newly added qubits start off with an undefined mapping, unless one_to_one
     * is set, in which case virtual qubit i maps to real qubit i for all
     * qubits. The state of the new qubits is initialized as specified.
     */
    void resize(
        utils::UInt num_qubits,
        utils::Bool one_to_one = false,
        QubitState initial_state = QubitState::NONE
    );

    /**
     * Map virtual qubit index to real qubit index.
     */
    utils::UInt &operator[](utils::UInt v);

    /**
     * Map virtual qubit index to real qubit index.
     */
    const utils::UInt &operator[](utils::UInt v) const;

    /**
     * Returns the underlying virtual to real qubit vector.
     */
    const utils::Vec<utils::UInt> &get_virt_to_real() const;

    /**
     * Map real qubit to the virtual qubit index that is mapped to it (i.e.
     * backward map). When none, return UNDEFINED_QUBIT. This currently loops
     * over all qubits, so it isn't particularly fast.
     */
    utils::UInt get_virtual(utils::UInt real) const;

    /**
     * Returns the current state for the given real qubit.
     */
    QubitState get_state(utils::UInt real) const;

    /**
     * Sets the state for the given real qubit.
     */
    void set_state(utils::UInt real, QubitState state);

    /**
     * Returns the underlying qubit state vector.
     */
    const utils::Vec<QubitState> &get_state() const;

    /**
     * Allocate a real qubit for the given unmapped virtual qubit.
     */
    utils::UInt allocate(utils::UInt virt);

    /**
     * Updates the mapping to reflect a swap for the given real qubit indices,
     * so when v0 was in r0 and v1 was in r1, then v0 is now in r1 and v1 is now
     * in r0.
     */
    void swap(utils::UInt r0, utils::UInt r1);

    /**
     * Returns a string representation of the state of the given real qubit.
     */
    utils::Str real_to_string(utils::UInt real) const;

    /**
     * Returns a string representation of the state of the given virtual qubit.
     */
    utils::Str virtual_to_string(utils::UInt virt) const;

    /**
     * Dumps the state of this mapping to the given stream.
     */
    void dump_state(std::ostream &os=std::cout, const utils::Str &line_prefix="") const;

};

} // namespace com
} // namespace ql
