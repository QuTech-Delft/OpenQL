/** \file
 * FreeCycle implementation.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "ql/utils/opt.h"
#include "ql/plat/platform.h"
#include "ql/plat/resource/manager.h"
#include "ql/ir/ir.h"
#include "ql/com/qubit_mapping.h"
#include "options.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace route {
namespace detail {

/**
 * FreeCycle: map each real qubit to the first cycle that it is free for use.
 *
 * In scheduling gates, qubit dependencies cause latencies. For each real qubit,
 * the first cycle that it is free to use is the cycle that the last gate that
 * was scheduled in the qubit, has just finished (i.e. in the previous cycle);
 * the map serves as a summary to ease scheduling next gates.
 *
 * Likewise, while mapping, swaps are scheduled just before a non-NN two-qubit
 * gate. Moreover, such swaps may involve real qubits on the path between the
 * real operand qubits of the gate, which may be different from the real operand
 * qubits. The evaluation of which path of swaps is best is, among other data,
 * based on which path causes the latency of the whole circuit to be extended
 * the least. This latency extension is measured from the data in the FreeCycle
 * map; so a FreeCycle map is part of each path of swaps that is evaluated for a
 * particular non-NN 2-qubit gate next to a FreeCycle map that is part of the
 * output stream (the main past).
 *
 * Since gate durations are in nanoseconds, and one cycle is some fixed number
 * of nanoseconds, the duration is converted to a rounded-up number of cycles
 * when computing the added latency.
 */
class FreeCycle {
private:

    plat::PlatformRef        platformp;   // platform description
    OptionsRef               options;     // parsed mapper pass options
    utils::UInt              nq;          // map is (nq+nb) long; after initialization, will always be the same
    utils::UInt              nb;          // bregs are in map (behind qubits) to track dependences around conditions
                                          // FIXME JvS: why qubits and bregs, but not cregs?
    utils::UInt              ct;          // multiplication factor from cycles to nano-seconds (unit of duration)
    utils::Vec<utils::UInt>  fcv;         // fcv[real qubit index i]: qubit i is free from this cycle on
    utils::Opt<plat::resource::State> rs; // actual resources occupied by scheduled gates


    // access free cycle value of qubit q[i] or breg b[i-nq]
    utils::UInt &operator[](utils::UInt i);
    const utils::UInt &operator[](utils::UInt i) const;

public:

    // explicit FreeCycle constructor
    // needed for virgin construction
    // default constructor was deleted because it cannot construct resource_manager_t without parameters
    FreeCycle();

    void Init(const plat::PlatformRef &p, const OptionsRef &opt);

    // depth of the FreeCycle map
    // equals the max of all entries minus the min of all entries
    // not used yet; would be used to compute the max size of a top window on the past
    utils::UInt Depth() const;

    // min of the FreeCycle map equals the min of all entries;
    utils::UInt Min() const;

    // max of the FreeCycle map equals the max of all entries;
    utils::UInt Max() const;

    void DPRINT(const utils::Str &s) const;
    void Print(const utils::Str &s) const;

    // return whether gate with first operand qubit r0 can be scheduled earlier than with operand qubit r1
    utils::Bool IsFirstOperandEarlier(utils::UInt r0, utils::UInt r1) const;

    // will a swap(fr0,fr1) start earlier than a swap(sr0,sr1)?
    // is really a short-cut ignoring config file and perhaps several other details
    utils::Bool IsFirstSwapEarliest(utils::UInt fr0, utils::UInt fr1, utils::UInt sr0, utils::UInt sr1) const;

    // when we would schedule gate g, what would be its start cycle? return it
    // gate operands are real qubit indices and breg indices
    // is purely functional, doesn't affect state
    utils::UInt StartCycleNoRc(const ir::GateRef &g) const;

    // when we would schedule gate g, what would be its start cycle? return it
    // gate operands are real qubit indices and breg indices
    // is purely functional, doesn't affect state
    utils::UInt StartCycle(const ir::GateRef &g) const;

    // schedule gate g in the FreeCycle map
    // gate operands are real qubit indices and breg indices
    // the FreeCycle map is updated, not the resource map
    // this is done, because AddNoRc is used to represent just gate dependences, avoiding a build of a dep graph
    void AddNoRc(const ir::GateRef &g, utils::UInt startCycle);

    // schedule gate g in the FreeCycle and resource maps
    // gate operands are real qubit indices and breg indices
    // both the FreeCycle map and the resource map are updated
    // startcycle must be the result of an earlier StartCycle call (with rc!)
    void Add(const ir::GateRef &g, utils::UInt startCycle);

};

} // namespace detail
} // namespace route
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
