/** \file
 * Basic pass group implementation.
 */

#pragma once

#include <functional>
#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/ptr.h"
#include "ql/ir/ir.h"
#include "ql/pmgr/pass_types/specializations.h"

namespace ql {
namespace pmgr {

/**
 * A generic group of passes, with no special functionality or default set of
 * passes.
 */
class Group : public pass_types::Group {
public:

    /**
     * Constructs the pass group. No error checking here; this is up to the
     * parent pass group. Note that the type name is missing, and that
     * instance_name defaults to the empty string; generic passes always have
     * an empty type name, and the root group has an empty instance name as
     * well.
     */
    Group(
        const utils::Ptr<const Factory> &pass_factory,
        const utils::Str &instance_name
    );

protected:

    /**
     * Implementation for the initial pass list. This is no-op for a generic
     * pass group.
     */
    void get_passes(
        const utils::Ptr<const Factory> &factory,
        utils::List<PassRef> &passes
    ) final;

    /**
     * Writes the documentation for a basic pass group to the given stream.
     */
    void dump_docs(
        std::ostream &os,
        const utils::Str &line_prefix
    ) const override;

public:

    /**
     * Returns a user-friendly type name for this pass.
     */
    utils::Str get_friendly_type() const override;

};

} // namespace pmgr
} // namespace ql
