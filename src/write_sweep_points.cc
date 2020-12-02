/** \file
 * Implementation of pass that writes sweep points.
 */

#include "write_sweep_points.h"

#include "utils/filesystem.h"
#include "options.h"

namespace ql {

using namespace utils;

// write_sweep_points pass
void write_sweep_points(
    quantum_program *programp,
    const quantum_platform &platform,
    const Str &passname
) {
    QL_DOUT("write_sweep_points()");
    if (programp->sweep_points.size()) {
        StrStrm ss_swpts;
        ss_swpts << "{ \"measurement_points\" : [";
        for (UInt i = 0; i < programp->sweep_points.size() - 1; i++) {
            ss_swpts << programp->sweep_points[i] << ", ";
        }
        ss_swpts << programp->sweep_points[programp->sweep_points.size() - 1] << "] }";
        Str config = ss_swpts.str();

        StrStrm ss_config;
        if (programp->default_config) {
            ss_config << options::get("output_dir") << "/" << programp->unique_name << "_config.json";
        } else {
            ss_config << options::get("output_dir") << "/" << programp->config_file_name;
        }
        Str conf_file_name = ss_config.str();

        QL_IOUT("writing sweep points to '" << conf_file_name << "'...");
        OutFile(conf_file_name).write(config);
    } else {
        QL_IOUT("sweep points file not generated as sweep point array is empty !");
    }
    QL_DOUT("write_sweep_points() [Done] ");
}

} // namespace ql
