/** \file
 * Past implementation.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/list.h"
#include "ql/utils/map.h"
#include "ql/utils/vec.h"
#include "ql/plat/platform.h"
#include "ql/ir/ir.h"
#include "ql/com/qubit_mapping.h"
#include "options.h"
#include "free_cycle.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace route {
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
 *  - [isempty(waitinglg)]
 *  - if 2q nonNN clone mult. pasts, in each clone Add swap/move gates, Schedule, evaluate clones, select, Add swaps to mainPast
 *  - Add, Add, ...: add quantum gates to waitinglg, waiting to be scheduled in [!isempty(waitinglg)]
 *  - Schedule: schedules all quantum gates of waitinglg into lg [isempty(waitinglg) && !isempty(lg)]
 *
 * On arrival of a classical gate:
 *  - FlushAll: lg flushed to outlg [isempty(waitinglg) && isempty(lg) && !isempty(outlg)]
 *  - ByPass: classical gate added to outlg [isempty(waitinglg) && isempty(lg) && !isempty(outlg)]
 *
 * On no gates:
 *  - [isempty(waitinglg)]
 *  - FlushAll: lg flushed to outlg [isempty(waitinglg) && isempty(lg) && !isempty(outlg)]
 *
 * On end:
 *  - Out: outlg flushed to outCirc [isempty(waitinglg) && isempty(lg) && isempty(outlg)]
 */
class Past {
private:

    utils::UInt                 nq;         // width of Past, Virt2Real, UseCount maps in number of real qubits
    utils::UInt                 nb;         // extends FreeCycle next to qubits with bregs
    utils::UInt                 ct;         // cycle time, multiplier from cycles to nano-seconds
    plat::PlatformRef           platformp;  // platform describing resources for scheduling
    ir::KernelRef               kernelp;    // current kernel for creating gates
    OptionsRef                  options;    // parsed mapper pass options

    com::QubitMapping           v2r;        // state: current Virt2Real map, imported/exported to kernel
    FreeCycle                   fc;         // state: FreeCycle map (including resource_manager) of this Past
    utils::List<ir::GateRef>    waitinglg;  // . . .  list of q gates in this Past, topological order, waiting to be scheduled in
    //        waitinglg only contains gates from Add and final Schedule call
    //        when evaluating alternatives, it is empty when Past is cloned; so no state
public:
    utils::List<ir::GateRef>    lg;         // state: list of q gates in this Past, scheduled by their (start) cycle values
    //        so this is the result list of this Past, to compare with other Alters
private:
    utils::List<ir::GateRef>    outlg;      // . . .  list of gates flushed out of this Past, not yet put in outCirc
    //        when evaluating alternatives, outlg stays constant; so no state
    utils::Map<ir::GateRef,utils::UInt> cycle;      // state: gate to cycle map, startCycle value of each past gatecycle[gp]
    //        cycle[gp] can be different for each gp for each past
    //        gp->cycle is not used by MapGates
    //        although updated by set_cycle called from MakeAvailable/TakeAvailable
    utils::UInt                 nswapsadded;// number of swaps (including moves) added to this past
    utils::UInt                 nmovesadded;// number of moves added to this past

public:

    // explicit Past constructor
    // needed for virgin construction
    Past();

    // past initializer
    void Init(const plat::PlatformRef &p, const ir::KernelRef &k, const OptionsRef &opt);

    // import Past's v2r from v2r_value
    void ImportV2r(const com::QubitMapping &v2r_value);

    // export Past's v2r into v2r_destination
    void ExportV2r(com::QubitMapping &v2r_destination) const;

    void DFcPrint() const;
    void FcPrint() const;
    void Print(const utils::Str &s) const;

    // all gates in past.waitinglg are scheduled here into past.lg
    // note that these gates all are mapped and so have real operand qubit indices
    // the FreeCycle map reflects for each qubit the first free cycle
    // all new gates, now in waitinglist, get such a cycle assigned below, increased gradually, until definitive
    void Schedule();

    // compute costs in cycle extension of optionally scheduling initcirc before the inevitable circ
    utils::Int InsertionCost(const ir::Circuit &initcirc, const ir::Circuit &circ) const;

    // add the mapped gate to the current past
    // means adding it to the current past's waiting list, waiting for it to be scheduled later
    void Add(const ir::GateRef &gp);

    // create a new gate with given name and qubits
    // return whether this was successful
    // return the created gate(s) in circ (which is supposed to be empty on entry)
    //
    // since kernel.h only provides a gate interface as method of class quantum_kernel, that adds the gate to kernel.c,
    // and we want the gate (or its decomposed sequence) here to be added to circ,
    // the kludge is implemented to make sure that kernel.c (the current kernel's mapper input/output circuit)
    // is available for this:
    // in class Future, kernel.c is copied into the dependence graph or copied to a local circuit; and
    // in Mapper::MapCircuit, a temporary local output circuit is used, which is written to kernel.c only at the very end
    utils::Bool new_gate(
        ir::Circuit &circ,
        const utils::Str &gname,
        const utils::Vec<utils::UInt> &qubits,
        const utils::Vec<utils::UInt> &cregs = {},
        utils::UInt duration = 0,
        utils::Real angle = 0.0,
        const utils::Vec<utils::UInt> &bregs = {},
        ir::ConditionType gcond = ir::ConditionType::ALWAYS,
        const utils::Vec<utils::UInt> &gcondregs = {}
    ) const;

    // return number of swaps added to this past
    utils::UInt NumberOfSwapsAdded() const;

    // return number of moves added to this past
    utils::UInt NumberOfMovesAdded() const;

    static void new_gate_exception(const utils::Str &s);

    // will a swap(fr0,fr1) start earlier than a swap(sr0,sr1)?
    // is really a short-cut ignoring config file and perhaps several other details
    utils::Bool IsFirstSwapEarliest(utils::UInt fr0, utils::UInt fr1, utils::UInt sr0, utils::UInt sr1) const;

    // generate a move into circ with parameters r0 and r1 (which GenMove may reverse)
    // whether this was successfully done can be seen from whether circ was extended
    // please note that the reversal of operands may have been done also when GenMove was not successful
    void GenMove(ir::Circuit &circ, utils::UInt &r0, utils::UInt &r1);

    // generate a single swap/move with real operands and add it to the current past's waiting list;
    // note that the swap/move may be implemented by a series of gates (circuit circ below),
    // and that a swap/move essentially is a commutative operation, interchanging the states of the two qubits;
    //
    // a move is implemented by 2 CNOTs, while a swap by 3 CNOTs, provided the target qubit is in |0> (inited) state;
    // so, when one of the operands is the current location of an unused virtual qubit,
    // use a move with that location as 2nd operand,
    // after first having initialized the target qubit in |0> (inited) state when that has not been done already;
    // but this initialization must not extend the depth so can only be done when cycles for it are for free
    void AddSwap(utils::UInt r0, utils::UInt r1);

    // add the mapped gate (with real qubit indices as operands) to the past
    // by adding it to the waitinglist and scheduling it into the past
    void AddAndSchedule(const ir::GateRef &gp);

    // find real qubit index implementing virtual qubit index;
    // if not yet mapped, allocate a new real qubit index and map to it
    utils::UInt MapQubit(utils::UInt v);

    static void stripname(utils::Str &name);

    // MakeReal gp
    // assume gp points to a virtual gate with virtual qubit indices as operands;
    // when a gate can be created with the same name but with "_real" appended, with the real qubits as operands, then create that gate
    // otherwise keep the old gate; replace the virtual qubit operands by the real qubit indices
    // since creating a new gate may result in a decomposition to several gates, the result is returned as a circuit vector
    //
    // So each gate in the circuit (optionally) passes through the following phases:
    // 1. it is created:
    //      when a decomposition in config file, decompose immediately, otherwise just create (k.gate)
    //      so we expect gates like: x, cz, cnot to be specified in config file;
    //      on the resulting (decomposed) gates, the routing is done including depth/cost estimation
    // 2a.if needed for mapping, swap/move is created:
    //      first try creating swap_real/move_real as above, otherwise just swap/real (AddSwap)
    //      so we expect gates like: swap_real, move_real to be specified in config file,
    //      swap_real/move_real unlike swap/real allow immediate decomposition;
    //      when no swap_real/move_real are specified, just swap/move must be present
    //      and swap/move are created usually without decomposition;
    //      on the resulting (decomposed) gates, the routing is done including depth/cost estimation;
    //      when the resulting gates end in _prim, see step 3
    // 2b.the resulting gates of step 1: map operands/gate:
    //      first try creating gate_real as above, otherwise just gate (MakeReal)
    //      gate_real unlike gate allows immediate decomposition;
    //      when the resulting gates end in _prim, see step 3
    // 3. make primitive gates:
    //      for each gate try recreating it with _prim appended to its name, otherwise keep it; this decomposes those with corresponding _prim entries
    // 4. final schedule:
    //      the resulting gates are subject to final scheduling (the original resource-constrained scheduler)
    void MakeReal(ir::GateRef &gp, ir::Circuit &circ);

    // as mapper after-burner
    // make primitives of all gates that also have an entry with _prim appended to its name
    // and decomposing it according to the .json file gate decomposition
    void MakePrimitive(ir::GateRef &gp, ir::Circuit &circ) const;

    utils::UInt MaxFreeCycle() const;

    // nonq and q gates follow separate flows through Past:
    // - q gates are put in waitinglg when added and then scheduled; and then ordered by cycle into lg
    //      in lg they are waiting to be inspected and scheduled, until [too many are there,] a nonq comes or end-of-circuit
    // - nonq gates first cause lg to be flushed/cleared to output before the nonq gate is output
    // all gates in outlg are out of view for scheduling/mapping optimization and can be taken out to elsewhere
    void FlushAll();

    // gp as nonq gate immediately goes to outlg
    void ByPass(const ir::GateRef &gp);

    // mainPast flushes outlg to parameter oc
    void Out(ir::Circuit &oc);

};

} // namespace detail
} // namespace route
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
