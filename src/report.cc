/**
 * @file   report.cc
 * @date   08/2019
 * @author Hans van Someren
 * @brief  report utils
 */

#include <utils.h>
#include <options.h>
#include <ir.h>
#include <report.h>

namespace ql
{
    /*
     * write qasm as an independent pass
     * - write_qasm(programp, platform, extension)
     *      writes the qasm of each kernel; in bundles format when cycles_valid of each kernel
     *
     * reporting qasm before ("in") and after ("out") executing a pass ("passname")
     * only when global option write_qasm_files is "yes".
     * - report_qasm(programp, platform, in or out, passname):
     *      writes the qasm of each kernel; in bundles format when cycles_valid of each kernel
     *
     * reporting statistics before ("in") and after ("out") executing a pass ("pass_name")
     * only when option write_report_files is "yes":
     * - report_statistics(programp, platform, in or out, pass_name, comment_prefix):
     *      writes the standard statistics of each kernel, each line prefixed with the comment prefix;
     *      when more than the standard statistics must be printed,
     *      the 5 more primitive interfaces can be used, that are shown next;
     *      see the source code of report_statistics for an example how to call these
     *
     * - report_open: return an ofstream to a created/truncated report file
     *      the name of the report file is created for the given program and
     *      place from where the report is done
     * - report_kernel_statistics: add a report to the report file (of the ofstream)
     *      with the statistics of the given single kernel
     * - report_totals_statistics: add a report to the report file (of the ofstream)
     *      with the totals of the statistics of the given kernels
     * - report_string: add the given string to the ofstream
     * - report_close: close the report file's ofstream again
     */

    /*
     * support functions for reporting statistics
     */
    size_t  get_classical_operations_count(const circuit& c, const quantum_platform& platform)
    {
        size_t classical_operations = 0;
        // DOUT("... reporting get_classical_operations_count");
        for (auto & gp: c)
        {
            switch(gp->type())
            {
            case __classical_gate__:
                classical_operations++;
            case __wait_gate__:
                break;
            default:    // quantum gate
                break;
            }
        }
        // DOUT("... reporting get_classical_operations_count [done]");
        return classical_operations;
    }

    size_t  get_non_single_qubit_quantum_gates_count(const circuit& c, const quantum_platform& platform)
    {
        size_t quantum_gates = 0;
        // DOUT("... reporting get_non_single_qubit_quantum_gates_count");
        for (auto & gp: c)
        {
            switch(gp->type())
            {
            case __classical_gate__:
                break;
            case __wait_gate__:
                break;
            default:    // quantum gate
                if( gp->operands.size() > 1 )
                {
                    quantum_gates++;
                }
                break;
            }
        }
        // DOUT("... reporting get_non_single_qubit_quantum_gates_count [done]");
        return quantum_gates;
    }

    void  get_qubit_usecount(const circuit& c, const quantum_platform& platform, std::vector<size_t>& usecount)
    {
        // DOUT("... reporting get_qubit_usecount");
        for (auto & gp: c)
        {
            switch(gp->type())
            {
            case __classical_gate__:
            case __wait_gate__:
                break;
            default:    // quantum gate
                for (auto v: gp->operands)
                {
                    usecount[v]++;
                }
                break;
            }
        }
        // DOUT("... reporting get_qubit_usecount [done]");
        return;
    }

    void  get_qubit_usedcyclecount(const circuit& c, const quantum_platform& platform, std::vector<size_t>& usedcyclecount)
    {
        size_t  cycle_time = platform.cycle_time;

        // DOUT("... reporting get_qubit_usedcyclecount");
        for (auto & gp: c)
        {
            switch(gp->type())
            {
            case __classical_gate__:
            case __wait_gate__:
                break;
            default:    // quantum gate
                for (auto v: gp->operands)
                {
                    usedcyclecount[v] += (gp->duration+cycle_time-1)/cycle_time;
                }
                break;
            }
        }
        // DOUT("... reporting get_qubit_usedcyclecount [done]");
        return;
    }

    size_t  get_quantum_gates_count(const circuit& c, const quantum_platform& platform)
    {
        size_t quantum_gates = 0;
        // DOUT("... reporting get_quantum_gates_count");
        for (auto & gp: c)
        {
            switch(gp->type())
            {
            case __classical_gate__:
                break;
            case __wait_gate__:
                break;
            default:    // quantum gate
                quantum_gates++;
                break;
            }
        }
        // DOUT("... reporting get_quantum_gates_count [done]");
        return quantum_gates;
    }

    size_t  get_circuit_latency(const circuit& c, const quantum_platform& platform)
    {
        size_t  cycle_time = platform.cycle_time;
        size_t  circuit_latency_result;
        // DOUT("... reporting get_circuit_latency");
        if (c.size() < 1)
        {
            circuit_latency_result = 0;
            // DOUT("In get_circuit_latency() result is 0 because circuit is empty");
        }
        else if (c.back()->cycle == MAX_CYCLE)
        {
            circuit_latency_result = 0;
            // DOUT("In get_circuit_latency() result is 0 because c.back()->cycle == MAX_CYCLE");
        }
        else
        {
            circuit_latency_result = c.back()->cycle + (c.back()->duration+cycle_time-1)/cycle_time - c.front()->cycle;
        }
        // DOUT("Computed get_circuit_latency(): result is " << circuit_latency_result);
        return circuit_latency_result;
    }

    /*
     * writing out the circuits of the given kernels in qasm/bundles depending on whether cycles_valid for all kernels
     * - writing out in qasm writes the gates, one by one in order on separate lines and ignores the cycle attributes
     * - writing out as bundles writes the bundles, one by one in order on separate lines, each line representing a next cycle
     */
    void report_write_qasm(std::stringstream& fname, quantum_program* programp, const ql::quantum_platform& platform)
    {
        // DOUT("... reporting report_write_qasm");
        std::stringstream out_qasm;
        out_qasm << "version 1.0\n";
        out_qasm << "# this file has been automatically generated by the OpenQL compiler please do not modify it manually.\n";
        out_qasm << "qubits " << programp->qubit_count << "\n";

        bool    do_bundles = true;
        for(auto &kernel : programp->kernels)
        {
            do_bundles &= kernel.cycles_valid;
        }
        // DOUT("... reporting do_bundles=" << do_bundles);

        for(auto &kernel : programp->kernels)
        {
            if (do_bundles)
            {
                out_qasm << kernel.get_prologue();
                ql::ir::bundles_t bundles = ql::ir::bundler(kernel.c, platform.cycle_time);
                out_qasm << ql::ir::qasm(bundles);
                out_qasm << kernel.get_epilogue();
            }
            else
            {
                out_qasm << kernel.qasm();
            }
        }
        ql::utils::write_file(fname.str(), out_qasm.str());
        // DOUT("... reporting report_write_qasm [done]");
    }

    /*
     * composes the write file's name
     * that contains the program name and the extension
     */
    std::stringstream report_compose_write_name(const std::string   unique_name,
                const std::string               extension
               )
    {
        std::stringstream fname;
        fname << ql::options::get("output_dir") << "/"
          << unique_name << extension;
        return fname;
    }

    /*
     * write the qasm
     * in a file with a name that contains the program unique name and the given extension
     */
    void write_qasm_extension(ql::quantum_program*          programp,
                const ql::quantum_platform&     platform,
                const std::string               extension
               )
    {
        std::stringstream fname;
        fname = report_compose_write_name(programp->unique_name, extension);
        report_write_qasm(fname, programp, platform);
    }

    /*
     * write the qasm
     * in a file with a name that contains the program unique name and an extension defined by the pass_name
     */
    void write_qasm(ql::quantum_program*          programp,
                const ql::quantum_platform&     platform,
                const std::string               pass_name
               )
    {
        std::string       extension;
        // next is ugly; must be done by built-in pass class option with different value for each concrete pass
        if (pass_name == "initialqasmwriter" || pass_name == "outputIR") extension = ".qasm";
        else if (pass_name == "scheduledqasmwriter" || pass_name == "outputIRscheduled") extension = "_scheduled.qasm";
        else FATAL("write_qasm: pass_name " << pass_name << " unknown; don't know which extension to generate");

        write_qasm_extension(programp, platform, extension);
    }


    /*
     * composes the report file's name
     * that contains the program name and the place from where the report is done
     */
    std::stringstream report_compose_report_name(const std::string   unique_name,
                const std::string               in_or_out,
                const std::string               pass_name,
                const std::string               extension
               )
    {
        std::stringstream fname;
        fname << ql::options::get("output_dir") << "/"
          << unique_name << "_" << pass_name << "_" << in_or_out << "." << extension;
        return fname;
    }

    /*
     * reports the qasm
     * in a file with a name that contains the program name and the place from where the report is done
     */
    void report_qasm(quantum_program*           programp,
                const ql::quantum_platform&     platform,
                const std::string               in_or_out,
                const std::string               pass_name
               )
    {
        if( ql::options::get("write_qasm_files") == "yes")
        {
            std::stringstream fname;
            fname = report_compose_report_name(programp->unique_name, in_or_out, pass_name, "qasm");
            report_write_qasm(fname, programp, platform);
        }
    }

    /*
     * create a report file for the given program and place, and open it
     * return an ofstream to it
     */
    std::ofstream report_open(ql::quantum_program* programp,
                const std::string               in_or_out,
                const std::string               pass_name
               )
    {
        std::ofstream ofs;

        // DOUT("report_open: " << programp->unique_name << " in_or_out: " << in_or_out << " pass_name: " << pass_name);
        if( ql::options::get("write_report_files") != "yes")
        {
            ofs.setstate(std::ios::badbit);
            // DOUT("report_open no report [done]");
            return ofs;
        }
        std::stringstream fname;
        fname = report_compose_report_name(programp->unique_name, in_or_out, pass_name, "report");
        ofs.open(fname.str(), std::ofstream::trunc);
        if (ofs.fail())
        {
            FATAL("[x] error opening file '" << fname.str() << "' !" << std::endl
              << "    make sure the output directory exists for '" << fname.str() << "'" << std::endl);
        }
        // DOUT("report_open [done]");
        return ofs;
    }

    /*
     * close the report file again
     */
    void report_close(std::ofstream&           ofs)
    {
        // DOUT("report_close");
        if( ql::options::get("write_report_files") != "yes")
        {
            // DOUT("report_close no report [done]");
            return;
        }
        ofs.close();
        // DOUT("report_close [done]");
    }


    /*
     * report statistics of the circuit of the given kernel
     */
    void get_kernel_statistics(std::string* ofs,
                quantum_kernel&                 k,
                const ql::quantum_platform&     platform,
                const std::string               comment_prefix
               )
    {
        if( ql::options::get("write_report_files") != "yes")
        {
            return;
        }

        // DOUT("... reporting report_kernel_statistics");
        std::vector<size_t> usecount;
        usecount.resize(platform.qubit_number, 0);
        get_qubit_usecount(k.c, platform, usecount);
        size_t qubits_used = 0; for (auto v: usecount) { if (v != 0) { qubits_used++; } } 

        std::vector<size_t> usedcyclecount;
        usedcyclecount.resize(platform.qubit_number, 0);
        get_qubit_usedcyclecount(k.c, platform, usedcyclecount);

        size_t  circuit_latency = get_circuit_latency(k.c, platform);
        *ofs += comment_prefix; *ofs += "kernel: " ; *ofs += k.name ; *ofs += "\n";
        *ofs ; *ofs += comment_prefix ; *ofs += "----- circuit_latency: " ; *ofs += circuit_latency ; *ofs += "\n";
        *ofs ; *ofs += comment_prefix ; *ofs += "----- quantum gates: " ; *ofs += get_quantum_gates_count(k.c, platform) ; *ofs += "\n";
        *ofs += comment_prefix; *ofs += "----- non single qubit gates: " ; *ofs += get_non_single_qubit_quantum_gates_count(k.c, platform) ; *ofs += "\n";
        *ofs += comment_prefix; *ofs +=  "----- classical operations: "; *ofs += get_classical_operations_count(k.c, platform); *ofs +=  "\n";
        *ofs += comment_prefix; *ofs += "----- qubits used: "; *ofs += qubits_used; *ofs += "\n";
        *ofs += comment_prefix; *ofs += "----- qubit cycles use:"; *ofs += ql::utils::to_string(usedcyclecount); *ofs += "\n";
        
        // DOUT("... reporting report_kernel_statistics [done]");
    }

    /*
     * report statistics of the circuit of the given kernel
     */
    void report_kernel_statistics(std::ofstream&           ofs,
                quantum_kernel&                 k,
                const ql::quantum_platform&     platform,
                const std::string               comment_prefix
               )
    {
        if( ql::options::get("write_report_files") != "yes")
        {
            return;
        }

        // DOUT("... reporting report_kernel_statistics");
        std::vector<size_t> usecount;
        usecount.resize(platform.qubit_number, 0);
        get_qubit_usecount(k.c, platform, usecount);
        size_t qubits_used = 0; for (auto v: usecount) { if (v != 0) { qubits_used++; } } 

        std::vector<size_t> usedcyclecount;
        usedcyclecount.resize(platform.qubit_number, 0);
        get_qubit_usedcyclecount(k.c, platform, usedcyclecount);

        size_t  circuit_latency = get_circuit_latency(k.c, platform);
        ofs << comment_prefix << "kernel: " << k.name << "\n";
        ofs << comment_prefix << "----- circuit_latency: " << circuit_latency << "\n";
        ofs << comment_prefix << "----- quantum gates: " << get_quantum_gates_count(k.c, platform) << "\n";
        ofs << comment_prefix << "----- non single qubit gates: " << get_non_single_qubit_quantum_gates_count(k.c, platform) << "\n";
        ofs << comment_prefix << "----- classical operations: " << get_classical_operations_count(k.c, platform) << "\n";
        ofs << comment_prefix << "----- qubits used: " << qubits_used << "\n";
        ofs << comment_prefix << "----- qubit cycles use:" << ql::utils::to_string(usedcyclecount) << "\n";
        // DOUT("... reporting report_kernel_statistics [done]");
    }

    /*
     * report given string which is assumed to be closed by an endl by the caller
     */
    void report_string(std::ofstream&   ofs,
                std::string             s
               )
    {
        // DOUT("... reporting string");
        if( ql::options::get("write_report_files") != "yes")
        {
            // DOUT("... reporting string no report [done]");
            return;
        }

        ofs << s;
        // DOUT("... reporting string [done]");
    }

    /*
     * reports only the total statistics of the circuits of the given kernels
     */
    void report_totals_statistics(std::ofstream&           ofs,
                std::vector<quantum_kernel>&    kernels,
                const ql::quantum_platform&     platform,
                const std::string               comment_prefix
               )
    {
        if( ql::options::get("write_report_files") != "yes")
        {
            return;
        }

        // DOUT("... reporting report_totals_statistics");
        // totals reporting, collect info from all kernels
        std::vector<size_t> usecount;
        usecount.resize(platform.qubit_number, 0);
        size_t total_circuit_latency = 0;
        size_t total_classical_operations = 0;
        size_t total_quantum_gates = 0;
        size_t total_non_single_qubit_gates= 0;
        for (auto& k : kernels)
        {
            get_qubit_usecount(k.c, platform, usecount);

            total_circuit_latency += get_circuit_latency(k.c, platform);
            total_classical_operations += get_classical_operations_count(k.c, platform);
            total_quantum_gates += get_quantum_gates_count(k.c, platform);
            total_non_single_qubit_gates += get_non_single_qubit_quantum_gates_count(k.c, platform);
        }
        size_t qubits_used = 0; for (auto v: usecount) { if (v != 0) { qubits_used++; } } 

        // report totals
        ofs << "\n";
        ofs << comment_prefix << "Total circuit_latency: " << total_circuit_latency << "\n";
        ofs << comment_prefix << "Total no. of quantum gates: " << total_quantum_gates << "\n";
        ofs << comment_prefix << "Total no. of non single qubit gates: " << total_non_single_qubit_gates << "\n";
        ofs << comment_prefix << "Total no. of classical operations: " << total_classical_operations << "\n";
        ofs << comment_prefix << "Qubits used: " << qubits_used << "\n";
        ofs << comment_prefix << "No. kernels: " << kernels.size() << "\n";
        // DOUT("... reporting report_totals_statistics [done]");
    }

    /*
     * reports only the total statistics of the circuits of the given kernels
     */
    void get_totals_statistics(std::string*           ofs,
                std::vector<quantum_kernel>&    kernels,
                const ql::quantum_platform&     platform,
                const std::string               comment_prefix
               )
    {
        if( ql::options::get("write_report_files") != "yes")
        {
            return;
        }

        // DOUT("... reporting report_totals_statistics");
        // totals reporting, collect info from all kernels
        std::vector<size_t> usecount;
        usecount.resize(platform.qubit_number, 0);
        size_t total_circuit_latency = 0;
        size_t total_classical_operations = 0;
        size_t total_quantum_gates = 0;
        size_t total_non_single_qubit_gates= 0;
        for (auto& k : kernels)
        {
            get_qubit_usecount(k.c, platform, usecount);

            total_circuit_latency += get_circuit_latency(k.c, platform);
            total_classical_operations += get_classical_operations_count(k.c, platform);
            total_quantum_gates += get_quantum_gates_count(k.c, platform);
            total_non_single_qubit_gates += get_non_single_qubit_quantum_gates_count(k.c, platform);
        }
        size_t qubits_used = 0; for (auto v: usecount) { if (v != 0) { qubits_used++; } } 

        // report totals
        *ofs += "\n";
        *ofs += comment_prefix; *ofs += "Total circuit_latency: "; *ofs += total_circuit_latency; *ofs += "\n";
        *ofs += comment_prefix; *ofs += "Total no. of quantum gates: "; *ofs += total_quantum_gates; *ofs += "\n";
        *ofs += comment_prefix; *ofs += "Total no. of non single qubit gates: "; *ofs += total_non_single_qubit_gates; *ofs += "\n";
        *ofs += comment_prefix; *ofs += "Total no. of classical operations: "; *ofs += total_classical_operations; *ofs += "\n";
        *ofs += comment_prefix; *ofs += "Qubits used: "; *ofs += qubits_used; *ofs += "\n";
        *ofs += comment_prefix; *ofs += "No. kernels: "; *ofs += kernels.size(); *ofs += "\n";
        // DOUT("... reporting report_totals_statistics [done]");
    }
    
    /*
     * reports the statistics of the circuits of the given kernels individually and in total
     * by appending them to the report file of the given program and place from where the report is done;
     * this report file is first created/truncated
     *
     * report_statistics is used in the cases where there is no pass specific data to report
     * otherwise, the sequence of calls in here has to be copied and supplemented by some report_string calls
     */
    void report_statistics(quantum_program*     programp,
                const ql::quantum_platform&     platform,
                const std::string               in_or_out,
                const std::string               pass_name,
                const std::string               comment_prefix
               )
    {
        if( ql::options::get("write_report_files") != "yes")
        {
            return;
        }
        
        // DOUT("... reporting report_statistics");
        std::ofstream   ofs;
        ofs = report_open(programp, in_or_out, pass_name);

        // per kernel reporting
        for (auto& k : programp->kernels)
        {
            report_kernel_statistics(ofs, k, platform, comment_prefix);
        }

        // and total collecting and reporting
        report_totals_statistics(ofs, programp->kernels, platform, comment_prefix);
        report_close(ofs);
        // DOUT("... reporting report_statistics [done]");
    }
    
    void report_statistics(ql::quantum_program* programp,
                const ql::quantum_platform&     platform,
                const std::string               in_or_out,
                const std::string               pass_name,
                const std::string               comment_prefix,
                const std::string               additionalStatistics
               )
    {
        if( ql::options::get("write_report_files") != "yes")
        {
            return;
        }
        
        // DOUT("... reporting report_statistics");
        std::ofstream   ofs;
        ofs = report_open(programp, in_or_out, pass_name);

        // per kernel reporting
        for (auto& k : programp->kernels)
        {
            report_kernel_statistics(ofs, k, platform, comment_prefix);
        }

        // and total collecting and reporting
        report_totals_statistics(ofs, programp->kernels, platform, comment_prefix);
        
        ofs << " \n\n" << additionalStatistics;
        
        report_close(ofs);
        // DOUT("... reporting report_statistics [done]");
    }
    

    /*
     * support a unique file called 'get("output_dir")/name.unique'
     * it is a seed to create unique output files (qasm, report, etc.) for the same program (with name 'name')
     * - when the unique file is not there, it is created with the value 0 (which is then the current value)
     *   otherwise, it just reads the current value from that file
     * - it then increments the current value by 1, stores it in the file and returns this value
     * since this may be the first time that the output_dir is used, it warns when that doesn't exist
     */
    int report_bump_unique_file_version(ql::quantum_program* programp)
    {
        std::stringstream ss_unique;
        ss_unique << ql::options::get("output_dir") << "/" << programp->name << ".unique";
    
        std::fstream ufs;
        int vers;
    
        // retrieve old version number
        ufs.open (ss_unique.str(), std::fstream::in);
        if (!ufs.is_open())
        {
            // no file there, initialize old version number to 0
            ufs.open(ss_unique.str(), std::fstream::out);
            if (!ufs.is_open())
            {
                FATAL("Cannot create: " << ss_unique.str() << ". Probably output directory " << ql::options::get("output_dir") << " does not exist");
            }
            ufs << 0 << std::endl;
            vers = 0;
        }
        else
        {
            // read stored number
            ufs >> vers;
        }
        ufs.close();
    
        // increment to get new one, store it for a later run (so in a file) and return
        vers++;
        ufs.open(ss_unique.str(), std::fstream::out);
        ufs << vers << std::endl;
        ufs.close();
    
        return vers;
    }

    /*
     * initialization of program.unique_name that is used by file name generation for reporting and printing
     * it is the program's name with a suffix appended that represents the number of the run of the program
     *
     * objective of this all is that of a later run of the same program, the output files don't overwrite the earlier ones
     *
     * do this only if unique_output option is set; if not, just use the program's name and let files overwrite
     * when set, maintain a seed with the run number; the first run is version 1; the first run uses the program's name;
     * the second and later use the version (2 or larger) as suffix to the program's name
     */
    void report_init(ql::quantum_program*      programp,
                const ql::quantum_platform&    platform
               )
    {
	    programp->unique_name = programp->name;
	    if (ql::options::get("unique_output") == "yes")
	    {
	        int vers;
	        vers = report_bump_unique_file_version(programp);
	        if (vers > 1)
	        {
	            programp->unique_name = ( programp->name + std::to_string(vers) );
	            DOUT("Unique program name after bump_unique_file_version: " << programp->unique_name << " based on version: " << vers);
	        }
	    }
    }

} // ql namespace
