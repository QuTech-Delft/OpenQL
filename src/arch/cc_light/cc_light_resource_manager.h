/** \file
 * Resource management for CC-light platform.
 */

#pragma once

#include <fstream>
#include "utils/num.h"
#include "utils/str.h"
#include "utils/pair.h"
#include "utils/vec.h"
#include "utils/json.h"
#include "resource_manager.h"

namespace ql {
namespace arch {


// ============ interfaces to access platform dependent attributes of a gate

// in configuration file, duration is in nanoseconds, while here we prefer it to have it in cycles
// it is needed to define the extend of the resource occupation in case of multi-cycle operations
utils::UInt ccl_get_operation_duration(gate *ins, const quantum_platform &platform);

// operation type is "mw" (for microwave), "flux", or "readout"
// it reflects the different resources used to implement the various gates and that resource management must distinguish
utils::Str ccl_get_operation_type(gate *ins, const quantum_platform &platform);

// operation name is used to know which operations are the same when one qwg steers several qubits using the vsm
utils::Str ccl_get_operation_name(gate *ins, const quantum_platform &platform);


// ============ classes of resources that _may_ appear in a configuration file
// these are a superset of those allocated by the cc_light_resource_manager_t constructor below

// Each qubit can be used by only one gate at a time.
class ccl_qubit_resource_t : public resource_t {
public:
    // fwd: qubit q is busy till cycle=state[q], i.e. all cycles < state[q] it is busy, i.e. start_cycle must be >= state[q]
    // bwd: qubit q is busy from cycle=state[q], i.e. all cycles >= state[q] it is busy, i.e. start_cycle+duration must be <= state[q]
    utils::Vec<utils::UInt> state;

    ccl_qubit_resource_t(const quantum_platform &platform, scheduling_direction_t dir);

    ccl_qubit_resource_t *clone() const & override;
    ccl_qubit_resource_t *clone() && override;

    utils::Bool available(utils::UInt op_start_cycle, gate *ins, const quantum_platform &platform) override;
    void reserve(utils::UInt op_start_cycle, gate *ins, const quantum_platform &platform) override;
};

// Single-qubit rotation gates (instructions of 'mw' type) are controlled by qwgs.
// Each qwg controls a private set of qubits.
// A qwg can control multiple qubits at the same time, but only when they perform the same gate and start at the same time.
class ccl_qwg_resource_t : public resource_t {
public:
    utils::Vec<utils::UInt> fromcycle;          // qwg is busy from cycle==fromcycle[qwg], inclusive
    utils::Vec<utils::UInt> tocycle;            // qwg is busy to cycle==tocycle[qwg], not inclusive

    // there was a bug here: when qwg is busy from cycle i with operation x
    // then a new x is ok when starting at i or later
    // but a new y must wait until the last x has finished;
    // the bug was that a new x was always ok (so also when starting earlier than cycle i)

    utils::Vec<utils::Str> operations;    // with operation_name==operations[qwg]
    utils::Map<utils::UInt,utils::UInt> qubit2qwg;      // on qwg==qubit2qwg[q]

    ccl_qwg_resource_t(const quantum_platform & platform, scheduling_direction_t dir);

    ccl_qwg_resource_t *clone() const & override;
    ccl_qwg_resource_t *clone() && override;

    utils::Bool available(utils::UInt op_start_cycle, gate *ins, const quantum_platform &platform) override;
    void reserve(utils::UInt op_start_cycle, gate *ins, const quantum_platform &platform) override;
};

// Single-qubit measurements (instructions of 'readout' type) are controlled by measurement units.
// Each one controls a private set of qubits.
// A measurement unit can control multiple qubits at the same time, but only when they start at the same time.
class ccl_meas_resource_t : public resource_t {
public:
    utils::Vec<utils::UInt> fromcycle;  // last measurement start cycle
    utils::Vec<utils::UInt> tocycle;    // is busy till cycle
    utils::Map<utils::UInt,utils::UInt> qubit2meas;

    ccl_meas_resource_t(const quantum_platform & platform, scheduling_direction_t dir);

    ccl_meas_resource_t *clone() const & override;
    ccl_meas_resource_t *clone() && override;

    utils::Bool available(utils::UInt op_start_cycle, gate *ins, const quantum_platform &platform) override;
    void reserve(utils::UInt op_start_cycle, gate *ins, const quantum_platform &platform) override;
};

// Two-qubit flux gates only operate on neighboring qubits, i.e. qubits connected by an edge.
// A two-qubit flux gate operates by lowering (detuning) the frequency of the operand qubit
// with the highest frequency to get close to the frequency of the other operand qubit.
// But any two qubits which have close frequencies execute a two-qubit flux gate:
// this may happen between the detuned frequency qubit and each of its other neighbors with a frequency close to this;
// to prevent this, those neighbors must have their frequency detuned (lowered out of the way, parked) as well.
// A parked qubit cannot engage in any gate, so also not a two-qubit gate.
// As a consequence, for each edge executing a two-qubit gate,
// certain other edges cannot execute a two-qubit gate in parallel.
class ccl_edge_resource_t : public resource_t
{
public:
    // fwd: edge is busy till cycle=state[edge], i.e. all cycles < state[edge] it is busy, i.e. start_cycle must be >= state[edge]
    // bwd: edge is busy from cycle=state[edge], i.e. all cycles >= state[edge] it is busy, i.e. start_cycle+duration must be <= state[edge]
    utils::Vec<utils::UInt> state;                          // machine state recording the cycles that given edge is free/busy
    typedef utils::Pair<utils::UInt, utils::UInt> qubits_pair_t;
    utils::Map<qubits_pair_t, utils::UInt> qubits2edge;      // constant helper table to find edge between a pair of qubits
    utils::Map<utils::UInt, utils::Vec<utils::UInt>> edge2edges;  // constant "edges" table from configuration file

    ccl_edge_resource_t(const quantum_platform &platform, scheduling_direction_t dir);

    ccl_edge_resource_t *clone() const & override;
    ccl_edge_resource_t *clone() && override;

    utils::Bool available(utils::UInt op_start_cycle, gate *ins, const quantum_platform &platform) override;
    void reserve(utils::UInt op_start_cycle, gate * ins, const quantum_platform & platform) override;
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
class ccl_detuned_qubits_resource_t : public resource_t {
public:
    utils::Vec<utils::UInt> fromcycle;                              // qubit q is busy from cycle fromcycle[q]
    utils::Vec<utils::UInt> tocycle;                                // till cycle tocycle[q]
    utils::Vec<utils::Str> operations;                        // with an operation of operation_type==operations[q]

    typedef utils::Pair<utils::UInt, utils::UInt> qubits_pair_t;
    utils::Map<qubits_pair_t, utils::UInt> qubitpair2edge;           // map: pair of qubits to edge (from grid configuration)
    utils::Map<utils::UInt, utils::Vec<utils::UInt>> edge_detunes_qubits; // map: edge to vector of qubits that edge detunes (resource desc.)

    ccl_detuned_qubits_resource_t(const quantum_platform &platform, scheduling_direction_t dir);

    ccl_detuned_qubits_resource_t *clone() const & override;
    ccl_detuned_qubits_resource_t *clone() && override;

    utils::Bool available(utils::UInt op_start_cycle, gate *ins, const quantum_platform &platform) override;
    void reserve(utils::UInt op_start_cycle, gate *ins, const quantum_platform &platform) override;
};

// ============ platform specific resource_manager matching config file resources sections with resource classes above
// each config file resources section must have a resource class above
// not all resource classes above need to be actually used and specified in a config file; only those specified, are used

class cc_light_resource_manager_t : public platform_resource_manager_t {
public:
    cc_light_resource_manager_t() = default;

    // Allocate those resources that were specified in the config file.
    // Those that are not specified, are not allocatd, so are not used in scheduling/mapping.
    // The resource names tested below correspond to the names of the resources sections in the config file.
    cc_light_resource_manager_t(const quantum_platform &platform, scheduling_direction_t dir);

    cc_light_resource_manager_t *clone() const & override;
    cc_light_resource_manager_t *clone() && override;
};

} // namespace arch
} // namespace ql
