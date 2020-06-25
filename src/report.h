/**
 * @file   report.h
 * @date   08/2019
 * @author Hans van Someren
 * @brief  report utils
 */

#ifndef QL_REPORT_H
#define QL_REPORT_H

#include <platform.h>
#include <program.h>

namespace ql
{
    /*
     * write qasm as an independent pass
     * - write_qasm(programp, platform, pass_name)
     *      writes the qasm of each kernel; it is in bundles format only when cycles_valid of all kernels
     *
     * reporting qasm before ("in") and after ("out") executing a pass ("pass_name")
     * only when global option write_qasm_files is "yes".
     * - report_qasm(programp, platform, in or out, pass_name):
     *      writes qasm of each kernel; it is in bundles format only when cycles_valid of all kernels
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
     *
     * initialization
     * - report_init(programp, platform)
     *      initializes unique_name facility to have different file names for different compiler runs
     */

    /*
     * write qasm
     * in a file with a name that contains the program unique name and an extension defined by the pass_name
     */
    void write_qasm(ql::quantum_program*        programp,
                const ql::quantum_platform&     platform,
                const std::string               pass_name
               );

    /*
     * reports qasm
     * in a file with a name that contains the program unique name and the place from where the report is done
     */
    void report_qasm(ql::quantum_program*         programp,
                const ql::quantum_platform&     platform,
                const std::string               in_or_out,
                const std::string               pass_name
               );

    /*
     * create a report file for the given program and place, and open it
     * return an ofstream to it
     */
    std::ofstream report_open(ql::quantum_program* programp,
                const std::string               in_or_out,
                const std::string               pass_name
               );

    /*
     * close the report file again
     */
    void report_close(std::ofstream&           ofs);


    /*
     * report statistics of the circuit of the given kernel
     */
    void report_kernel_statistics(std::ofstream&           ofs,
                quantum_kernel&                 k,
                const ql::quantum_platform&     platform,
                const std::string               comment_prefix
               );

    /*
     * get statistics of the circuit of the given kernel for the mapper pass
     */
    void get_kernel_statistics(std::string*           ofs,
                quantum_kernel&                 k,
                const ql::quantum_platform&     platform,
                const std::string               comment_prefix
               );

    /*
     * report given string which is assumed to be closed by an endl by the caller
     */
    void report_string(std::ofstream&   ofs,
                std::string             s
               );

    /*
     * reports only the totals of the statistics of the circuits of the given kernels
     */
    void report_totals_statistics(std::ofstream&           ofs,
                std::vector<quantum_kernel>&    kernels,
                const ql::quantum_platform&     platform,
                const std::string               comment_prefix
               );

    /*
     * get only the totals of the statistics of the circuits of the given kernels
     */
    void get_totals_statistics(std::string*           ofs,
                std::vector<quantum_kernel>&    kernels,
                const ql::quantum_platform&     platform,
                const std::string               comment_prefix
               );
    
    /*
     * reports the statistics of the circuits of the given kernels individually and in total
     * by appending them to the report file of the given program and place from where the report is done;
     * this report file is first created/truncated
     *
     * report_statistics is used in the cases where there is no pass specific data to report
     * otherwise, the sequence of calls in here has to be copied and supplemented by some report_string calls
     */
    void report_statistics(ql::quantum_program* programp,
                const ql::quantum_platform&     platform,
                const std::string               in_or_out,
                const std::string               pass_name,
                const std::string               comment_prefix
               );

    void report_statistics(ql::quantum_program* programp,
                const ql::quantum_platform&     platform,
                const std::string               in_or_out,
                const std::string               pass_name,
                const std::string               comment_prefix,
                const std::string               additionalStatistics
               );
    
    /*
     * initialization of program.unique_name that is used by file name generation for reporting and printing
     * it is the program's name with a suffix appended that represents the number of the run of the program
     *
     * objective of this all is that of a later run of the same program, the output files don't overwrite the earlier ones
     *
     * do this only if unique_output option is set; if not, just use the program's name and let files overwrite
     * when set, maintain a seed with the run number; the first run is version 1; the first run uses the program's name
     * the second and later use the version (2 or larger) as suffix to the program's name
     */
    void report_init(ql::quantum_program*      programp,
                const ql::quantum_platform&    platform
               );

} // ql namespace

#endif //QL_REPORT_H
