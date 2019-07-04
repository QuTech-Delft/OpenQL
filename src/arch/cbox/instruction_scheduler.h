/**
 * @file   instruction_scheduler.h
 * @date   07/2017
 * @author Nader Khammassi
 * @brief  instruction scheduler
 */

#ifndef QL_INSTRUCTION_SCHEDULER_H
#define QL_INSTRUCTION_SCHEDULER_H

#include <iostream>
#include <iomanip>
#include <sstream>

#include <src/arch/cbox/cbox_eqasm_compiler.h>

namespace ql
{
   namespace arch
   {
      class instruction_scheduler
      {
         public:

            /**
             *
             */
            instruction_scheduler()
            {
            }
      };

      /*  hardware ressource type definition  */
      typedef size_t                   channel_t;
      typedef std::string              label_t;
      typedef std::string              color_t;
      typedef std::vector<std::string> channels_t;

      typedef enum 
      {
         __center_pos__,
         __top_pos__   ,
         __bottom_pos__
      } position_t;

      /**
       * instruction trace
       */
      typedef struct
      {
         channel_t  channel;
         label_t    label;
         size_t     start;
         size_t     end;
         color_t    color;
         position_t position;
      } instruction_trace_t;

      typedef std::vector<instruction_trace_t>  instruction_traces_t;

      /**
       * time diagram class
       */
      class time_diagram
      {
         public:

            instruction_traces_t traces;
            channels_t           channels;
            size_t               exec_time;
            size_t               time_step;

         public:

            /**
             * time diagram
             */
            time_diagram(channels_t channels, size_t exec_time, size_t time_step) : channels(channels), exec_time(exec_time), time_step(time_step)
            {
            }

            /**
             * add trace 
             */
            void add_trace(channel_t channel, size_t start, size_t end, std::string label="", color_t color= "#EEEEEE", position_t pos = __center_pos__)
            {
               instruction_trace_t trace = { channel, label, start, end, color, pos };
               add_trace(trace);
            }

            /**
             * add instruction
             */
            void add_trace(instruction_trace_t& trace)
            {
               traces.push_back(trace);
            }

            /**
             * dump
             */
            void dump(std::string file_name="")
            {
               std::stringstream ss;
               // chart properties 
               ss << "{\"chart\":" << charts() << ",\n"; 
               // categories
               ss << start_categories() << "\n";
               ss << categories() << "\n";
               // processes
               ss << processes() << "\n";
               // instructions
               // ss << "\"tasks\": { \"showlabels\": \"1\", \"task\": [\n";
               ss << "\"tasks\": { \"task\": [\n";
               size_t sz = traces.size()-1;
               for (instruction_trace_t t : traces)
               {
                  json j; 
                  to_json(j,t);
                  ss << j << (sz ? "," : "");
                  sz--; 
               }
               ss << "]}}\n";  // end
               std::string trace_data = ss.str();
               if (file_name == "")
                  std::cout << ss.str() << std::endl;
               else
                  utils::write_file(file_name,trace_data);
            }

            void to_json(json& j, const instruction_trace_t& t) 
            {
               j = {
                  { "processid" , std::string(channels.at(t.channel)) },
                  { "start", format_time(t.start) },
                  { "end" , format_time(t.end) },
                  { "label", t.label },
                  { "color", t.color },
                  { "height", "25%" } ,
                  { "toppadding", (t.position == __center_pos__ ? "60%" : ( t.position == __top_pos__ ? "30%" : "60%")) },
               };
            }


         private:

            std::string format_time(size_t time)
            {
               std::stringstream ss;
               size_t hh = time/(3600); 
               size_t mn = (time%(3600))/60; 
               size_t sc = time%60;
               ss << std::setfill('0') << std::setw(2) << hh << ':' 
                  << std::setfill('0') << std::setw(2) << mn << ':' 
                  << std::setfill('0') << std::setw(2) << sc;
               return ss.str();
            }

            json charts()
            {
               json js = { 
                  { "dateformat" , "dd/mm/yyyy" },
                  { "outputdateformat" , "ss" },
                  { "caption" , "OpenQL Quantum Instructions Schedule" },
                  { "subCaption" , "QuMis Instruction Traces" },
                  { "canvasBorderAlpha" , "30"} ,
                  { "ganttPaneDuration", "1"},
                  { "ganttPaneDurationUnit", "mn"},
                  { "theme" , "fint" }
               };
               return js;
            }

            std::string start_categories()
            {
               std::stringstream ss;
               ss << "\"categories\": [" <<
                  "{" <<
                  "\"category\": [" <<
                  "{" <<
                  "\"start\": \"00:00:00\"," <<
                  "\"end\": \"" << format_time(exec_time) << "\"," <<
                  "\"label\": \"Time (Clock Cycles)\"" <<
                  "}]" <<
                  "},{" <<
                  "\"align\": \"left\"," <<
                  "\"category\": ["; 
               return ss.str();
            } 

            std::string categories()
            {
               std::stringstream ss;
               for (size_t i=0; i<(exec_time-time_step); i += time_step)
               {
                  json j = {
                     { "start" , format_time(i) },
                     { "end", format_time(i+time_step) },
                     { "label", std::to_string(i) }
                     // { "showTaskStartDate", "1" },
                     // { "showTaskEndDate", "1" }
                  };
                  ss << j << ",";
               }
               json j = {
                  { "start" , format_time(exec_time-time_step) },
                  { "end", format_time(exec_time) },
                  { "label", std::to_string((exec_time-time_step)) }
               };
               ss << j << "]}],";
               return ss.str();
            }


            std::string processes()
            {
               std::stringstream ss;
               ss << "\"processes\": { \"fontsize\": \"12\", \"isbold\": \"1\", \"align\": \"left\", \"headertext\": \"Channels\", \"headerfontsize\": \"14\", \"headervalign\": \"middle\", \"headeralign\": \"left\", \"process\": [";
               // print channels
               size_t sz = channels.size()-1;
               for (std::string ch : channels)
               {
                  json j = {
                     { "label", ch },
                     {    "id", ch }
                  };
                  ss << j << (sz ? "," : "");
                  sz--;
               }
               ss << "]},";
               return ss.str();
            }

            std::string start_tasks()
            {
               return "\"tasks\": { \"showlabels\": \"1\", \"task\": [";
            }
      };
   }
}

#endif // QL_INSTRUCTION_SCHEDULER_H
 
