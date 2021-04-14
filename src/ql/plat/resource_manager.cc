/** \file
 * Resource manager interface for the scheduler.
 */

#include "ql/plat/resource_manager.h"

#include "arch/cc_light/cc_light_resource_manager.h"

namespace ql {
namespace plat {

using namespace utils;

Resource::Resource(
    const Str &n,
    com::SchedulingDirection dir
) :
    name(n),
    direction(dir)
{
    QL_DOUT("constructing resource: " << n << " for direction: " << dir);
}

PlatformResourceManager::PlatformResourceManager(
    const plat::PlatformRef &platform,
    com::SchedulingDirection dir
) {
    // DOUT("Constructing (platform,dir) parameterized platform_resource_manager_t");
}

// copy constructor doing a deep copy
// *org_resource_ptr->clone() does the trick to create a copy of the actual derived class' object
PlatformResourceManager::PlatformResourceManager(const PlatformResourceManager &org) {
    resource_ptrs.clear();
    for (const auto &org_resource_ptr : org.resource_ptrs) {
        resource_ptrs.push_back(org_resource_ptr.clone());
    }
}

// copy-assignment operator
// follow pattern to use tmp copy to allow self-assignment and to be exception safe
PlatformResourceManager &PlatformResourceManager::operator=(const PlatformResourceManager &rhs) {
    resource_ptrs.clear();
    for (const auto &org_resource_ptr : rhs.resource_ptrs) {
        resource_ptrs.push_back(org_resource_ptr.clone());
    }
    return *this;
}

utils::Bool PlatformResourceManager::available(
    utils::UInt op_start_cycle,
    const ir::GateRef &ins,
    const plat::PlatformRef &platform
) const {
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

void PlatformResourceManager::reserve(
    utils::UInt op_start_cycle,
    const ir::GateRef &ins,
    const plat::PlatformRef &platform
) {
    // DOUT("reserving resources for: " << ins->qasm());
    for (auto rptr : resource_ptrs) {
        // DOUT("... reserving resource " << rptr->name);
        rptr->reserve(op_start_cycle, ins, platform);
    }
    // DOUT("all resources reserved for: " << ins->qasm());
}

// (platform,dir) parameterized resource_manager_t
// dynamically allocating platform specific platform_resource_manager_t depending on platform
ResourceManager::ResourceManager(
    const plat::PlatformRef &platform,
    com::SchedulingDirection dir
) {
    // DOUT("Constructing (platform,dir) parameterized resource_manager_t");
    Str eqasm_compiler_name = platform->eqasm_compiler_name;

    if (eqasm_compiler_name == "cc_light_compiler") {
        platform_resource_manager_ptr.emplace<arch::cc_light_resource_manager_t>(platform, dir);
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
ResourceManager::ResourceManager(const ResourceManager &org_resource_manager) {
    platform_resource_manager_ptr = org_resource_manager.platform_resource_manager_ptr.clone();
}

// copy-assignment operator
// follow pattern to use tmp copy to allow self-assignment and to be exception safe
ResourceManager &ResourceManager::operator=(const ResourceManager &rhs) {
    platform_resource_manager_ptr = rhs.platform_resource_manager_ptr.clone();
    return *this;
}

utils::Bool ResourceManager::available(
    utils::UInt op_start_cycle,
    const ir::GateRef &ins,
    const plat::PlatformRef &platform
) const {
    // DOUT("resource_manager.available()");
    return platform_resource_manager_ptr->available(op_start_cycle, ins, platform);
}

void ResourceManager::reserve(
    utils::UInt op_start_cycle,
    const ir::GateRef &ins,
    const plat::PlatformRef &platform
) {
    // DOUT("resource_manager.reserve()");
    platform_resource_manager_ptr->reserve(op_start_cycle, ins, platform);
}

} // namespace plat
} // namespace ql
