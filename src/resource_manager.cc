/** \file
 * Resource manager interface for the scheduler.
 */

#include "resource_manager.h"

#include "arch/cc_light/cc_light_resource_manager.h"
// #include "arch/cc/cc_resource_manager.h"

namespace ql {
namespace arch {

using namespace utils;

resource_t::resource_t(
    const Str &n,
    scheduling_direction_t dir
) :
    name(n),
    direction(dir)
{
    QL_DOUT("constructing resource: " << n << " for direction (0:fwd,1:bwd): " << dir);
}

void resource_t::Print(const Str &s) {
    QL_DOUT(s);
    QL_DOUT("resource name=" << name << "; count=" << count);
}

platform_resource_manager_t::platform_resource_manager_t(
    const quantum_platform &platform,
    scheduling_direction_t dir
) {
    // DOUT("Constructing (platform,dir) parameterized platform_resource_manager_t");
}

void platform_resource_manager_t::Print(const Str &s) {
    QL_DOUT(s);
}

// copy constructor doing a deep copy
// *org_resource_ptr->clone() does the trick to create a copy of the actual derived class' object
platform_resource_manager_t::platform_resource_manager_t(const platform_resource_manager_t &org) {
    // DOUT("Copy constructing platform_resource_manager_t");
    resource_ptrs.clear();
    for (auto org_resource_ptr : org.resource_ptrs) {
        resource_ptrs.push_back(org_resource_ptr->clone());
    }
}

// copy-assignment operator
// follow pattern to use tmp copy to allow self-assignment and to be exception safe
platform_resource_manager_t &platform_resource_manager_t::operator=(const platform_resource_manager_t &rhs) {
    // DOUT("Copy assigning platform_resource_manager_t");
    Vec<resource_t *> new_resource_ptrs;
    for (auto org_resource_ptr : rhs.resource_ptrs) {
        // DOUT("... clone one of the contained resource_ptr");
        new_resource_ptrs.push_back(org_resource_ptr->clone());
    }
    // DOUT("... cloned all contained resource_ptrs");
    for (auto rptr : resource_ptrs) {
        // DOUT("... delete one of the old contained resource_ptr");
        delete rptr;
    }
    // DOUT("... deleted all contained resource_ptrs; now copy the new ones in, and return the platform_resource_manager");
    resource_ptrs = new_resource_ptrs;
    return *this;
}

utils::Bool platform_resource_manager_t::available(
    utils::UInt op_start_cycle,
    gate *ins,
    const quantum_platform &platform
) {
    // DOUT("checking availability of resources for: " << ins->qasm());
    for (auto rptr : resource_ptrs) {
        // DOUT("... checking availability for resource " << rptr->name);
        if (rptr->available(op_start_cycle, ins, platform) == false) {
            // DOUT("... resource " << rptr->name << "not available");
            return false;
        }
    }
    // DOUT("all resources available for: " << ins->qasm());
    return true;
}

void platform_resource_manager_t::reserve(
    utils::UInt op_start_cycle,
    gate *ins,
    const quantum_platform &platform
) {
    // DOUT("reserving resources for: " << ins->qasm());
    for (auto rptr : resource_ptrs) {
        // DOUT("... reserving resource " << rptr->name);
        rptr->reserve(op_start_cycle, ins, platform);
    }
    // DOUT("all resources reserved for: " << ins->qasm());
}

// destructor destroying deep resource_t's
// runs before shallow destruction which is done by synthesized platform_resource_manager_t destructor
platform_resource_manager_t::~platform_resource_manager_t() {
    // DOUT("Destroying platform_resource_manager_t");
    for (auto rptr : resource_ptrs) {
        delete rptr;
    }
}

resource_manager_t::resource_manager_t() {
    // DOUT("Constructing virgin resource_manager_t");
    platform_resource_manager_ptr = NULL;
}

// (platform,dir) parameterized resource_manager_t
// dynamically allocating platform specific platform_resource_manager_t depending on platform
resource_manager_t::resource_manager_t(
    const quantum_platform &platform,
    scheduling_direction_t dir
) {
    // DOUT("Constructing (platform,dir) parameterized resource_manager_t");
    Str eqasm_compiler_name = platform.eqasm_compiler_name;

    if (eqasm_compiler_name == "cc_light_compiler") {
        platform_resource_manager_ptr = new cc_light_resource_manager_t(platform, dir);
//  } else if (eqasm_compiler_name == "cc_compiler") {
//      platform_resource_manager_ptr = new cc_resource_manager_t(platform, dir);
    } else {
        QL_FATAL("the '" << eqasm_compiler_name
                         << "' eqasm compiler backend is not supported !");
    }
}

// copy constructor doing a deep copy
// *org_resource_manager.platform_resource_manager_ptr->clone() does the trick
//      to create a copy of the actual derived class' object
resource_manager_t::resource_manager_t(const resource_manager_t &org_resource_manager) {
    // DOUT("Copy constructing resource_manager_t");
    platform_resource_manager_ptr = org_resource_manager.platform_resource_manager_ptr->clone();
    // DOUT("... done copy constructing resource_manager_t by cloning the contained platform_resource_manager_t");
}

// copy-assignment operator
// follow pattern to use tmp copy to allow self-assignment and to be exception safe
resource_manager_t &resource_manager_t::operator=(const resource_manager_t &rhs) {
    // DOUT("Copy assigning resource_manager_t");
    platform_resource_manager_t *new_resource_manager_ptr;
    // DOUT("... about to clone resource_manager rhs' contained platform_resource_manager_t");
    new_resource_manager_ptr = rhs.platform_resource_manager_ptr->clone();
    // DOUT("... about to delete the this'(lhs) resource_manager contained platform_resource_manager_t");
    delete platform_resource_manager_ptr;
    // DOUT("... and then assign the cloned copy platform_resource_manager_t to the this resource_manager contained one");
    platform_resource_manager_ptr = new_resource_manager_ptr;
    // DOUT("... having done this, copy the copied resource_manager_t to the lhs");
    return *this;
}

utils::Bool resource_manager_t::available(
    utils::UInt op_start_cycle,
    gate *ins,
    const quantum_platform &platform
) {
    // DOUT("resource_manager.available()");
    return platform_resource_manager_ptr->available(op_start_cycle, ins, platform);
}

void resource_manager_t::reserve(
    utils::UInt op_start_cycle,
    gate *ins,
    const quantum_platform &platform
) {
    // DOUT("resource_manager.reserve()");
    platform_resource_manager_ptr->reserve(op_start_cycle, ins, platform);
}

// destructor destroying deep platform_resource_managert_t
// runs before shallow destruction which is done by synthesized resource_manager_t destructor
resource_manager_t::~resource_manager_t() {
    // DOUT("Destroying resource_manager_t");
    delete platform_resource_manager_ptr;
}

} // namespace arch
} // namespace ql
