/** \file
 * Clifford sequence optimizer.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/vec.h"
#include "ql/ir/compat/compat.h"

namespace ql {
namespace pass {
namespace opt {
namespace clifford {
namespace optimize {
namespace detail {

/**
 * Clifford optimizer logic implementation.
 */
class Clifford {
private:

    /**
     * Shorthand for the number of qubits in the kernel.
     */
    utils::UInt nq;

    /**
     * Shorthand for the platform's cycle time in nanoseconds.
     */
    utils::UInt ct;

    /**
     * Current accumulated Clifford state per qubit.
     */
    utils::Vec<utils::Int> cliffstate;

    /**
     * Current accumulated Clifford cycles per qubit.
     */
    utils::Vec<utils::UInt> cliffcycles;

    /**
     * Total number of cycles saved per kernel.
     */
    utils::UInt total_saved;

    /**
     * Create gate sequences for all accumulated cliffords, output them and
     * reset state.
     */
    void sync_all(const ir::compat::KernelRef &k);

    /**
     * Create gate sequence for accumulated cliffords of qubit q, output it and
     * reset state.
     */
    void sync(const ir::compat::KernelRef &k, utils::UInt q);

    /**
     * Clifford state transition table.
     *
     * [from state][accumulating sequence represented as state] => new state
     */
    const utils::Int TRANSITION_TABLE[24][24] = {
        {  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23 },
        {  1, 2, 0,10,11, 9, 4, 5, 3, 7, 8, 6,23,21,22,14,12,13,20,18,19,17,15,16 },
        {  2, 0, 1, 8, 6, 7,11, 9,10, 5, 3, 4,16,17,15,22,23,21,19,20,18,13,14,12 },
        {  3, 4, 5, 0, 1, 2, 9,10,11, 6, 7, 8,15,16,17,12,13,14,21,22,23,18,19,20 },
        {  4, 5, 3, 7, 8, 6, 1, 2, 0,10,11, 9,20,18,19,17,15,16,23,21,22,14,12,13 },
        {  5, 3, 4,11, 9,10, 8, 6, 7, 2, 0, 1,13,14,12,19,20,18,22,23,21,16,17,15 },
        {  6, 7, 8, 9,10,11, 0, 1, 2, 3, 4, 5,18,19,20,21,22,23,12,13,14,15,16,17 },
        {  7, 8, 6, 4, 5, 3,10,11, 9, 1, 2, 0,17,15,16,20,18,19,14,12,13,23,21,22 },
        {  8, 6, 7, 2, 0, 1, 5, 3, 4,11, 9,10,22,23,21,16,17,15,13,14,12,19,20,18 },
        {  9,10,11, 6, 7, 8, 3, 4, 5, 0, 1, 2,21,22,23,18,19,20,15,16,17,12,13,14 },
        { 10,11, 9, 1, 2, 0, 7, 8, 6, 4, 5, 3,14,12,13,23,21,22,17,15,16,20,18,19 },
        { 11, 9,10, 5, 3, 4, 2, 0, 1, 8, 6, 7,19,20,18,13,14,12,16,17,15,22,23,21 },
        { 12,13,14,21,22,23,18,19,20,15,16,17, 0, 1, 2, 9,10,11, 6, 7, 8, 3, 4, 5 },
        { 13,14,12,16,17,15,22,23,21,19,20,18, 5, 3, 4, 2, 0, 1, 8, 6, 7,11, 9,10 },
        { 14,12,13,20,18,19,17,15,16,23,21,22,10,11, 9, 4, 5, 3, 7, 8, 6, 1, 2, 0 },
        { 15,16,17,18,19,20,21,22,23,12,13,14, 3, 4, 5, 6, 7, 8, 9,10,11, 0, 1, 2 },
        { 16,17,15,13,14,12,19,20,18,22,23,21, 2, 0, 1, 5, 3, 4,11, 9,10, 8, 6, 7 },
        { 17,15,16,23,21,22,14,12,13,20,18,19, 7, 8, 6, 1, 2, 0,10,11, 9, 4, 5, 3 },
        { 18,19,20,15,16,17,12,13,14,21,22,23, 6, 7, 8, 3, 4, 5, 0, 1, 2, 9,10,11 },
        { 19,20,18,22,23,21,16,17,15,13,14,12,11, 9,10, 8, 6, 7, 2, 0, 1, 5, 3, 4 },
        { 20,18,19,14,12,13,23,21,22,17,15,16, 4, 5, 3,10,11, 9, 1, 2, 0, 7, 8, 6 },
        { 21,22,23,12,13,14,15,16,17,18,19,20, 9,10,11, 0, 1, 2, 3, 4, 5, 6, 7, 8 },
        { 22,23,21,19,20,18,13,14,12,16,17,15, 8, 6, 7,11, 9,10, 5, 3, 4, 2, 0, 1 },
        { 23,21,22,17,15,16,20,18,19,14,12,13, 1, 2, 0, 7, 8, 6, 4, 5, 3,10,11, 9 }
    };

    /**
     * Find the clifford state from identity to given gate, or return -1 if
     * unknown or the gate is not in C1.
     *
     * TODO: this currently infers the Clifford index by gate name; instead
     *  semantics like this should be in the config file somehow.
     */
    static utils::Int gate2cs(const ir::compat::GateRef &gate);

    /**
     * Find the duration of the gate sequence corresponding to given clifford
     * state.
     *
     * TODO: should be implemented using configuration file, searching for
     *  created gates and retrieving durations
     */
    static utils::UInt cs2cycles(utils::Int cs);

    /**
     * Return the gate sequence as string for debug output corresponding to
     * given clifford state.
     */
    static utils::Str cs2string(utils::Int cs);

public:

    /**
     * Optimizes the given kernel, returning how many cycles were saved.
     */
    utils::UInt optimize_kernel(const ir::compat::KernelRef &kernel);

};

} // namespace detail
} // namespace optimize
} // namespace clifford
} // namespace opt
} // namespace pass
} // namespace ql
