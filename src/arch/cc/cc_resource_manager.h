/** \file
 *  Resource management for CC platform.
 *  Based on arch/cc_light/cc_light_resource_manager.h commit a95bc15c90ad17c2837adc2b3c36e031595e68d1
 */

#pragma once

#include "types_cc.h"

#include "resource_manager.h"
#include "utils/num.h"
#include "utils/str.h"
#include "utils/pair.h"
#include "utils/vec.h"
#include "utils/json.h"

#include <fstream>

namespace ql {
namespace arch {
namespace cc {

// ============ classes of resources that _may_ appear in a configuration file
// these are a superset of those allocated by the cc_resource_manager constructor below

// Each qubit can be used by only one gate at a time.
class cc_resource_qubit : public resource_t {
public:
    // fwd: qubit q is busy till cycle=cycle[q], i.e. all cycles < cycle[q] it is busy, i.e. start_cycle must be >= cycle[q]
    // bwd: qubit q is busy from cycle=cycle[q], i.e. all cycles >= cycle[q] it is busy, i.e. start_cycle+duration must be <= cycle[q]
    Vec<UInt> cycle;

    cc_resource_qubit(const quantum_platform &platform, scheduling_direction_t dir);

    cc_resource_qubit *clone() const & override;
    cc_resource_qubit *clone() && override;

    Bool available(UInt op_start_cycle, gate *ins, const quantum_platform &platform) override;
    void reserve(UInt op_start_cycle, gate *ins, const quantum_platform &platform) override;
};


// Single-qubit measurements (instructions of 'readout' type) are controlled by measurement units.
// Each one controls a private set of qubits.
// A measurement unit can control multiple qubits at the same time, but only when they start at the same time.
class cc_resource_meas : public resource_t {
public:
    Vec<UInt> fromcycle;  // last measurement start cycle
    Vec<UInt> tocycle;    // is busy till cycle
    Map<UInt,UInt> qubit2meas;

    cc_resource_meas(const quantum_platform & platform, scheduling_direction_t dir);

    cc_resource_meas *clone() const & override;
    cc_resource_meas *clone() && override;

    Bool available(UInt op_start_cycle, gate *ins, const quantum_platform &platform) override;
    void reserve(UInt op_start_cycle, gate *ins, const quantum_platform &platform) override;
};



#if 0   // FIXME: unused
// Two-qubit flux gates only operate on neighboring qubits, i.e. qubits connected by an edge.
// A two-qubit flux gate operates by lowering (detuning) the frequency of the operand qubit
// with the highest frequency to get close to the frequency of the other operand qubit.
// But any two qubits which have close frequencies execute a two-qubit flux gate:
// this may happen between the detuned frequency qubit and each of its other neighbors with a frequency close to this;
// to prevent this, those neighbors must have their frequency detuned (lowered out of the way, parked) as well.
// A parked qubit cannot engage in any gate, so also not a two-qubit gate.
// As a consequence, for each edge executing a two-qubit gate,
// certain other edges cannot execute a two-qubit gate in parallel.
class cc_edge_resource_t : public resource_t
{
public:
    // fwd: edge is busy till cycle=state[edge], i.e. all cycles < state[edge] it is busy, i.e. start_cycle must be >= state[edge]
    // bwd: edge is busy from cycle=state[edge], i.e. all cycles >= state[edge] it is busy, i.e. start_cycle+duration must be <= state[edge]
    Vec<UInt> state;                          // machine state recording the cycles that given edge is free/busy
    typedef Pair<UInt, UInt> qubits_pair_t;
    Map<qubits_pair_t, UInt> qubits2edge;      // constant helper table to find edge between a pair of qubits
    Map<UInt, Vec<UInt>> edge2edges;  // constant "edges" table from configuration file

    cc_edge_resource_t(const quantum_platform &platform, scheduling_direction_t dir);

    cc_edge_resource_t *clone() const & override;
    cc_edge_resource_t *clone() && override;

    Bool available(UInt op_start_cycle, gate *ins, const quantum_platform &platform) override;
    void reserve(UInt op_start_cycle, gate * ins, const quantum_platform & platform) override;
};

// A two-qubit flux gate lowers the frequency of its source qubit to get near the freq of its target qubit.
// Any two qubits which have near frequencies execute a two-qubit flux gate.
// To prevent any neighbor qubit of the source qubit that has the same frequency as the target qubit
// to interact as well, those neighbors must have their frequency detuned (lowered out of the way).
// A detuned qubit cannot execute a single-qubit rotation.
// An edge is a pair of qubits which can execute a two-qubit flux gate.
// The detuned_qubits resource describes for each edge doing a two-qubit gate which qubits it detunes.
//
// A two-qubit flux gate must check whether the qubits it would detune are not busy with a rotation.
// A one-qubit rotation gate must check whether its operand qubit is not detuned (busy with a flux gate).
//
// A two-qubit flux gate must set the qubits it would detune to detuned, busy with a flux gate.
// A one-qubit rotation gate must set its operand qubit to busy, busy with a rotation.
//
// The resource state machine maintains:
// - fromcycle[q]: qubit q is busy from cycle fromcycle[q]
// - tocycle[q]: to cycle tocycle[q] with an operation of the current operation type ...
// - operations[q]: a "flux" or a "mw" (note: "" is initial value different from these two)
// The fromcycle and tocycle are needed since a qubit can be busy with multiple "flux"s (i.e. being the detuned qubit for several "flux"s),
// so the second, third, etc. of these "flux"s can be scheduled in parallel to the first but not earlier than fromcycle[q],
// since till that cycle is was likely to be busy with "mw", which doesn't allow a "flux" in parallel. Similar for backward scheduling.
// The other members contain internal copies of the resource description and grid configuration of the json file.
class cc_detuned_qubits_resource_t : public resource_t {
public:
    Vec<UInt> fromcycle;                              // qubit q is busy from cycle fromcycle[q]
    Vec<UInt> tocycle;                                // till cycle tocycle[q]
    Vec<Str> operations;                        // with an operation of operation_type==operations[q]

    typedef Pair<UInt, UInt> qubits_pair_t;
    Map<qubits_pair_t, UInt> qubitpair2edge;           // map: pair of qubits to edge (from grid configuration)
    Map<UInt, Vec<UInt>> edge_detunes_qubits; // map: edge to vector of qubits that edge detunes (resource desc.)

    cc_detuned_qubits_resource_t(const quantum_platform &platform, scheduling_direction_t dir);

    cc_detuned_qubits_resource_t *clone() const & override;
    cc_detuned_qubits_resource_t *clone() && override;

    Bool available(UInt op_start_cycle, gate *ins, const quantum_platform &platform) override;
    void reserve(UInt op_start_cycle, gate *ins, const quantum_platform &platform) override;
};
#endif



// ============ platform specific resource_manager matching config file resources sections with resource classes above
// each config file resources section must have a resource class above
// not all resource classes above need to be actually used and specified in a config file; only those specified, are used

class cc_resource_manager : public platform_resource_manager_t {
public:
    cc_resource_manager() = default;

    // Allocate those resources that were specified in the config file.
    // Those that are not specified, are not allocated, so are not used in scheduling/mapping.
    // The resource names tested below correspond to the names of the resources sections in the config file.
    cc_resource_manager(const quantum_platform &platform, scheduling_direction_t dir);

    cc_resource_manager *clone() const & override;
    cc_resource_manager *clone() && override;
};

} // namespace cc
} // namespace arch
} // namespace ql
