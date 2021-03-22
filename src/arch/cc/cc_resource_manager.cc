/** \file
 *resource management for CC platform.
 */

#include "cc_resource_manager.h"

#include "utils/json.h"

namespace ql {
namespace arch {
namespace cc {

using namespace utils;

#if 0
// operation type is "mw" (for microwave), "flux", or "readout"
// it reflects the different resources used to implement the various gates and that resource management must distinguish
Str cc_get_operation_type(gate *ins, const quantum_platform &platform) {
    return platform.find_instruction_type(ins->name);
}
#endif

// operation name is used to know which operations are the same when one qwg steers several qubits using the vsm
Str cc_get_operation_name(gate *ins, const quantum_platform &platform) {
    Str operation_name(ins->name);
    QL_JSON_ASSERT(platform.instruction_settings, ins->name, ins->name);
    if (!platform.instruction_settings[ins->name]["cc_instr"].is_null()) {
        operation_name = platform.instruction_settings[ins->name]["cc_instr"].get<Str>();
    }
    return operation_name;
}

/************************************************************************\
| cc_resource_qubit
\************************************************************************/

cc_resource_qubit::cc_resource_qubit(
    const quantum_platform &platform,
    scheduling_direction_t dir
) :
    resource_t("qubits", dir)
{
    count = json_get<UInt>(platform.hardware_settings, "qubit_number", "hardware_settings/qubit_number");
    state.resize(count);
    for (UInt q = 0; q < count; q++) {
        state[q] = (forward_scheduling == dir ? 0 : MAX_CYCLE);
    }
}

cc_resource_qubit *cc_resource_qubit::clone() const & {
    QL_DOUT("Cloning/copying cc_resource_qubit");
    return new cc_resource_qubit(*this);
}

cc_resource_qubit *cc_resource_qubit::clone() && {
    QL_DOUT("Cloning/moving cc_resource_qubit");
    return new cc_resource_qubit(std::move(*this));
}

Bool cc_resource_qubit::available(
    UInt op_start_cycle,
    gate *ins,
    const quantum_platform &platform
) {
    UInt operation_duration = platform.time_to_cycles(ins->duration);

    for (auto q : ins->operands) {
        if (forward_scheduling == direction) {
            QL_DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle << "  qubit: " << q << " is busy till cycle : " << state[q]);
            if (op_start_cycle < state[q]) {
                QL_DOUT("    " << name << " resource busy ...");
                return false;
            }
        } else {
            QL_DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle << "  qubit: " << q << " is busy from cycle : " << state[q]);
            if (op_start_cycle + operation_duration > state[q]) {
                QL_DOUT("    " << name << " resource busy ...");
                return false;
            }
        }
    }
    QL_DOUT("    " << name << " resource available ...");
    return true;
}

void cc_resource_qubit::reserve(
    UInt op_start_cycle,
    gate *ins,
    const quantum_platform &platform
) {
    UInt operation_duration = platform.time_to_cycles(ins->duration);

    for (auto q : ins->operands) {
        state[q] = (forward_scheduling == direction ?  op_start_cycle + operation_duration : op_start_cycle );
        QL_DOUT("reserved " << name << ". op_start_cycle: " << op_start_cycle << " qubit: " << q << " reserved till/from cycle: " << state[q]);
    }
}


#if 0   // FIXME: unused
/************************************************************************\
|
\************************************************************************/

cc_qwg_resource_t::cc_qwg_resource_t(
    const quantum_platform &platform,
    scheduling_direction_t dir
) :
    resource_t("qwgs", dir)
{
    // DOUT("... creating " << name << " resource");
    count = platform.resources[name]["count"];
    fromcycle.resize(count);
    tocycle.resize(count);
    operations.resize(count);

    for (UInt i = 0; i < count; i++) {
        fromcycle[i] = (forward_scheduling == dir ? 0 : MAX_CYCLE);
        tocycle[i] = (forward_scheduling == dir ? 0 : MAX_CYCLE);
        operations[i] = "";
    }
    auto & constraints = platform.resources[name]["connection_map"];
    for (auto it = constraints.cbegin(); it != constraints.cend(); ++it) {
        // COUT(it.key() << " : " << it.value() );
        UInt qwgNo = stoi( it.key() );
        auto & connected_qubits = it.value();
        for (auto &q : connected_qubits) {
            qubit2qwg.set(q) = qwgNo;
        }
    }
}

cc_qwg_resource_t *cc_qwg_resource_t::clone() const & {
    QL_DOUT("Cloning/copying cc_qwg_resource_t");
    return new cc_qwg_resource_t(*this);
}

cc_qwg_resource_t *cc_qwg_resource_t::clone() && {
    QL_DOUT("Cloning/moving cc_qwg_resource_t");
    return new cc_qwg_resource_t(std::move(*this));
}

Bool cc_qwg_resource_t::available(
    UInt op_start_cycle,
    gate *ins,
    const quantum_platform &platform
) {
    Str operation_type = platform.find_instruction_type(ins->name);
    Str operation_name = cc_get_operation_name(ins, platform);
    UInt      operation_duration = cc_get_operation_duration(ins, platform);

    Bool is_mw = (operation_type == "mw");
    if (is_mw) {
        for (auto q : ins->operands) {
            QL_DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle << "  qwg: " << qubit2qwg.at(q) << " is busy from cycle: " << fromcycle[qubit2qwg.at(q)] << " to cycle: " << tocycle[qubit2qwg.at(q)] << " for operation: " << operations[qubit2qwg.at(q)]);
            if (direction == forward_scheduling) {
                if (
                    op_start_cycle < fromcycle[qubit2qwg.at(q)]
                    || (op_start_cycle < tocycle[qubit2qwg.at(q)] && operations[qubit2qwg.at(q)] != operation_name)
                ) {
                    QL_DOUT("    " << name << " resource busy ...");
                    return false;
                }
            } else {
                if (
                    op_start_cycle + operation_duration > tocycle[qubit2qwg.at(q)]
                    || ( op_start_cycle + operation_duration > fromcycle[qubit2qwg.at(q)] && operations[qubit2qwg.at(q)] != operation_name)
                ) {
                    QL_DOUT("    " << name << " resource busy ...");
                    return false;
                }
            }
        }
        QL_DOUT("    " << name << " resource available ...");
    }
    return true;
}

void cc_qwg_resource_t::reserve(
    UInt op_start_cycle,
    gate *ins,
    const quantum_platform &platform
) {
    Str operation_type = platform.find_instruction_type(ins->name);
    Str operation_name = cc_get_operation_name(ins, platform);
    UInt      operation_duration = cc_get_operation_duration(ins, platform);

    Bool is_mw = operation_type == "mw";
    if (is_mw) {
        for (auto q : ins->operands) {
            if (direction == forward_scheduling) {
                if (operations[qubit2qwg.at(q)] == operation_name) {
                    tocycle[qubit2qwg.at(q)] = max(tocycle[qubit2qwg.at(q)], op_start_cycle + operation_duration);
                } else {
                    fromcycle[qubit2qwg.at(q)] = op_start_cycle;
                    tocycle[qubit2qwg.at(q)] = op_start_cycle + operation_duration;
                    operations[qubit2qwg.at(q)] = operation_name;
                }
            } else {
                if (operations[qubit2qwg.at(q)] == operation_name) {
                    fromcycle[qubit2qwg.at(q)] = min(fromcycle[qubit2qwg.at(q)], op_start_cycle);
                } else {
                    fromcycle[qubit2qwg.at(q)] = op_start_cycle;
                    tocycle[qubit2qwg.at(q)] = op_start_cycle + operation_duration;
                    operations[qubit2qwg.at(q)] = operation_name;
                }
            }
            QL_DOUT("reserved " << name << ". op_start_cycle: " << op_start_cycle << " qwg: " << qubit2qwg.at(q) << " reserved from cycle: " << fromcycle[qubit2qwg.at(q)] << " to cycle: " << tocycle[qubit2qwg.at(q)] << " for operation: " << operations[qubit2qwg.at(q)]);
        }
    }
}
#endif


/************************************************************************\
| cc_resource_meas
\************************************************************************/

cc_resource_meas::cc_resource_meas(
    const quantum_platform &platform,
    scheduling_direction_t dir
) :
    resource_t("meas_units", dir)   // FIXME
{
    count = platform.resources[name]["count"];
    fromcycle.resize(count);
    tocycle.resize(count);

    for (UInt i=0; i<count; i++) {
        fromcycle[i] = (forward_scheduling == dir ? 0 : MAX_CYCLE);
        tocycle[i] = (forward_scheduling == dir ? 0 : MAX_CYCLE);
    }
    auto &constraints = platform.resources[name]["connection_map"];
    for (auto it = constraints.begin(); it != constraints.end(); ++it) {
        UInt measUnitNo = stoi( it.key() );
        auto & connected_qubits = it.value();
        for (auto &q : connected_qubits) {
            qubit2meas.set(q) = measUnitNo;
        }
    }
}

cc_resource_meas *cc_resource_meas::clone() const & {
    QL_DOUT("Cloning/copying cc_resource_meas");
    return new cc_resource_meas(*this);
}

cc_resource_meas *cc_resource_meas::clone() && {
    QL_DOUT("Cloning/moving cc_resource_meas");
    return new cc_resource_meas(std::move(*this));
}

Bool cc_resource_meas::available(
    UInt op_start_cycle,
    gate *ins,
    const quantum_platform &platform
) {
    Str operation_type = platform.find_instruction_type(ins->name);
    UInt operation_duration = platform.time_to_cycles(ins->duration);

    Bool is_measure = (operation_type == "readout");
    if (is_measure) {
        for (auto q : ins->operands) {
            QL_DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle << "  meas: " << qubit2meas.at(q) << " is busy from cycle: " << fromcycle[qubit2meas.at(q)] << " to cycle: " << tocycle[qubit2meas.at(q)] );
            if (direction == forward_scheduling) {
                if (op_start_cycle != fromcycle[qubit2meas.at(q)]) {
                    // If current measurement on same measurement-unit does not start in the
                    // same cycle, then it should wait for current measurement to finish
                    if (op_start_cycle < tocycle[qubit2meas.at(q)]) {
                        QL_DOUT("    " << name << " resource busy ...");
                        return false;
                    }
                }
            } else {
                if (op_start_cycle != fromcycle[qubit2meas.at(q)]) {
                    // If current measurement on same measurement-unit does not start in the
                    // same cycle, then it should wait until it would finish at start of or earlier than current measurement
                    if (op_start_cycle + operation_duration > fromcycle[qubit2meas.at(q)]) {
                        QL_DOUT("    " << name << " resource busy ...");
                        return false;
                    }
                }
            }
        }
        QL_DOUT("    " << name << " resource available ...");
    }
    return true;
}

void cc_resource_meas::reserve(
    UInt op_start_cycle,
    gate *ins,
    const quantum_platform &platform
) {
    Str operation_type = platform.find_instruction_type(ins->name);
    UInt operation_duration = platform.time_to_cycles(ins->duration);

    Bool is_measure = (operation_type == "readout");
    if (is_measure) {
        for (auto q : ins->operands) {
            fromcycle[qubit2meas.at(q)] = op_start_cycle;
            tocycle[qubit2meas.at(q)] = op_start_cycle + operation_duration;
            QL_DOUT("reserved " << name << ". op_start_cycle: " << op_start_cycle << " meas: " << qubit2meas.at(q) << " reserved from cycle: " << fromcycle[qubit2meas.at(q)] << " to cycle: " << tocycle[qubit2meas.at(q)]);
        }
    }
}


#if 0   // FIXME: unused
/************************************************************************\
|
\************************************************************************/

cc_edge_resource_t::cc_edge_resource_t(
    const quantum_platform &platform,
    scheduling_direction_t dir
) :
    resource_t("edges", dir)
{
    // DOUT("... creating " << name << " resource");
    count = platform.resources[name]["count"];
    state.resize(count);

    for (UInt i = 0; i < count; i++) {
        state[i] = (forward_scheduling == dir ? 0 : MAX_CYCLE);
    }

    for (auto &anedge : platform.topology["edges"]) {
        UInt s = anedge["src"];
        UInt d = anedge["dst"];
        UInt e = anedge["id"];

        qubits_pair_t aqpair(s,d);
        auto it = qubits2edge.find(aqpair);
        if (it != qubits2edge.end()) {
            QL_EOUT("re-defining edge " << s << "->" << d << " !");
            throw Exception("[x] Error : re-defining edge !", false);
        } else {
            qubits2edge.set(aqpair) = e;
        }
    }

    auto &constraints = platform.resources[name]["connection_map"];
    for (auto it = constraints.cbegin(); it != constraints.cend(); ++it) {
        // COUT(it.key() << " : " << it.value() << "\n");
        UInt edgeNo = stoi( it.key() );
        auto &connected_edges = it.value();
        for (auto &e : connected_edges) {
            edge2edges.set(e).push_back(edgeNo);
        }
    }
}

cc_edge_resource_t *cc_edge_resource_t::clone() const & {
    QL_DOUT("Cloning/copying cc_edge_resource_t");
    return new cc_edge_resource_t(*this);
}

cc_edge_resource_t *cc_edge_resource_t::clone() && {
    QL_DOUT("Cloning/moving cc_edge_resource_t");
    return new cc_edge_resource_t(std::move(*this));
}

Bool cc_edge_resource_t::available(
    UInt op_start_cycle,
    gate *ins,
    const quantum_platform &platform
) {
    Str operation_type = platform.find_instruction_type(ins->name);
    UInt operation_duration = platform.time_to_cycles(ins->duration);

    auto gname = ins->name;
    Bool is_flux = (operation_type == "flux");
    if (is_flux) {
        auto nopers = ins->operands.size();
        if (nopers == 1) {
            // single qubit flux operation does not reserve an edge resource
            QL_DOUT(" available for single qubit flux operation: " << name);
        } else if (nopers == 2) {
            auto q0 = ins->operands[0];
            auto q1 = ins->operands[1];
            qubits_pair_t aqpair(q0, q1);
            auto it = qubits2edge.find(aqpair);
            if (it != qubits2edge.end()) {
                auto edge_no = qubits2edge.at(aqpair);

                QL_DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle << ", edge: " << edge_no << " is busy till/from cycle : " << state[edge_no] << " for operation: " << ins->name);

                Vec<UInt> edges2check(edge2edges.get(edge_no));
                edges2check.push_back(edge_no);
                for (auto &e : edges2check) {
                    if (direction == forward_scheduling) {
                        if (op_start_cycle < state[e]) {
                            QL_DOUT("    " << name << " resource busy ...");
                            return false;
                        }
                    } else {
                        if (op_start_cycle + operation_duration > state[e]) {
                            QL_DOUT("    " << name << " resource busy ...");
                            return false;
                        }
                    }
                }
                QL_DOUT("    " << name << " resource available ...");
            } else {
                QL_FATAL("Use of illegal edge: " << q0 << "->" << q1 << " in operation: " << ins->name << " !");
            }
        } else {
            QL_FATAL("Incorrect number of operands used in operation: " << ins->name << " !");
        }
    }
    return true;
}

void cc_edge_resource_t::reserve(
    UInt op_start_cycle,
    gate *ins,
    const quantum_platform &platform
) {
    Str operation_type = platform.find_instruction_type(ins->name);
    UInt operation_duration = platform.time_to_cycles(ins->duration);

    auto gname = ins->name;
    Bool is_flux = (operation_type == "flux");
    if (is_flux) {
        auto nopers = ins->operands.size();
        if (nopers == 1) {
            // single qubit flux operation does not reserve an edge resource
        } else if (nopers == 2) {
            auto q0 = ins->operands[0];
            auto q1 = ins->operands[1];
            qubits_pair_t aqpair(q0, q1);
            auto edge_no = qubits2edge.at(aqpair);
            if (direction == forward_scheduling) {
                state[edge_no] = op_start_cycle + operation_duration;
                for (auto &e : edge2edges.get(edge_no)) {
                    state[e] = op_start_cycle + operation_duration;
                }
            } else {
                state[edge_no] = op_start_cycle;
                for (auto &e : edge2edges.get(edge_no)) {
                    state[e] = op_start_cycle;
                }
            }
            QL_DOUT("reserved " << name << ". op_start_cycle: " << op_start_cycle << " edge: " << edge_no << " reserved till cycle: " << state[ edge_no ] << " for operation: " << ins->name);
        } else {
            QL_FATAL("Incorrect number of operands used in operation: " << ins->name << " !");
        }
    }
}

/************************************************************************\
|
\************************************************************************/

cc_detuned_qubits_resource_t::cc_detuned_qubits_resource_t(
    const quantum_platform &platform,
    scheduling_direction_t dir
) :
    resource_t("detuned_qubits", dir)
{
    // DOUT("... creating " << name << " resource");
    count = platform.resources[name]["count"];
    fromcycle.resize(count);
    tocycle.resize(count);
    operations.resize(count);

    // initialize resource state machine to be free for all qubits
    for (UInt i = 0; i < count; i++) {
        fromcycle[i] = (forward_scheduling == dir ? 0 : MAX_CYCLE);
        tocycle[i] = (forward_scheduling == dir ? 0 : MAX_CYCLE);
        operations[i] = "";
    }

    // initialize qubitpair2edge map from json description; this is a constant map
    for (auto &anedge : platform.topology["edges"]) {
        UInt s = anedge["src"];
        UInt d = anedge["dst"];
        UInt e = anedge["id"];

        qubits_pair_t aqpair(s,d);
        auto it = qubitpair2edge.find(aqpair);
        if (it != qubitpair2edge.end()) {
            QL_EOUT("re-defining edge " << s << "->" << d << " !");
            throw Exception("[x] Error : re-defining edge !", false);
        } else {
            qubitpair2edge.set(aqpair) = e;
        }
    }

    // initialize edge_detunes_qubits map from json description; this is a constant map
    auto &constraints = platform.resources[name]["connection_map"];
    for (auto it = constraints.cbegin(); it != constraints.cend(); ++it) {
        // COUT(it.key() << " : " << it.value() << "\n");
        UInt edgeNo = stoi( it.key() );
        auto & detuned_qubits = it.value();
        for (auto & q : detuned_qubits) {
            edge_detunes_qubits.set(edgeNo).push_back(q);
        }
    }
}

cc_detuned_qubits_resource_t *cc_detuned_qubits_resource_t::clone() const & {
    QL_DOUT("Cloning/copying cc_detuned_qubits_resource_t");
    return new cc_detuned_qubits_resource_t(*this);
}

cc_detuned_qubits_resource_t *cc_detuned_qubits_resource_t::clone() && {
    QL_DOUT("Cloning/moving cc_detuned_qubits_resource_t");
    return new cc_detuned_qubits_resource_t(std::move(*this));
}

// When a two-qubit flux gate, check whether the qubits it would detune are not busy with a rotation.
// When a one-qubit rotation, check whether the qubit is not detuned (busy with a flux gate).
Bool cc_detuned_qubits_resource_t::available(
    UInt op_start_cycle,
    gate *ins,
    const quantum_platform &platform
) {
    Str operation_type = platform.find_instruction_type(ins->name);
    UInt      operation_duration = cc_get_operation_duration(ins, platform);

    auto gname = ins->name;
    Bool is_flux = operation_type == "flux";
    if (is_flux) {
        auto nopers = ins->operands.size();
        if (nopers == 1) {
            // single qubit flux operation does not reserve a detuned qubits resource
            QL_DOUT(" available for single qubit flux operation: " << name);
        } else if (nopers == 2) {
            auto q0 = ins->operands[0];
            auto q1 = ins->operands[1];
            qubits_pair_t aqpair(q0, q1);
            auto it = qubitpair2edge.find(aqpair);
            if (it != qubitpair2edge.end()) {
                auto edge_no = qubitpair2edge.at(aqpair);

                for (auto &q : edge_detunes_qubits.get(edge_no)) {
                    QL_DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle << ", edge: " << edge_no << " detuning qubit: " << q << " for operation: " << ins->name << " busy from: " << fromcycle[q] << " till: " << tocycle[q] << " with operation_type: " << operation_type);
                    if (direction == forward_scheduling) {
                        if (
                            op_start_cycle < fromcycle[q]
                            || ( op_start_cycle < tocycle[q] && operations[q] != operation_type)
                        ) {
                            QL_DOUT("    " << name << " resource busy for a two-qubit gate...");
                            return false;
                        }
                    } else {
                        if (
                            op_start_cycle + operation_duration > tocycle[q]
                            || ( op_start_cycle + operation_duration > fromcycle[q] && operations[q] != operation_type)
                        ) {
                            QL_DOUT("    " << name << " resource busy for a two-qubit gate...");
                            return false;
                        }
                    }
                }	// for over edges
            } else { // no edge found
                QL_EOUT("Use of illegal edge: " << q0 << "->" << q1 << " in operation: " << ins->name << " !");
                throw Exception("[x] Error : Use of illegal edge" + to_string(q0) + "->" + to_string(q1) + "in operation:" + ins->name + " !", false);
            }
        } else { // nopers != 1 or 2
            QL_FATAL("Incorrect number of operands used in operation: " << ins->name << " !");
        }
    }

    Bool is_mw = operation_type == "mw";
    if (is_mw) {
        for (auto q : ins->operands) {
            QL_DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle << ", qubit: " << q << " for operation: " << ins->name << " busy from: " << fromcycle[q] << " till: " << tocycle[q] << " with operation_type: " << operation_type);
            if (direction == forward_scheduling) {
                if (op_start_cycle < fromcycle[q]) {
                    QL_DOUT("    " << name << " busy for rotation: op_start cycle " << op_start_cycle << " < fromcycle[" << q << "] " << fromcycle[q] );
                    return false;
                }
                if (op_start_cycle < tocycle[q] && operations[q] != operation_type) {
                    QL_DOUT("    " << name << " busy for rotation with flux: op_start cycle " << op_start_cycle << " < tocycle[" << q << "] " << tocycle[q] );
                    return false;
                }
            } else {
                if (op_start_cycle + operation_duration > tocycle[q]) {
                    QL_DOUT("    " << name << " busy for rotation: op_start cycle " << op_start_cycle << " + duration > tocycle[" << q << "] " << tocycle[q] );
                    return false;
                }
                if (op_start_cycle + operation_duration > fromcycle[q] && operations[q] != operation_type) {
                    QL_DOUT("    " << name << " busy for rotation with flux: op_start cycle " << op_start_cycle << " + duration > fromcycle[" << q << "] " << fromcycle[q] );
                    return false;
                }
            }
        }
    }
    if (is_flux || is_mw) QL_DOUT("    " << name << " resource available ...");
    return true;
}

// A two-qubit flux gate must set the qubits it would detune to detuned, busy with a flux gate.
// A one-qubit rotation gate must set its operand qubit to busy, busy with a rotation.
void cc_detuned_qubits_resource_t::reserve(
    UInt op_start_cycle,
    gate *ins,
    const quantum_platform &platform
) {
    Str operation_type = platform.find_instruction_type(ins->name);
    UInt      operation_duration = cc_get_operation_duration(ins, platform);

    auto gname = ins->name;
    Bool is_flux = operation_type == "flux";
    if (is_flux) {
        auto nopers = ins->operands.size();
        if (nopers == 1) {
            // single qubit flux operation does not reserve a detuned qubits resource
        } else if (nopers == 2) {
            auto q0 = ins->operands[0];
            auto q1 = ins->operands[1];
            qubits_pair_t aqpair(q0, q1);
            auto edge_no = qubitpair2edge.at(aqpair);

            for (auto &q : edge_detunes_qubits.get(edge_no)) {
                if (direction == forward_scheduling) {
                    if (operations[q] == operation_type) {
                        tocycle[q] = max(tocycle[q], op_start_cycle + operation_duration);
                        QL_DOUT("reserving " << name << ". for qubit: " << q << " reusing cycle: " << fromcycle[q] << " to extending tocycle: " << tocycle[q] << " for old operation: " << ins->name);
                    } else {
                        fromcycle[q] = op_start_cycle;
                        tocycle[q] = op_start_cycle + operation_duration;
                        operations[q] = operation_type;
                        QL_DOUT("reserving " << name << ". for qubit: " << q << " from fromcycle: " << fromcycle[q] << " to new tocycle: " << tocycle[q] << " for new operation: " << ins->name);
                    }
                } else {
                    if (operations[q] == operation_type) {
                        fromcycle[q] = min(fromcycle[q], op_start_cycle);
                        QL_DOUT("reserving " << name << ". for qubit: " << q << " from extended cycle: " << fromcycle[q] << " reusing tocycle: " << tocycle[q] << " for old operation: " << ins->name);
                    } else {
                        fromcycle[q] = op_start_cycle;
                        tocycle[q] = op_start_cycle + operation_duration;
                        operations[q] = operation_type;
                        QL_DOUT("reserving " << name << ". for qubit: " << q << " from new cycle: " << fromcycle[q] << " to tocycle: " << tocycle[q] << " for new operation: " << ins->name);
                    }
                }
                QL_DOUT("reserved " << name << ". op_start_cycle: " << op_start_cycle << " edge: " << edge_no << " detunes qubit: " << q << " reserved from cycle: " << fromcycle[q] << " till cycle: " << tocycle[q] << " for operation: " << ins->name);
            }
        } else {
            QL_FATAL("Incorrect number of operands used in operation: " << ins->name << " !");
        }
    }
    Bool is_mw = operation_type == "mw";
    if (is_mw) {
        for (auto q : ins->operands) {
            if (direction == forward_scheduling) {
                if (operations[q] == operation_type) {
                    tocycle[q] = max(tocycle[q], op_start_cycle + operation_duration);
                    QL_DOUT("reserving " << name << ". for qubit: " << q << " reusing cycle: " << fromcycle[q] << " to extending tocycle: " << tocycle[q] << " for old operation: " << ins->name);
                } else {
                    fromcycle[q] = op_start_cycle;
                    tocycle[q] = op_start_cycle + operation_duration;
                    operations[q] = operation_type;
                    QL_DOUT("reserving " << name << ". for qubit: " << q << " from fromcycle: " << fromcycle[q] << " to new tocycle: " << tocycle[q] << " for new operation: " << ins->name);
                }
            } else {
                if (operations[q] == operation_type) {
                    fromcycle[q] = min(fromcycle[q], op_start_cycle);
                    QL_DOUT("reserving " << name << ". for qubit: " << q << " from extended cycle: " << fromcycle[q] << " reusing tocycle: " << tocycle[q] << " for old operation: " << ins->name);
                } else {
                    fromcycle[q] = op_start_cycle;
                    tocycle[q] = op_start_cycle + operation_duration;
                    operations[q] = operation_type;
                    QL_DOUT("reserving " << name << ". for qubit: " << q << " from new cycle: " << fromcycle[q] << " to tocycle: " << tocycle[q] << " for new operation: " << ins->name);
                }
            }
            QL_DOUT("... reserved " << name << ". op_start_cycle: " << op_start_cycle << " for qubit: " << q << " reserved from cycle: " << fromcycle[q] << " till cycle: " << tocycle[q] << " for operation: " << ins->name);
        }
    }
}
#endif



/************************************************************************\
| cc_resource_manager_t
\************************************************************************/

// Allocate those resources that were specified in the config file.
// Those that are not specified, are not allocatd, so are not used in scheduling/mapping.
// The resource names tested below correspond to the names of the resources sections in the config file.
cc_resource_manager_t::cc_resource_manager_t(
    const quantum_platform &platform,
    scheduling_direction_t dir
) :
    platform_resource_manager_t(platform, dir)
{
    QL_DOUT("Constructing (platform,dir) parameterized platform_resource_manager_t");
    QL_DOUT("New one for direction " << dir << " with no of resources : " << platform.resources.size() );
    for (auto it = platform.resources.cbegin(); it != platform.resources.cend(); ++it) {
        Str key = it.key();

        if (key == "qubits") {
            resource_t *res = new cc_resource_qubit(platform, dir);
            resource_ptrs.push_back(res);
#if 0
        } else if (key == "qwgs") {
            resource_t *res = new cc_qwg_resource_t(platform, dir);
            resource_ptrs.push_back(res);
#endif
        } else if (key == "meas_units") {
            resource_t *res = new cc_resource_meas(platform, dir);
            resource_ptrs.push_back(res);
#if 0
        } else if (key == "edges") {
            resource_t *res = new cc_edge_resource_t(platform, dir);
            resource_ptrs.push_back(res);
        } else if (key == "detuned_qubits") {
            resource_t *res = new cc_detuned_qubits_resource_t(platform, dir);
            resource_ptrs.push_back(res);
#endif
        } else {
            QL_JSON_FATAL("resource key '" << key << "' not supported");
        }
    }
}

cc_resource_manager_t *cc_resource_manager_t::clone() const & {
    QL_DOUT("Cloning/copying cc_resource_manager_t");
    return new cc_resource_manager_t(*this);
}

cc_resource_manager_t *cc_resource_manager_t::clone() && {
    QL_DOUT("Cloning/moving cc_resource_manager_t");
    return new cc_resource_manager_t(std::move(*this));
}

} // namespace cc
} // namespace arch
} // namespace ql
