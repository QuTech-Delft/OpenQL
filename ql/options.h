/**
 * @file   options.h
 * @date   05/2018
 * @author Imran Ashraf
 * @brief  Options class implementing parsing of supported options
 */

#ifndef QL_OPTIONS_H
#define QL_OPTIONS_H

#include <ql/utils.h>
#include <ql/exception.h>

#include <CLI/CLI.hpp>
#include <iostream>

namespace ql
{
  class Options
  {
  private:
      CLI::App * app;
      std::map<std::string, std::string> opt_name2opt_val;

  public:
      Options(std::string app_name="testApp")
      {
          app = new CLI::App(app_name);

          // default values
          opt_name2opt_val["log_level"] = "LOG_NOTHING";
          opt_name2opt_val["output_dir"] = "test_output";
          opt_name2opt_val["optimize"] = "no";
          opt_name2opt_val["use_default_gates"] = "yes";
          opt_name2opt_val["optimize"] = "no";
          opt_name2opt_val["decompose_toffoli"] = "no";
          opt_name2opt_val["scheduler"] = "ALAP";
          opt_name2opt_val["scheduler_uniform"] = "no";
          opt_name2opt_val["scheduler_commute"] = "no";
          opt_name2opt_val["scheduler_post179"] = "yes";
          opt_name2opt_val["backend_cc_map_input_file"] = "";


          // add options with default values and list of possible values
          app->add_set_ignore_case("--log_level", opt_name2opt_val["log_level"],
            {"LOG_NOTHING", "LOG_CRITICAL", "LOG_ERROR", "LOG_WARNING", "LOG_INFO", "LOG_DEBUG"}, "Log levels", true);
          app->add_option("--output_dir", opt_name2opt_val["output_dir"], "Name of output directory", true);
          app->add_set_ignore_case("--scheduler_post179", opt_name2opt_val["scheduler_post179"], {"no", "yes"}, "Issue 179 solution included", true);
          app->add_set_ignore_case("--scheduler", opt_name2opt_val["scheduler"], {"ASAP", "ALAP"}, "scheduler type", true);
          app->add_set_ignore_case("--scheduler_uniform", opt_name2opt_val["scheduler_uniform"], {"yes", "no"}, "Do uniform scheduling or not", true);
          app->add_set_ignore_case("--scheduler_commute", opt_name2opt_val["scheduler_commute"], {"yes", "no"}, "Commute gates when possible, or not", true);
          app->add_set_ignore_case("--use_default_gates", opt_name2opt_val["use_default_gates"], {"yes", "no"}, "Use default gates or not", true);
          app->add_set_ignore_case("--optimize", opt_name2opt_val["optimize"], {"yes", "no"}, "optimize or not", true);
          app->add_set_ignore_case("--decompose_toffoli", opt_name2opt_val["decompose_toffoli"], {"no", "NC", "MA"}, "Type of decomposition used for toffoli", true);
          app->add_option("--backend_cc_map_input_file", opt_name2opt_val["backend_cc_map_input_file"], "Name of CC input map file", true);
      }

      void print_current_values()
      {
          std::cout << "optimize: " << opt_name2opt_val["optimize"] << std::endl
                    << "scheduler: " << opt_name2opt_val["scheduler"] << std::endl
                    << "scheduler_uniform: " << opt_name2opt_val["scheduler_uniform"] << std::endl
                    << "scheduler_post179: " << opt_name2opt_val["scheduler_post179"] << std::endl
                    << "scheduler_commute: " << opt_name2opt_val["scheduler_uniform"] << std::endl;
          // FIXME: incomplete, function seems unused
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
      extern ql::Options ql_options;
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
  } // namespace option
} // namespace ql

#endif

/*
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

