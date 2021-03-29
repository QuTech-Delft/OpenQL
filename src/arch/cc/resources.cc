/** \file
 *  Resources that are not specific for the CC platform.
 *  Based on arch/cc_light/cc_light_resource_manager.cc commit a95bc15c90ad17c2837adc2b3c36e031595e68d1
 */

//#include "settings_cc.h"    // FIXME: required because there is no platform-independent way to know what constitutes a measurement

#include "resources.h"


namespace ql {
namespace arch {
namespace cc {

using namespace utils;

/************************************************************************\
| cc_resource_qubit
\************************************************************************/

cc_resource_qubit::cc_resource_qubit(
    const quantum_platform &platform,
    scheduling_direction_t dir,
    UInt qubit_number
) :
    resource_t("qubits", dir)
{
    cycle.resize(qubit_number);
    UInt val = forward_scheduling == dir ? 0 : MAX_CYCLE;
    for (UInt q = 0; q < qubit_number; q++) {
        cycle[q] = val;
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
    for (auto q : ins->operands) {
        if (forward_scheduling == direction) {
            QL_DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle << "  qubit: " << q << " is busy till cycle : " << cycle[q]);
            if (op_start_cycle < cycle[q]) {
                QL_DOUT("    " << name << " resource busy ...");
                return false;
            }
        } else {
            QL_DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle << "  qubit: " << q << " is busy from cycle : " << cycle[q]);
            UInt operation_duration = platform.time_to_cycles(ins->duration);
            if (op_start_cycle + operation_duration > cycle[q]) {
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
    UInt val = forward_scheduling == direction ?  op_start_cycle + operation_duration : op_start_cycle;

    for (auto q : ins->operands) {
        cycle[q] = val;
        QL_DOUT("reserved " << name << ". op_start_cycle: " << op_start_cycle << " qubit: " << q << " reserved till/from cycle: " << cycle[q]);
    }
}


/************************************************************************\
| cc_resource_meas
\************************************************************************/

cc_resource_meas::cc_resource_meas(
    const quantum_platform &platform,
    scheduling_direction_t dir,
    UInt num_meas_unit,
    const Map<UInt,UInt> &_qubit2meas,
    tUsesResource _usesResourceFunc
) :
    resource_t("meas_units", dir)   // FIXME
{
    qubit2meas = _qubit2meas;
    usesResourceFunc = _usesResourceFunc;

    fromcycle.resize(num_meas_unit);
    tocycle.resize(num_meas_unit);

    UInt val = forward_scheduling == dir ? 0 : MAX_CYCLE;
    for (UInt i=0; i<num_meas_unit; i++) {
        fromcycle[i] = val;
        tocycle[i] = val;
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

// FIXME: should we disallow gates during measurement?
Bool cc_resource_meas::available(
    UInt op_start_cycle,
    gate *ins,
    const quantum_platform &platform
) {
    if (usesResourceFunc(platform, ins->name)) {
        for (auto q : ins->operands) {
            QL_DOUT(
                " available " << name
                << "? op_start_cycle: " << op_start_cycle
                << "  meas: " << qubit2meas.at(q)
                << " is busy from cycle: " << fromcycle[qubit2meas.at(q)]
                << " to cycle: " << tocycle[qubit2meas.at(q)]
            );
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
                    UInt operation_duration = platform.time_to_cycles(ins->duration);
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
    if (usesResourceFunc(platform, ins->name)) {
        UInt operation_duration = platform.time_to_cycles(ins->duration);
        for (auto q : ins->operands) {
            fromcycle[qubit2meas.at(q)] = op_start_cycle;
            tocycle[qubit2meas.at(q)] = op_start_cycle + operation_duration;
            QL_DOUT(
                "reserved " << name
                << ". op_start_cycle: " << op_start_cycle
                << " meas: " << qubit2meas.at(q)
                << " reserved from cycle: " << fromcycle[qubit2meas.at(q)]
                << " to cycle: " << tocycle[qubit2meas.at(q)]
            );
        }
    }
}

} // namespace cc
} // namespace arch
} // namespace ql
