/**
 * @file   report.h
 * @date   08/2019
 * @author Hans van Someren
 * @brief  report utils
 */

#ifndef QL_REPORT_H
#define QL_REPORT_H

#include <utils.h>
#include <platform.h>
#include <options.h>
#include <kernel.h>
#include <ir.h>
// #include "metrics.h"
// #include <metrics.h>
extern double ql::quick_fidelity_circuit(const ql::circuit &circuit);



namespace ql
{
namespace metrics
{
	extern double quick_fidelity_circuit(const ql::circuit & circuit );
}
namespace report
{

    /*
     * reporting statistics external interfaces
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
     *
     * - report_statistics: open, report for each kernel, report the totals and close
     * Each of above interfaces does nothing when option write_report_files is not "yes".
     *
     * - report_qasm: open, write qasm for each kernel, and close
     * - report_bundles: open, write qasm for each kernel, and close
     * Each of above two interfaces does nothing when option write_qasm_files is not "yes".
     * The latter are designed with the same list of parameters as report_statistics,
     * so that easily at each report place also the corresponding qasm can be written.
     *
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

    size_t  get_depth(const circuit& c, const quantum_platform& platform)
    {
        size_t  cycle_time = platform.cycle_time;
        size_t  depth_result;
        // DOUT("... reporting get_depth");
        if (c.size() < 1)
        {
            depth_result = 0;
            // DOUT("In get_depth() result is 0 because circuit is empty");
        }
        else if (c.back()->cycle == MAX_CYCLE)
        {
            depth_result = 0;
            // DOUT("In get_depth() result is 0 because c.back()->cycle == MAX_CYCLE");
        }
        else
        {
            depth_result = c.back()->cycle + (c.back()->duration+cycle_time-1)/cycle_time - c.front()->cycle;
        }
        // DOUT("Computed get_depth(): result is " << depth_result);
        return depth_result;
    }

    /*
     * writing out the circuits of the given kernels in qasm
     * this writes the gates, one by one in order on separate lines
     * and ignores the cycle attributes
     */
    void report_write_qasm(std::stringstream& fname, std::vector<quantum_kernel>& kernels, const ql::quantum_platform& platform)
    {
        stringstream out_qasm;
        // DOUT("... reporting report_write_qasm");
        out_qasm << "version 1.0\n";
        out_qasm << "# this file has been automatically generated by the OpenQL compiler please do not modify it manually.\n";
        out_qasm << "qubits " << platform.qubit_number << "\n";
        out_qasm << "\n";
        for(auto &kernel : kernels)
        {
            out_qasm << kernel.qasm();
        }
        out_qasm << "\n";
        ql::utils::write_file(fname.str(), out_qasm.str());
        // DOUT("... reporting report_write_qasm [done]");
    }

    /*
     * writing out the circuits of the given kernels as bundles
     * this writes the bundles, one by one in order on separate lines
     * bundling must have been done, and the cycle attributes of bundles and gates must be valid
     */
    void report_write_bundles(std::stringstream& fname, std::vector<quantum_kernel>& kernels, const ql::quantum_platform& platform)
    {
        stringstream out_qasm;
        // DOUT("... reporting report_write_bundles");
        out_qasm << "version 1.0\n";
        out_qasm << "# this file has been automatically generated by the OpenQL compiler please do not modify it manually.\n";
        out_qasm << "qubits " << platform.qubit_number << "\n";
        for(auto &kernel : kernels)
        {
            out_qasm << "\n" << kernel.get_prologue();
            out_qasm << ql::ir::qasm(kernel.bundles);
            out_qasm << kernel.get_epilogue();
        }
        out_qasm << "\n";
        ql::utils::write_file(fname.str(), out_qasm.str());
        // DOUT("... reporting report_write_bundles [done]");
    }

    /*
     * composes the report file's name
     * that contains the program name and the place from where the report is done
     */
    std::stringstream report_compose_name(const std::string   prog_name,
                const std::string               fromwhere_rel,
                const std::string               fromwhere_abs,
                const std::string               what
               )
    {
        std::stringstream fname;
        fname << ql::options::get("output_dir") << "/"
          << prog_name << "_" << fromwhere_abs << "_" << fromwhere_rel << "." << what;
        return fname;
    }

    /*
     * reports the circuits of the given kernels as qasm
     * in a file with a name that contains the program name and the place from where the report is done
     */
    void report_qasm(const std::string          prog_name,
                std::vector<quantum_kernel>&    kernels,
                const ql::quantum_platform&     platform,
                const std::string               fromwhere_rel,
                const std::string               fromwhere_abs
               )
    {
        if( ql::options::get("write_qasm_files") == "yes")
        {
            std::stringstream fname;
            fname = report_compose_name(prog_name, fromwhere_rel, fromwhere_abs, "qasm");
            report_write_qasm(fname, kernels, platform);
        }
    }

    /*
     * reports the circuits of the given kernels as bundles
     * in a file with a name that contains the program name and the place from where the report is done
     */
    void report_bundles(const std::string       prog_name,
                std::vector<quantum_kernel>&    kernels,
                const ql::quantum_platform&     platform,
                const std::string               fromwhere_rel,
                const std::string               fromwhere_abs
               )
    {
        if( ql::options::get("write_qasm_files") == "yes")
        {
            std::stringstream fname;
            fname = report_compose_name(prog_name, fromwhere_rel, fromwhere_abs, "qasm");
            report_write_bundles(fname, kernels, platform);
        }
    }

    /*
     * create a report file for the given program and place, and open it
     * return an ofstream to it
     */
    std::ofstream report_open(const std::string    prog_name,
                const std::string               fromwhere_rel,
                const std::string               fromwhere_abs
               )
    {
        std::ofstream ofs;

        // DOUT("report_open: " << prog_name << " rel: " << fromwhere_rel << " abs: " << fromwhere_abs);
        if( ql::options::get("write_report_files") != "yes")
        {
            ofs.setstate(ios::badbit);
            // DOUT("report_open no report [done]");
            return ofs;
        }
        std::stringstream fname;
        fname = report_compose_name(prog_name, fromwhere_rel, fromwhere_abs, "report");
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
    void report_kernel_statistics(std::ofstream&           ofs,
                quantum_kernel&                 k,
                const ql::quantum_platform&     platform,
                const std::string               prefix
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

        size_t  depth = get_depth(k.c, platform);
        ofs << prefix << "kernel: " << k.name << "\n";
        ofs << prefix << "----- depth: " << depth << "\n";
        ofs << prefix << "----- quantum gates: " << get_quantum_gates_count(k.c, platform) << "\n";
        ofs << prefix << "----- non single qubit gates: " << get_non_single_qubit_quantum_gates_count(k.c, platform) << "\n";
        ofs << prefix << "----- classical operations: " << get_classical_operations_count(k.c, platform) << "\n";
        ofs << prefix << "----- qubits used: " << qubits_used << "\n";
        ofs << prefix << "----- qubit cycles use:" << ql::utils::to_string(usedcyclecount) << "\n";

		std::string maxfidelity_1qbgatefid = ql::options::get("maxfidelity_1qbgatefid");
		std::string maxfidelity_2qbgatefid = ql::options::get("maxfidelity_2qbgatefid");
		std::string maxfidelity_idlefid = ql::options::get("maxfidelity_idlefid");
		std::string maxfidelity_outputmode = ql::options::get("maxfidelity_outputmode");

		ql::options::set("maxfidelity_1qbgatefid", "0.999");
		ql::options::set("maxfidelity_2qbgatefid", "0.9968");
		ql::options::set("maxfidelity_idlefid", "0.9991");
		ql::options::set("maxfidelity_outputmode", "product");
        ofs << prefix << "----- Metrics Score1:" << ql::utils::to_string(ql::quick_fidelity_circuit(k.c)) << "\n";

		ql::options::set("maxfidelity_1qbgatefid", "0.999");
		ql::options::set("maxfidelity_2qbgatefid", "0.99");
		ql::options::set("maxfidelity_idlefid", "0.999334");
		ql::options::set("maxfidelity_outputmode", "product");
        ofs << prefix << "----- Metrics Score2:" << ql::utils::to_string(ql::quick_fidelity_circuit(k.c)) << "\n";

		ql::options::set("maxfidelity_1qbgatefid", "0.999");
		ql::options::set("maxfidelity_2qbgatefid", "0.99");
		ql::options::set("maxfidelity_idlefid", "0.9867");
		ql::options::set("maxfidelity_outputmode", "product");
        ofs << prefix << "----- Metrics Score3:" << ql::utils::to_string(ql::quick_fidelity_circuit(k.c)) << "\n";


		ql::options::set("maxfidelity_1qbgatefid", maxfidelity_1qbgatefid);
		ql::options::set("maxfidelity_2qbgatefid", maxfidelity_2qbgatefid);
		ql::options::set("maxfidelity_idlefid", maxfidelity_idlefid);
		ql::options::set("maxfidelity_outputmode", maxfidelity_outputmode);
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
                const std::string               prefix
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
        size_t total_depth = 0;
        size_t total_classical_operations = 0;
        size_t total_quantum_gates = 0;
        size_t total_non_single_qubit_gates= 0;
        for (auto& k : kernels)
        {
            get_qubit_usecount(k.c, platform, usecount);

            total_depth += get_depth(k.c, platform);
            total_classical_operations += get_classical_operations_count(k.c, platform);
            total_quantum_gates += get_quantum_gates_count(k.c, platform);
            total_non_single_qubit_gates += get_non_single_qubit_quantum_gates_count(k.c, platform);
        }
        size_t qubits_used = 0; for (auto v: usecount) { if (v != 0) { qubits_used++; } } 

        // report totals
        ofs << "\n";
        ofs << prefix << "Total depth: " << total_depth << "\n";
        ofs << prefix << "Total no. of quantum gates: " << total_quantum_gates << "\n";
        ofs << prefix << "Total no. of non single qubit gates: " << total_non_single_qubit_gates << "\n";
        ofs << prefix << "Total no. of classical operations: " << total_classical_operations << "\n";
        ofs << prefix << "Qubits used: " << qubits_used << "\n";
        ofs << prefix << "No. kernels: " << kernels.size() << "\n";
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
    void report_statistics(const std::string    prog_name,
                std::vector<quantum_kernel>&    kernels,
                const ql::quantum_platform&     platform,
                const std::string               fromwhere_rel,
                const std::string               fromwhere_abs,
                const std::string               prefix
               )
    {
        if( ql::options::get("write_report_files") != "yes")
        {
            return;
        }
        
        // DOUT("... reporting report_statistics");
        std::ofstream   ofs;
        ofs = report_open(prog_name, fromwhere_rel, fromwhere_abs);

        // per kernel reporting
        for (auto& k : kernels)
        {
            report_kernel_statistics(ofs, k, platform, prefix);
        }

        // and total collecting and reporting
        report_totals_statistics(ofs, kernels, platform, prefix);
        report_close(ofs);
        // DOUT("... reporting report_statistics [done]");
    }

} // report namespace
} // ql namespace

#endif //QL_REPORT_H
