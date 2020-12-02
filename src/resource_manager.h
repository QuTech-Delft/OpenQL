/** \file
 * Resource manager interface for the scheduler.
 */

#pragma once

#include "utils/num.h"
#include "utils/str.h"
#include "utils/vec.h"
#include "platform.h"

namespace ql {

typedef enum {
    forward_scheduling = 0,
    backward_scheduling = 1
} scheduling_direction_t;

namespace arch {

class resource_t {
public:
    utils::Str name;
    utils::UInt count;
    scheduling_direction_t direction;

    resource_t(const utils::Str &n, scheduling_direction_t dir);
    virtual ~resource_t() = default;

    virtual utils::Bool available(utils::UInt op_start_cycle, gate *ins, const quantum_platform &platform) = 0;
    virtual void reserve(utils::UInt op_start_cycle, gate *ins, const quantum_platform &platform) = 0;

    virtual resource_t *clone() const & = 0;
    virtual resource_t *clone() && = 0;

    void Print(const utils::Str &s);
};

class platform_resource_manager_t {
public:

    utils::Vec<resource_t*> resource_ptrs;

    // constructor needed by mapper::FreeCycle to bridge time from its construction to its Init
    // see the note on the use of constructors and Init functions at the start of mapper.h
    platform_resource_manager_t() = default;
    platform_resource_manager_t(
        const quantum_platform &platform,
        scheduling_direction_t dir
    );

    virtual platform_resource_manager_t *clone() const & = 0;
    virtual platform_resource_manager_t *clone() && = 0;

    void Print(const utils::Str &s);

    // copy constructor doing a deep copy
    // *org_resource_ptr->clone() does the trick to create a copy of the actual derived class' object
    platform_resource_manager_t(const platform_resource_manager_t &org);

    // copy-assignment operator
    // follow pattern to use tmp copy to allow self-assignment and to be exception safe
    platform_resource_manager_t &operator=(const platform_resource_manager_t &rhs);

    utils::Bool available(utils::UInt op_start_cycle, gate *ins, const quantum_platform &platform);
    void reserve(utils::UInt op_start_cycle, gate *ins, const quantum_platform &platform);

    // destructor destroying deep resource_t's
    // runs before shallow destruction which is done by synthesized platform_resource_manager_t destructor
    virtual ~platform_resource_manager_t();
};

class resource_manager_t {
public:

    platform_resource_manager_t *platform_resource_manager_ptr;     // pointer to specific platform_resource_manager

    resource_manager_t();

    // (platform,dir) parameterized resource_manager_t
    // dynamically allocating platform specific platform_resource_manager_t depending on platform
    resource_manager_t(const quantum_platform &platform, scheduling_direction_t dir);

    // copy constructor doing a deep copy
    // *org_resource_manager.platform_resource_manager_ptr->clone() does the trick
    //      to create a copy of the actual derived class' object
    resource_manager_t(const resource_manager_t &org_resource_manager);

    // copy-assignment operator
    // follow pattern to use tmp copy to allow self-assignment and to be exception safe
    resource_manager_t &operator=(const resource_manager_t &rhs);

    utils::Bool available(utils::UInt op_start_cycle, gate *ins, const quantum_platform &platform);
    void reserve(utils::UInt op_start_cycle, gate *ins, const quantum_platform &platform);

    // destructor destroying deep platform_resource_managert_t
    // runs before shallow destruction which is done by synthesized resource_manager_t destructor
    virtual ~resource_manager_t();
};

} // namespace arch
} // namespacq ql
