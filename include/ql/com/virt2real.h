/** \file
 * Virt2Real: map of a virtual qubit index to its real qubit index.
 *
 * TODO JvS: clean up docs
 *
 * Mapping maps each used virtual qubit to a real qubit index, but which one that is, may change.
 * For a 2-qubit gate its operands should be nearest neighbor; when its virtual operand qubits
 * are not mapping to nearest neighbors, that should be accomplished by moving/swapping
 * the virtual qubits from their current real qubits to real qubits that are nearest neighbors:
 * those moves/swaps are inserted just before that 2-qubit gate.
 * Anyhow, the virtual operand qubits of gates must be mapped to the real ones, holding their state.
 *
 * The number of virtual qubits is less equal than the number of real qubits,
 * so their indices use the same data type (utils::UInt) and the same range type 0<=index<nq.
 *
 * Virt2Real maintains two maps:
 * - a map (v2rMap[]) for each virtual qubit that is in use to its current real qubit index.
 *      Virtual qubits are in use as soon as they have been encountered as operands in the program.
 *      When a virtual qubit is not in use, it maps to UNDEFINED_QUBIT, the undefined real index.
 *      The reverse map (GetVirt()) is implemented by a reverse look-up:
 *      when there is no virtual qubit that maps to a particular real qubit,
 *      the reverse map maps the real qubit index to UNDEFINED_QUBIT, the undefined virtual index.
 *      At any time, the virtual to real and reverse maps are 1-1 for qubits that are in use.
 * - a map for each real qubit whether there is state in it, and, if so, which (rs[]).
 *      When a gate (except for swap/move) has been executed on a real qubit,
 *      its state becomes valuable and must be preserved (rs_hasstate below).
 *      But before that, it can be in a garbage state (rs_nostate below) or in a known state (rs_wasinited below).
 *      The latter is used to replace a swap using a real qubit with such state by a move, which is cheaper.
 * There is no support yet to make a virtual qubit not in use (which could be after a measure),
 * nor to bring a real qubit in the rs_wasinited or rs_nostate state (perhaps after measure or prep).
 *
 * Some special situations are worth mentioning:
 * - while a virtual qubit is being swapped/moved near to an other one,
 *      along the trip real qubits may be used which have no virtual qubit mapping to them;
 *      a move can then be used which assumes the 2nd real operand in the |0> (inited) state, and leaves
 *      the 1st real operand in that state (while the 2nd has assumed the state of the former 1st).
 *      the mapper implementation assumes that all real qubits in the rs_wasinited state are in that state.
 * - on program start, no virtual qubit has a mapping yet to a real qubit;
 *      mapping is initialized while virtual qubits are encountered as operands.
 * - with multiple kernels, kernels assume the (unified) mapping from their predecessors and leave
 *      the result mapping to their successors in the kernels' Control Flow Graph;
 *      i.e. Virt2Real is what is passed between kernels as dynamic state;
 *      statically, the grid, the maximum number of real qubits and the current platform stay unchanged.
 * - while evaluating sets of swaps/moves as variations to continue mapping, Virt2Real is passed along
 *      to represent the mapping state after such swaps/moves where done; when deciding on a particular
 *      variation, the v2r mapping in the mainPast is made to replect the swaps/moves done.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/vec.h"

namespace ql {
namespace com {
namespace virt2real {

typedef enum realstate {
    rs_nostate,     // real qubit has no relevant state needing preservation, i.e. is garbage
    rs_wasinited,   // real qubit has initialized state suitable for replacing swap by move
    rs_hasstate     // real qubit has a unique state which must be preserved
} realstate_t;

const utils::UInt UNDEFINED_QUBIT = utils::MAX;

class Virt2Real {
private:

    utils::UInt             nq;     // size of the map; after initialization, will always be the same
    utils::Vec<utils::UInt> v2rMap; // v2rMap[virtual qubit index] -> real qubit index | UNDEFINED_QUBIT
    utils::Vec<realstate_t> rs;     // rs[real qubit index] -> {nostate|wasinited|hasstate}

public:

    // map real qubit to the virtual qubit index that is mapped to it (i.e. backward map);
    // when none, return UNDEFINED_QUBIT;
    // a second vector next to v2rMap (i.e. an r2vMap) would speed this up;
    utils::UInt GetVirt(utils::UInt r) const;
    realstate_t GetRs(utils::UInt q) const;
    void SetRs(utils::UInt q, realstate_t rsvalue);

    // expand to desired size
    //
    // mapping starts off undefined for all virtual qubits
    // (unless option mapinitone2one is set, then virtual qubit i maps to real qubit i for all qubits)
    //
    // real qubits are assumed to have a garbage state
    // (unless option mapassumezeroinitstate was set,
    //  then all real qubits are assumed to have a state suitable for replacing swap by move)
    //
    // the rs initializations are done only once, for a whole program
    void Init(utils::UInt n);

    // map virtual qubit index to real qubit index
    utils::UInt &operator[](utils::UInt v);
    const utils::UInt &operator[](utils::UInt v) const;

    // allocate a new real qubit for an unmapped virtual qubit v (i.e. v2rMap[v] == UNDEFINED_QUBIT);
    // note that this may consult the grid or future gates to find a best real
    // and thus should not be in Virt2Real but higher up
    utils::UInt AllocQubit(utils::UInt v);

    // r0 and r1 are real qubit indices;
    // by execution of a swap(r0,r1), their states are exchanged at runtime;
    // so when v0 was in r0 and v1 was in r1, then v0 is now in r1 and v1 is in r0;
    // update v2r accordingly
    void Swap(utils::UInt r0, utils::UInt r1);

    void DPRINTReal(utils::UInt r) const;
    void PrintReal(utils::UInt r) const;
    void PrintVirt(utils::UInt v) const;
    void DPRINTReal(const utils::Str &s, utils::UInt r0, utils::UInt r1) const;
    void PrintReal(const utils::Str &s, utils::UInt r0, utils::UInt r1) const;
    void DPRINT(const utils::Str &s) const;
    void Print(const utils::Str &s) const;
    void Export(utils::Vec<utils::UInt> &kv2rMap) const;
    void Export(utils::Vec<utils::Int> &krs) const;

};

} // namespace virt2real
} // namespace com
} // namespace ql
