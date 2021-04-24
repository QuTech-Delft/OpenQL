/** \file
 * Resource management for CC-light platform.
 *
 * FIXME: needs cleanup, generalization, and conversion to new resource types,
 *  including support for the "undefined" direction (since the mapper was
 *  apparently already using it)
 */

#pragma once

#include <fstream>
#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/pair.h"
#include "ql/utils/vec.h"
#include "ql/utils/json.h"
#include "ql/rmgr/resource_types/compat.h"

namespace ql {
namespace resource {
namespace todo {

// ============ classes of resources that _may_ appear in a configuration file
// these are a superset of those allocated by the cc_light_resource_manager_t constructor below

// Each qubit can be used by only one gate at a time.
class ccl_qubit_resource_t : public rmgr::resource_types::OldResource {
public:
    // fwd: qubit q is busy till cycle=state[q], i.e. all cycles < state[q] it is busy, i.e. start_cycle must be >= state[q]
    // bwd: qubit q is busy from cycle=state[q], i.e. all cycles >= state[q] it is busy, i.e. start_cycle+duration must be <= state[q]
    utils::Vec<utils::UInt> state;

    ccl_qubit_resource_t(const plat::PlatformRef &platform, rmgr::Direction dir);

    utils::Bool available(utils::UInt op_start_cycle, const ir::GateRef &ins, const plat::PlatformRef &platform) const override;
    void reserve(utils::UInt op_start_cycle, const ir::GateRef &ins, const plat::PlatformRef &platform) override;
};

// Single-qubit rotation gates (instructions of 'mw' type) are controlled by qwgs.
// Each qwg controls a private set of qubits.
// A qwg can control multiple qubits at the same time, but only when they perform the same gate and start at the same time.
class ccl_qwg_resource_t : public rmgr::resource_types::OldResource {
public:
    utils::Vec<utils::UInt> fromcycle;          // qwg is busy from cycle==fromcycle[qwg], inclusive
    utils::Vec<utils::UInt> tocycle;            // qwg is busy to cycle==tocycle[qwg], not inclusive

    // there was a bug here: when qwg is busy from cycle i with operation x
    // then a new x is ok when starting at i or later
    // but a new y must wait until the last x has finished;
    // the bug was that a new x was always ok (so also when starting earlier than cycle i)

    utils::Vec<utils::Str> operations;    // with operation_name==operations[qwg]
    utils::Map<utils::UInt,utils::UInt> qubit2qwg;      // on qwg==qubit2qwg[q]

    ccl_qwg_resource_t(const plat::PlatformRef &platform, rmgr::Direction dir);

    utils::Bool available(utils::UInt op_start_cycle, const ir::GateRef &ins, const plat::PlatformRef &platform) const override;
    void reserve(utils::UInt op_start_cycle, const ir::GateRef &ins, const plat::PlatformRef &platform) override;
};

// Single-qubit measurements (instructions of 'readout' type) are controlled by measurement units.
// Each one controls a private set of qubits.
// A measurement unit can control multiple qubits at the same time, but only when they start at the same time.
class ccl_meas_resource_t : public rmgr::resource_types::OldResource {
public:
    utils::Vec<utils::UInt> fromcycle;  // last measurement start cycle
    utils::Vec<utils::UInt> tocycle;    // is busy till cycle
    utils::Map<utils::UInt,utils::UInt> qubit2meas;

    ccl_meas_resource_t(const plat::PlatformRef & platform, rmgr::Direction dir);

    utils::Bool available(utils::UInt op_start_cycle, const ir::GateRef &ins, const plat::PlatformRef &platform) const override;
    void reserve(utils::UInt op_start_cycle, const ir::GateRef &ins, const plat::PlatformRef &platform) override;
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
class ccl_edge_resource_t : public rmgr::resource_types::OldResource {
public:
    // fwd: edge is busy till cycle=state[edge], i.e. all cycles < state[edge] it is busy, i.e. start_cycle must be >= state[edge]
    // bwd: edge is busy from cycle=state[edge], i.e. all cycles >= state[edge] it is busy, i.e. start_cycle+duration must be <= state[edge]
    utils::Vec<utils::UInt> state;                          // machine state recording the cycles that given edge is free/busy
    typedef utils::Pair<utils::UInt, utils::UInt> qubits_pair_t;
    utils::Map<qubits_pair_t, utils::UInt> qubits2edge;      // constant helper table to find edge between a pair of qubits
    utils::Map<utils::UInt, utils::Vec<utils::UInt>> edge2edges;  // constant "edges" table from configuration file

    ccl_edge_resource_t(const plat::PlatformRef &platform, rmgr::Direction dir);

    utils::Bool available(utils::UInt op_start_cycle, const ir::GateRef &ins, const plat::PlatformRef &platform) const override;
    void reserve(utils::UInt op_start_cycle, const ir::GateRef &ins, const plat::PlatformRef &platform) override;
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
class ccl_detuned_qubits_resource_t : public rmgr::resource_types::OldResource {
public:
    utils::Vec<utils::UInt> fromcycle;                              // qubit q is busy from cycle fromcycle[q]
    utils::Vec<utils::UInt> tocycle;                                // till cycle tocycle[q]
    utils::Vec<utils::Str> operations;                        // with an operation of operation_type==operations[q]

    typedef utils::Pair<utils::UInt, utils::UInt> qubits_pair_t;
    utils::Map<qubits_pair_t, utils::UInt> qubitpair2edge;           // map: pair of qubits to edge (from grid configuration)
    utils::Map<utils::UInt, utils::Vec<utils::UInt>> edge_detunes_qubits; // map: edge to vector of qubits that edge detunes (resource desc.)

    ccl_detuned_qubits_resource_t(const plat::PlatformRef &platform, rmgr::Direction dir);

    utils::Bool available(utils::UInt op_start_cycle, const ir::GateRef &ins, const plat::PlatformRef &platform) const override;
    void reserve(utils::UInt op_start_cycle, const ir::GateRef &ins, const plat::PlatformRef &platform) override;
};

// Inter-core communication gates use channels between cores.
// Each inter-core communication gate uses two channels, one at each side of the communication,
// to establish a connection between the cores, and especially between the qubits that are operands of the gate.
// Each of these two channels must remain allocated to the connection, while the communication is ongoing.
// When the communication gate is ready, the two channels can be released.
// Of those channels only a limited and configurable number may be available for each core.
// While during scheduling of a communication gate, no such channels are available, the gate is delayed.
class ccl_channel_resource_t : public rmgr::resource_types::OldResource {
public:
    utils::UInt ncores;                             // topology.number_of_cores: total number of cores
    utils::UInt nchannels;                          // resources.channels.count: number of channels in each core
    // fwd: channel c is busy till cycle=state[core][c],
    // i.e. all cycles < state[core][c] it is busy, i.e. start_cycle must be >= state[core][c]
    // bwd: channel c is busy from cycle=state[core][c],
    // i.e. all cycles >= state[core][c] it is busy, i.e. start_cycle+duration must be <= state[core][c]
    utils::Vec<utils::Vec<utils::UInt>> state;

    ccl_channel_resource_t(const plat::PlatformRef &platform, rmgr::Direction dir);

    utils::Bool available(utils::UInt op_start_cycle, const ir::GateRef &ins, const plat::PlatformRef &platform) const override;
    void reserve(utils::UInt op_start_cycle, const ir::GateRef &ins, const plat::PlatformRef &platform) override;
};

} // namespace todo

using QWGs = rmgr::resource_types::Compat<todo::ccl_qwg_resource_t>;
using MeasUnits = rmgr::resource_types::Compat<todo::ccl_meas_resource_t>;
using Edges = rmgr::resource_types::Compat<todo::ccl_edge_resource_t>;
using DetunedQubits = rmgr::resource_types::Compat<todo::ccl_detuned_qubits_resource_t>;
using Channels = rmgr::resource_types::Compat<todo::ccl_channel_resource_t>;

} // namespace resource
} // namespace ql
