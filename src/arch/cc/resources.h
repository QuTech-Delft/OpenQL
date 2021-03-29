/** \file
 *  Resources that are not specific for the CC platform.
 *  Based on arch/cc_light/cc_light_resource_manager.h commit a95bc15c90ad17c2837adc2b3c36e031595e68d1
 */

#pragma once

#include "types_cc.h"

#include "resource_manager.h"
#include "utils/num.h"
#include "utils/str.h"
#include "utils/pair.h"
#include "utils/vec.h"

#include <fstream>

namespace ql {
namespace arch {
namespace cc {

// user function to determine whether instruction uses resource
typedef Bool (*tUsesResource)(const quantum_platform &platform, const Str &iname);


// Each qubit can be used by only one gate at a time.
class ResourceQubit : public resource_t {
public:
    // fwd: qubit q is busy till cycle=cycle[q], i.e. all cycles < cycle[q] it is busy, i.e. start_cycle must be >= cycle[q]
    // bwd: qubit q is busy from cycle=cycle[q], i.e. all cycles >= cycle[q] it is busy, i.e. start_cycle+duration must be <= cycle[q]
    Vec<UInt> cycle;

    ResourceQubit(const quantum_platform &platform, scheduling_direction_t dir, UInt qubit_number);

    ResourceQubit *clone() const & override;
    ResourceQubit *clone() && override;

    Bool available(UInt op_start_cycle, gate *ins, const quantum_platform &platform) override;
    void reserve(UInt op_start_cycle, gate *ins, const quantum_platform &platform) override;
};


// Some instruments can control multiple qubits at the same time, but only when they start at the same time.
class ResourceSharedInstrument : public resource_t {
public:
    Vec<UInt> fromcycle;  // last measurement start cycle
    Vec<UInt> tocycle;    // is busy till cycle
    Map<UInt,UInt> qubit2instr;

    ResourceSharedInstrument(
            const quantum_platform &platform,
            scheduling_direction_t dir,
            const Str &name,
            UInt num_instr,
            const Map<UInt,UInt> &_qubit2meas,
            tUsesResource usesResourceFunc);

    ResourceSharedInstrument *clone() const & override;
    ResourceSharedInstrument *clone() && override;

    Bool available(UInt op_start_cycle, gate *ins, const quantum_platform &platform) override;
    void reserve(UInt op_start_cycle, gate *ins, const quantum_platform &platform) override;

private:
    tUsesResource usesResourceFunc;
};

} // namespace cc
} // namespace arch
} // namespace ql
