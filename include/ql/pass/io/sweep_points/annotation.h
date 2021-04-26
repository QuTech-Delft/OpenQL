/** \file
 * Defines the program annotation used to record sweep point data.
 */

#pragma once

namespace ql {
namespace pass {
namespace io {
namespace sweep_points {

/**
 * Program annotation used to record sweep point data.
 */
struct Annotation {

    /**
     * TODO JvS: still not sure what this is.
     */
    utils::Vec<utils::Real> data = {};

    /**
     * Configuration file name for the sweep points pass. Leave empty to use the
     * default generated filename.
     */
    utils::Str config_file_name = "";

};

} // namespace sweep_points
} // namespace io
} // namespace pass
} // namespace ql
