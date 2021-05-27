/** \file
 * Basic pass group implementation.
 */

#include "ql/pmgr/group.h"

namespace ql {
namespace pmgr {

/**
 * Constructs the pass group. No error checking here; this is up to the
 * parent pass group. Note that the type name is missing, and that
 * instance_name defaults to the empty string; generic passes always have
 * an empty type name, and the root group has an empty instance name as
 * well.
 */
Group::Group(
    const utils::Ptr<const Factory> &pass_factory,
    const utils::Str &instance_name
) : pass_types::Group(pass_factory, instance_name, "") {
    construct();
}

/**
 * Implementation for the initial pass list. This is no-op for a generic
 * pass group.
 */
void Group::get_passes(
    const utils::Ptr<const Factory> &factory,
    utils::List<PassRef> &passes
) {
}

/**
 * Writes the documentation for a basic pass group to the given stream.
 */
void Group::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    os << line_prefix << "A basic pass group. These pass groups are just a tool for abstracting\n";
    os << line_prefix << "a number of passes that together perform a single logical transformation\n";
    os << line_prefix << "into a single pass. A pass group has exactly the same behavior as just\n";
    os << line_prefix << "putting all the contained passes in the parent pass group.\n";
}

/**
 * Returns a user-friendly type name for this pass.
 */
utils::Str Group::get_friendly_type() const {
    return "Generic group";
}

} // namespace pmgr
} // namespace ql
