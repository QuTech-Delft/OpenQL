/** \file
 *  resource management for CC platform.
 *  Based on arch/cc_light/cc_light_resource_manager.cc commit a95bc15c90ad17c2837adc2b3c36e031595e68d1
 */

#include "cc_resource_manager.h"
#include "settings_cc.h"

#include "resources.h"

#include "utils/json.h"

namespace ql {
namespace arch {
namespace cc {

using namespace utils;

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
    cycle.resize(count);

    for (UInt i = 0; i < count; i++) {
        cycle[i] = (forward_scheduling == dir ? 0 : MAX_CYCLE);
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

                QL_DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle << ", edge: " << edge_no << " is busy till/from cycle : " << cycle[edge_no] << " for operation: " << ins->name);

                Vec<UInt> edges2check(edge2edges.get(edge_no));
                edges2check.push_back(edge_no);
                for (auto &e : edges2check) {
                    if (direction == forward_scheduling) {
                        if (op_start_cycle < cycle[e]) {
                            QL_DOUT("    " << name << " resource busy ...");
                            return false;
                        }
                    } else {
                        if (op_start_cycle + operation_duration > cycle[e]) {
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
                cycle[edge_no] = op_start_cycle + operation_duration;
                for (auto &e : edge2edges.get(edge_no)) {
                    cycle[e] = op_start_cycle + operation_duration;
                }
            } else {
                cycle[edge_no] = op_start_cycle;
                for (auto &e : edge2edges.get(edge_no)) {
                    cycle[e] = op_start_cycle;
                }
            }
            QL_DOUT("reserved " << name << ". op_start_cycle: " << op_start_cycle << " edge: " << edge_no << " reserved till cycle: " << cycle[ edge_no ] << " for operation: " << ins->name);
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

    // initialize resource cycle machine to be free for all qubits
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
| cc_resource_manager
\************************************************************************/

static Map<UInt,UInt> qubit2instrument(const Json &instrument, UInt unit) {
    Map<UInt,UInt> ret;

    auto qubits = json_get<const Json &>(instrument, "qubits");
    UInt qubitGroupCnt = qubits.size();                                  // NB: JSON key qubits is a 'matrix' of [groups*qubits]
    for (UInt group=0; group<qubitGroupCnt; group++) {
        const Json qubitsOfGroup = qubits[group];
        if (qubitsOfGroup.size() == 1) {    // FIXME: specific for measurements
            Int qubit = qubitsOfGroup[0].get<Int>();
            QL_IOUT("instrument " << unit << "/" << instrument["name"] << ": adding qubit " << qubit);
            ret.set(qubit) = unit;
        }
    }
    return ret;
}

cc_resource_manager::cc_resource_manager(
    const quantum_platform &platform,
    scheduling_direction_t dir
) :
    platform_resource_manager_t(platform, dir)
{
    QL_DOUT("Constructing (platform,dir) parameterized platform_resource_manager_t");
    QL_DOUT("New one for direction " << dir << " with no of resources : " << platform.resources.size() );

    // unconditionally add resources that do not use JSON section "resources"

    // add qubit resource
    UInt qubit_number = json_get<UInt>(platform.hardware_settings, "qubit_number", "hardware_settings/qubit_number");
    resource_ptrs.push_back(new ResourceQubit(platform, dir, qubit_number));

    // load CC settings
    Settings settings;
    settings.loadBackendSettings(platform);

    // parse instrument definitions for resource information
    Map<UInt,UInt> qubit2meas;
    UInt meas_unit = 0;
    Map<UInt,UInt> qubit2flux;
    for (UInt i=0; i<settings.getInstrumentsSize(); i++) {
        const Json &instrument = settings.getInstrumentAtIdx(i);
        Str signal_type = json_get<Str>(instrument, "signal_type");
        // FIXME: this adds semantics to "signal_type", whereas the names are otherwise fully up to the user
        if ("measure" == signal_type) {
            Map<UInt,UInt> map = qubit2instrument(instrument, meas_unit);
#if 0   // FIXME: breaks CI, in utils/map.h
            qubit2meas.insert(map.begin(), map.end());
#endif
            meas_unit++;
        } else if ("flux" == signal_type) {
            /*  we map all fluxing on a single 'unit': the actual resource we'd like to manage is a *signal* that connects
                to a flux line of a qubit. On a single instrument (e.g. ZI HDAWG) these signals cannot be triggered
                during playback of other signals.
                Note however, that a single "flux" gate may trigger signals on different instruments. During scheduling,
                we don't know about these signals yet though, and we also don't particularly care about keeping the
                instruments independent where possible.
                Also note that all of this breaks down if/when we want to support stuff like "flux assisted measurement",
                where the flux line is manipulated during a measurement, and we can no longer tie types like "flux" and
                "measurement" to a gate, but must really work with th signals.
            */
            Map<UInt,UInt> map = qubit2instrument(instrument, 0);
#if 0   // FIXME: breaks CI, in utils/map.h
            qubit2flux.insert(map.begin(), map.end());
#endif
        }
    }

    // add resources based on instrument definitions
    UInt num_meas_unit = meas_unit;
    resource_ptrs.push_back(new ResourceSharedInstrument(platform, dir, "meas", num_meas_unit, qubit2meas, Settings::isReadout));
    resource_ptrs.push_back(new ResourceSharedInstrument(platform, dir, "flux", 1, qubit2flux, Settings::isFlux));

    // handle platform resource definitions
    for (auto it = platform.resources.cbegin(); it != platform.resources.cend(); ++it) {
        Str key = it.key();
        if (key == "qubits" || key == "meas_units" || key == "qwgs" || key == "edges" || key == "detuned_qubits") {  // keys used by CC-light
            QL_WOUT("resource key '" << key << "' not used by CC");
        } else {
            QL_JSON_FATAL("illegal resource key '" << key << "'");
        }
    }
}

cc_resource_manager *cc_resource_manager::clone() const & {
    QL_DOUT("Cloning/copying cc_resource_manager");
    return new cc_resource_manager(*this);
}

cc_resource_manager *cc_resource_manager::clone() && {
    QL_DOUT("Cloning/moving cc_resource_manager");
    return new cc_resource_manager(std::move(*this));
}

} // namespace cc
} // namespace arch
} // namespace ql
