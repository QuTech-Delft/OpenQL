/**
 * @file   options.h
 * @date   05/2018
 * @author Imran Ashraf
 * @brief  Options class implementing parsing of supported options
 */

#ifndef QL_OPTIONS_H
#define QL_OPTIONS_H

#include <utils.h>
#include <exception.h>
#include <CLI/CLI.hpp>
//#include <iostream>

namespace ql
{
  class Options
  {
  private:
      CLI::App * app;
      std::map<std::string, std::string> opt_name2opt_val;
      
      void set_defaults()
      {
          // default values
          opt_name2opt_val["log_level"] = "LOG_NOTHING";
          opt_name2opt_val["output_dir"] = "test_output";
          opt_name2opt_val["unique_output"] = "no";
          opt_name2opt_val["write_qasm_files"] = "no";
          opt_name2opt_val["write_report_files"] = "no";

          opt_name2opt_val["optimize"] = "no";
          opt_name2opt_val["use_default_gates"] = "yes";
          opt_name2opt_val["decompose_toffoli"] = "no";
          opt_name2opt_val["quantumsim"] = "no";
          opt_name2opt_val["issue_skip_319"] = "no";

          opt_name2opt_val["scheduler"] = "ALAP";
          opt_name2opt_val["scheduler_uniform"] = "no";
          opt_name2opt_val["scheduler_commute"] = "no";
          opt_name2opt_val["prescheduler"] = "yes";
          opt_name2opt_val["scheduler_post179"] = "yes";
          opt_name2opt_val["backend_cc_map_input_file"] = "";

          opt_name2opt_val["cz_mode"] = "manual";
          opt_name2opt_val["print_dot_graphs"] = "no";

          opt_name2opt_val["clifford_prescheduler"] = "no";
          opt_name2opt_val["clifford_postscheduler"] = "no";
          opt_name2opt_val["clifford_premapper"] = "no";
          opt_name2opt_val["clifford_postmapper"] = "no";

          opt_name2opt_val["mapper"] = "no";
          opt_name2opt_val["mapassumezeroinitstate"] = "no";
          opt_name2opt_val["mapinitone2one"] = "yes";
          opt_name2opt_val["mapprepinitsstate"] = "no";
          opt_name2opt_val["initialplace"] = "no";
          opt_name2opt_val["initialplace2qhorizon"] = "0";
          opt_name2opt_val["maplookahead"] = "noroutingfirst";
          opt_name2opt_val["mappathselect"] = "all";
          opt_name2opt_val["maprecNN2q"] = "no";
          opt_name2opt_val["mapselectmaxlevel"] = "0";
          opt_name2opt_val["mapselectmaxwidth"] = "min";
          opt_name2opt_val["mapselectswaps"] = "all";
          opt_name2opt_val["maptiebreak"] = "random";
          opt_name2opt_val["mapusemoves"] = "yes";
          opt_name2opt_val["mapreverseswap"] = "yes";

          // add options with default values and list of possible values
          app->add_set_ignore_case("--log_level", opt_name2opt_val["log_level"],
            {"LOG_NOTHING", "LOG_CRITICAL", "LOG_ERROR", "LOG_WARNING", "LOG_INFO", "LOG_DEBUG"}, "Log levels", true);
          app->add_option("--output_dir", opt_name2opt_val["output_dir"], "Name of output directory", true);
          app->add_set_ignore_case("--unique_output", opt_name2opt_val["unique_output"], {"no", "yes"}, "Make output files unique", true);
          app->add_set_ignore_case("--prescheduler", opt_name2opt_val["prescheduler"], {"no", "yes"}, "Run qasm (first) scheduler?", true);
          app->add_set_ignore_case("--scheduler_post179", opt_name2opt_val["scheduler_post179"], {"no", "yes"}, "Issue 179 solution included", true);
          app->add_set_ignore_case("--print_dot_graphs", opt_name2opt_val["print_dot_graphs"], {"no", "yes"}, "Print (un-)scheduled graphs in DOT format", true);
          app->add_set_ignore_case("--scheduler", opt_name2opt_val["scheduler"], {"ASAP", "ALAP"}, "scheduler type", true);
          app->add_set_ignore_case("--scheduler_uniform", opt_name2opt_val["scheduler_uniform"], {"yes", "no"}, "Do uniform scheduling or not", true);
          app->add_set_ignore_case("--scheduler_commute", opt_name2opt_val["scheduler_commute"], {"yes", "no"}, "Commute gates when possible, or not", true);
          app->add_set_ignore_case("--use_default_gates", opt_name2opt_val["use_default_gates"], {"yes", "no"}, "Use default gates or not", true);
          app->add_set_ignore_case("--optimize", opt_name2opt_val["optimize"], {"yes", "no"}, "optimize or not", true);
          app->add_set_ignore_case("--clifford_prescheduler", opt_name2opt_val["clifford_prescheduler"], {"yes", "no"}, "clifford optimize before prescheduler yes or not", true);
          app->add_set_ignore_case("--clifford_postscheduler", opt_name2opt_val["clifford_postscheduler"], {"yes", "no"}, "clifford optimize after prescheduler yes or not", true);
          app->add_set_ignore_case("--clifford_premapper", opt_name2opt_val["clifford_premapper"], {"yes", "no"}, "clifford optimize before mapping yes or not", true);
          app->add_set_ignore_case("--clifford_postmapper", opt_name2opt_val["clifford_postmapper"], {"yes", "no"}, "clifford optimize after mapping yes or not", true);
          app->add_set_ignore_case("--decompose_toffoli", opt_name2opt_val["decompose_toffoli"], {"no", "NC", "AM"}, "Type of decomposition used for toffoli", true);
          app->add_set_ignore_case("--quantumsim", opt_name2opt_val["quantumsim"], {"no", "yes", "qsoverlay"}, "Produce quantumsim output, and of which kind", true);
          app->add_set_ignore_case("--issue_skip_319", opt_name2opt_val["issue_skip_319"], {"no", "yes"}, "Issue skip instead of wait in bundles", true);
          app->add_option("--backend_cc_map_input_file", opt_name2opt_val["backend_cc_map_input_file"], "Name of CC input map file", true);
          app->add_set_ignore_case("--cz_mode", opt_name2opt_val["cz_mode"], {"manual", "auto"}, "CZ mode", true);

          app->add_set_ignore_case("--mapper", opt_name2opt_val["mapper"], {"no", "base", "baserc", "minextend", "minextendrc", "maxfidelity"}, "Mapper heuristic", true);
          app->add_set_ignore_case("--mapinitone2one", opt_name2opt_val["mapinitone2one"], {"no", "yes"}, "Initialize mapping of virtual qubits one to one to real qubits", true);
          app->add_set_ignore_case("--mapprepinitsstate", opt_name2opt_val["mapprepinitsstate"], {"no", "yes"}, "Prep gate leaves qubit in zero state", true);
          app->add_set_ignore_case("--mapassumezeroinitstate", opt_name2opt_val["assumezeroinitstate"], {"no", "yes"}, "Assume that qubits are initialized to zero state", true);
          app->add_set_ignore_case("--initialplace", opt_name2opt_val["initialplace"], {"no","yes","1s","10s","1m","10m","1h","1sx","10sx","1mx","10mx","1hx"}, "Initialplace qubits before mapping", true);
          app->add_set_ignore_case("--initialplace2qhorizon", opt_name2opt_val["initialplace2qhorizon"], {"0","1","2","3","4","5","6","7","8","9", "10","11","12","13","14","15","16","17","18","19","20","30","40","50","60","70","80","90","100"}, "Initialplace considers only this number of initial two-qubit gates", true);
          app->add_set_ignore_case("--maplookahead", opt_name2opt_val["maplookahead"], {"no", "1qfirst", "noroutingfirst", "all"}, "Strategy wrt selecting next gate(s) to map", true);
          app->add_set_ignore_case("--mappathselect", opt_name2opt_val["mappathselect"], {"all", "borders"}, "Which paths: all or borders", true);
          app->add_set_ignore_case("--mapselectswaps", opt_name2opt_val["mapselectswaps"], {"one", "all", "earliest"}, "Select only one swap, or earliest, or all swaps for one alternative", true);
          app->add_set_ignore_case("--maprecNN2q", opt_name2opt_val["maprecNN2q"], {"no","yes"}, "Recursing also on NN 2q gate?", true);
          app->add_set_ignore_case("--mapselectmaxlevel", opt_name2opt_val["mapselectmaxlevel"], {"0","1","2","3","4","5","6","7","8","9","10","inf"}, "Maximum recursion in selecting alternatives on minimum extension", true);
          app->add_set_ignore_case("--mapselectmaxwidth", opt_name2opt_val["mapselectmaxwidth"], {"min","minplusone","minplushalfmin","minplusmin","all"}, "Maximum width number of alternatives to enter recursion with", true);
          app->add_set_ignore_case("--maptiebreak", opt_name2opt_val["maptiebreak"], {"first", "last", "random", "critical"}, "Tie break method", true);
          app->add_set_ignore_case("--mapusemoves", opt_name2opt_val["mapusemoves"], {"no", "yes", "0","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16","17","18","19","20"}, "Use unused qubit to move thru", true);
          app->add_set_ignore_case("--mapreverseswap", opt_name2opt_val["mapreverseswap"], {"no", "yes"}, "Reverse swap operands when better", true);

          app->add_set_ignore_case("--write_qasm_files", opt_name2opt_val["write_qasm_files"], {"yes", "no"}, "write (un-)scheduled (with and without resource-constraint) qasm files", true);
          app->add_set_ignore_case("--write_report_files", opt_name2opt_val["write_report_files"], {"yes", "no"}, "write report files on circuit characteristics and pass results", true);
      }

  public:
      Options(std::string app_name="testApp")
      {
          app = new CLI::App(app_name);
          
          set_defaults();
      }

      void print_current_values()
      {
          std::cout << "log_level: " << opt_name2opt_val["log_level"] << std::endl
                    << "output_dir: " << opt_name2opt_val["output_dir"] << std::endl
                    << "unique_output: " << opt_name2opt_val["unique_output"] << std::endl
                    << "optimize: " << opt_name2opt_val["optimize"] << std::endl
                    << "use_default_gates: " << opt_name2opt_val["use_default_gates"] << std::endl
                    << "decompose_toffoli: " << opt_name2opt_val["decompose_toffoli"] << std::endl
                    << "quantumsim: " << opt_name2opt_val["quantumsim"] << std::endl
                    << "issue_skip_319: " << opt_name2opt_val["issue_skip_319"] << std::endl
                    << "clifford_prescheduler: " << opt_name2opt_val["clifford_prescheduler"] << std::endl
                    << "prescheduler: " << opt_name2opt_val["prescheduler"] << std::endl
                    << "scheduler: " << opt_name2opt_val["scheduler"] << std::endl
                    << "scheduler_uniform: " << opt_name2opt_val["scheduler_uniform"] << std::endl
                    << "clifford_postscheduler: " << opt_name2opt_val["clifford_postscheduler"] << std::endl
                    << "clifford_premapper: " << opt_name2opt_val["clifford_premapper"] << std::endl
                    << "mapper: "           << opt_name2opt_val["mapper"] << std::endl
                    << "mapinitone2one: "   << opt_name2opt_val["mapinitone2one"] << std::endl
                    << "initialplace: "     << opt_name2opt_val["initialplace"] << std::endl
                    << "initialplace2qhorizon: "<< opt_name2opt_val["initialplace2qhorizon"] << std::endl
                    << "maplookahead: "     << opt_name2opt_val["maplookahead"] << std::endl
                    << "mappathselect: "    << opt_name2opt_val["mappathselect"] << std::endl
                    << "maptiebreak: "      << opt_name2opt_val["maptiebreak"] << std::endl
                    << "mapusemoves: "      << opt_name2opt_val["mapusemoves"] << std::endl
                    << "mapreverseswap: "   << opt_name2opt_val["mapreverseswap"] << std::endl
                    << "mapselectswaps: "   << opt_name2opt_val["mapselectswaps"] << std::endl
                    << "clifford_postmapper: " << opt_name2opt_val["clifford_postmapper"] << std::endl
                    << "scheduler_post179: " << opt_name2opt_val["scheduler_post179"] << std::endl
                    << "scheduler_commute: " << opt_name2opt_val["scheduler_commute"] << std::endl
                    << "cz_mode: " << opt_name2opt_val["cz_mode"] << std::endl
                    << "write_qasm_files: " << opt_name2opt_val["write_qasm_files"] << std::endl
                    << "write_report_files: " << opt_name2opt_val["write_report_files"] << std::endl
                    << "print_dot_graphs: " << opt_name2opt_val["print_dot_graphs"] << std::endl;
          // FIXME: incomplete, function seems unused
      }

      void reset_options()
      {
          app = new CLI::App("testApp");
          
          set_defaults();
      }
      
      void help()
      {
          std::cout << app->help() << std::endl;
      }

      void set(std::string opt_name, std::string opt_value)
      {
          try
          {
            std::vector<std::string> opts = {opt_value, "--"+opt_name};
            app->parse(opts);
          }
          catch (const std::exception &e)
          {
            app->reset();
            EOUT("Un-known option:"<< e.what());
            throw ql::exception("Error parsing options. "+std::string(e.what())+" !",false);
          }
          app->reset();
      }

      std::string get(std::string opt_name)
      {
        std::string opt_value("UNKNOWN");
        if( opt_name2opt_val.find(opt_name) != opt_name2opt_val.end() )
        {
            opt_value = opt_name2opt_val[opt_name];
        }
        else
        {
            EOUT("Un-known option:"<< opt_name);
        }
        return opt_value;
      }
  };

  namespace options // FIXME: why wrap?
  {
//      static ql::Options ql_options("OpenQL Options");
      OPENQL_DECLSPEC extern ql::Options ql_options;
      inline void print()
      {
          ql_options.help();
      }
      inline void print_current_values()
      {
          ql_options.print_current_values();
      }
      inline void set(std::string opt_name, std::string opt_value)
      {
          ql_options.set(opt_name, opt_value);

          if(opt_name == "log_level")
              ql::utils::logger::set_log_level(opt_value);
          else if(opt_name == "output_dir")
              ql::utils::make_output_dir(opt_value);
      }
      inline std::string get(std::string opt_name)
      {
          return ql_options.get(opt_name);
      }
      inline void reset_options()
      {
          ql_options.reset_options();
      }
  } // namespace option
} // namespace ql

#endif

/*  FIXME: remove
        void set(std::string opt_name, std::string opt_value)
        {
            if( _options.find(opt_name) != _options.end() )
            {
                if(opt_name == "log_level")
                    ql::utils::logger::set_log_level(opt_value);
                else if(opt_name == "output_dir")
                    ql::utils::make_output_dir(opt_value);
                else if(opt_name == "scheduler" && opt_value == "ASAP" && opt_value == "ALAP")
                {

                }

                _options[opt_name] = opt_value;
            }
            else
            {
                std::cerr << "[OPENQL] " << __FILE__ <<":"<< __LINE__
                          <<" Error: Un-known option:"<< opt_name << std::endl;
            }
        }

        void print()
        {
            std::cout << "OpenQL Options:\n";
            for(auto elem : _options)
            {
                std::cout << "    " << elem.first << " : " << elem.second << "\n";
            }
        }
*/

