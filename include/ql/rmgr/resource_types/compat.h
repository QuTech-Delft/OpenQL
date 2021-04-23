/** \file
 * Temporary compatibility layer for resources.
 */

#pragma once

#include "ql/rmgr/resource_types/base.h"

namespace ql {
namespace rmgr {
namespace resource_types {

// FIXME JvS: replace all old-style resources with new ones, then delete this
//  whole file.
class OldResource {
public:
    utils::Str name;
    utils::UInt count;
    Direction direction;

    OldResource(const utils::Str &n, Direction dir) : name(n), direction(dir) {}
    virtual ~OldResource() = default;

    virtual utils::Bool available(utils::UInt op_start_cycle, const ir::GateRef &ins, const plat::PlatformRef &platform) const = 0;
    virtual void reserve(utils::UInt op_start_cycle, const ir::GateRef &ins, const plat::PlatformRef &platform) = 0;
};


template<class T>
class Compat : public Base {
private:

    /**
     * The old-style resource being wrapped.
     */
    utils::Opt<T> resource;

public:

    /**
     * Constructs the resource.
     */
    explicit Compat(const Context &context) : Base(context) {}

    /**
     * Abstract implementation for initialize(). This is where the JSON
     * structure should be parsed and the resource state should be initialized.
     * This will only be called once during the lifetime of this resource. The
     * default implementation is no-op.
     */
    void on_initialize(Direction direction) override {
        if (direction == Direction::UNDEFINED) {
            throw utils::Exception(
                "direction must be forward or backward for old-style resources"
            );
        }
        resource.emplace(context->platform, direction);
    }

    /**
     * Abstract implementation for gate().
     */
    utils::Bool on_gate(
        utils::UInt cycle,
        const ir::GateRef &gate,
        utils::Bool commit
    ) override {
        auto result = resource->available(cycle, gate, context->platform);
        if (result && commit) {
            resource->reserve(cycle, gate, context->platform);
        }
        return result;
    }

    /**
     * Writes the documentation for this resource to the given output stream.
     * May depend on type_name, but should not depend on anything else. The help
     * information should end in a newline, and every line printed should start
     * with line_prefix.
     */
    void on_dump_docs(
        std::ostream &os,
        const utils::Str &line_prefix
    ) const override {
        os << line_prefix << "Compatibility wrapper for " << typeid(T).name() << "\n";
    }

    /**
     * Returns a user-friendly type name for this resource.
     */
    utils::Str get_friendly_type() const override {
        return utils::Str("Compatibility wrapper for ") + typeid(T).name();
    }

    /**
     * Abstract implementation for dump_config().
     */
    void on_dump_config(
        std::ostream &os,
        const utils::Str &line_prefix
    ) const override {
        os << line_prefix << "Config dump is not implemented for compatibility wrapper\n";
    }

    /**
     * Dumps a debug representation of the current resource state.
     */
    void on_dump_state(
        std::ostream &os,
        const utils::Str &line_prefix
    ) const override {
        os << line_prefix << "State dump is not implemented for compatibility wrapper\n";
    }

};

} // namespace resource_types
} // namespace rmgr
} // namespacq ql
