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
     * write IR as an independent pass
     * - write_ir(programp, platform, extension)
     *      writes the IR of each kernel; in bundles format when cycles_valid of each kernel
     *
     * reporting IR before ("in") and after ("out") executing a pass ("passname")
     * only when global option write_qasm_files is "yes".
     * - report_ir(programp, platform, in or out, passname):
     *      writes the IR of each kernel; in bundles format when cycles_valid of each kernel
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
     * write the IR
     * in a file with a name that contains the program unique name and the given extension
     */
    void write_ir(ql::quantum_program*          programp,
                const ql::quantum_platform&     platform,
                const std::string               extension
               );

    /*
     * reports the IR
     * in a file with a name that contains the program unique name and the place from where the report is done
     */
    void report_ir(ql::quantum_program*         programp,
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

} // ql namespace

#endif //QL_REPORT_H
