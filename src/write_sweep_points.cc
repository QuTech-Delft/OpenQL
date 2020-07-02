/**
 * @file   write_sweep_points.cc
 * @date   11/2016
 * @author Nader Khammassi
 */
#include "utils.h"
#include "write_sweep_points.h"


namespace ql
{
    // write_sweep_points pass
    void write_sweep_points(ql::quantum_program* programp, const ql::quantum_platform& platform, std::string passname)
    {
        DOUT("write_sweep_points()");
        if (programp->sweep_points.size())
        {
            std::stringstream ss_swpts;
            ss_swpts << "{ \"measurement_points\" : [";
            for (size_t i=0; i<programp->sweep_points.size()-1; i++)
                ss_swpts << programp->sweep_points[i] << ", ";
            ss_swpts << programp->sweep_points[programp->sweep_points.size()-1] << "] }";
            std::string config = ss_swpts.str();

            std::stringstream ss_config;
            if (programp->default_config)
            {
                ss_config << ql::options::get("output_dir") << "/" << programp->unique_name << "_config.json";
            }
            else
            {
                ss_config << ql::options::get("output_dir") << "/" << programp->config_file_name;
            }
            std::string conf_file_name = ss_config.str();

            IOUT("writing sweep points to '" << conf_file_name << "'...");
            ql::utils::write_file(conf_file_name, config);
        }
        else
        {
            IOUT("sweep points file not generated as sweep point array is empty !");
        }
        DOUT("write_sweep_points() [Done] ");
    }
}
