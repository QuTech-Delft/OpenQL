/** \file
 * Resource manager interface for the scheduler.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "ql/utils/ptr.h"
#include "ql/plat/platform.h"
#include "ql/ir/ir.h"
#include "ql/com/types.h"

namespace ql {
namespace plat {

class Resource {
public:
    utils::Str name;
    utils::UInt count;
    com::SchedulingDirection direction;

    Resource(const utils::Str &n, com::SchedulingDirection dir);
    virtual ~Resource() = default;

    virtual utils::Bool available(utils::UInt op_start_cycle, const ir::GateRef &ins, const plat::PlatformRef &platform) const = 0;
    virtual void reserve(utils::UInt op_start_cycle, const ir::GateRef &ins, const plat::PlatformRef &platform) = 0;
};

class PlatformResourceManager {
public:

    utils::Vec<utils::ClonablePtr<Resource>> resource_ptrs;

    // constructor needed by mapper::FreeCycle to bridge time from its construction to its Init
    // see the note on the use of constructors and Init functions at the start of mapper.h
    PlatformResourceManager(
        const plat::PlatformRef &platform,
        com::SchedulingDirection dir
    );

    // copy constructor doing a deep copy
    // *org_resource_ptr->clone() does the trick to create a copy of the actual derived class' object
    PlatformResourceManager(const PlatformResourceManager &org);

    // copy-assignment operator
    // follow pattern to use tmp copy to allow self-assignment and to be exception safe
    PlatformResourceManager &operator=(const PlatformResourceManager &rhs);

    utils::Bool available(utils::UInt op_start_cycle, const ir::GateRef &ins, const plat::PlatformRef &platform) const;
    void reserve(utils::UInt op_start_cycle, const ir::GateRef &ins, const plat::PlatformRef &platform);

    // destructor destroying deep resource_t's
    // runs before shallow destruction which is done by synthesized platform_resource_manager_t destructor
    virtual ~PlatformResourceManager() = default;
};

class ResourceManager {
public:

    utils::ClonablePtr<PlatformResourceManager> platform_resource_manager_ptr;     // pointer to specific platform_resource_manager

    // (platform,dir) parameterized resource_manager_t
    // dynamically allocating platform specific platform_resource_manager_t depending on platform
    ResourceManager(const plat::PlatformRef &platform, com::SchedulingDirection dir);

    // copy constructor doing a deep copy
    // *org_resource_manager.platform_resource_manager_ptr->clone() does the trick
    //      to create a copy of the actual derived class' object
    ResourceManager(const ResourceManager &org_resource_manager);

    // copy-assignment operator
    // follow pattern to use tmp copy to allow self-assignment and to be exception safe
    ResourceManager &operator=(const ResourceManager &rhs);

    utils::Bool available(utils::UInt op_start_cycle, const ir::GateRef &ins, const plat::PlatformRef &platform) const;
    void reserve(utils::UInt op_start_cycle, const ir::GateRef &ins, const plat::PlatformRef &platform);

    // destructor destroying deep platform_resource_managert_t
    // runs before shallow destruction which is done by synthesized resource_manager_t destructor
    virtual ~ResourceManager() = default;
};

} // namespace plat
} // namespacq ql
