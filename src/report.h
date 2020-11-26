/** \file
 * Utilities for writing report files.
 *
 * \see report.cc
 */

#pragma once

#include "utils/opt.h"
#include "utils/str.h"
#include "utils/vec.h"
#include "utils/filesystem.h"
#include "platform.h"
#include "program.h"

namespace ql {

/**
 * Wraps OutFile such that the file is only created and written if the
 * write_report_files option is set.
 */
class ReportFile {
private:
    utils::Opt<utils::OutFile> of;
public:
    ReportFile(
        const quantum_program *programp,
        const utils::Str &in_or_out,
        const utils::Str &pass_name
    );
    void write(const utils::Str &content);
    void write_kernel_statistics(
        const quantum_kernel &k,
        const quantum_platform &platform,
        const utils::Str &comment_prefix=""
    );
    void write_totals_statistics(
        const utils::Vec<quantum_kernel> &kernels,
        const quantum_platform &platform,
        const utils::Str &comment_prefix=""
    );
    void close();
    template <typename T>
    ReportFile &operator<<(T &&rhs) {
        if (of) {
            *of << std::forward<T>(rhs);
        }
        return *this;
    }
};

/**
 * write qasm
 * in a file with a name that contains the program unique name and an extension defined by the pass_name
 */
void write_qasm(
    const quantum_program *programp,
    const quantum_platform &platform,
    const utils::Str &pass_name
);

/**
 * reports qasm
 * in a file with a name that contains the program unique name and the place from where the report is done
 */
void report_qasm(
    const quantum_program *programp,
    const quantum_platform &platform,
    const utils::Str &in_or_out,
    const utils::Str &pass_name
);

/**
 * report given string which is assumed to be closed by an endl by the caller
 */
void report_string(
    std::ostream &os,
    const utils::Str &s
);

/**
 * report statistics of the circuit of the given kernel
 */
void report_kernel_statistics(
    std::ostream &os,
    const quantum_kernel &k,
    const quantum_platform &platform,
    const utils::Str &comment_prefix
);

/**
 * reports only the totals of the statistics of the circuits of the given kernels
 */
void report_totals_statistics(
    std::ostream &os,
    const utils::Vec<quantum_kernel> &kernels,
    const quantum_platform &platform,
    const utils::Str &comment_prefix
);

/**
 * reports the statistics of the circuits of the given kernels individually and in total
 * by appending them to the report file of the given program and place from where the report is done;
 * this report file is first created/truncated
 *
 * report_statistics is used in the cases where there is no pass specific data to report
 * otherwise, the sequence of calls in here has to be copied and supplemented by some report_string calls
 */
void report_statistics(
    const quantum_program *programp,
    const quantum_platform &platform,
    const utils::Str &in_or_out,
    const utils::Str &pass_name,
    const utils::Str &comment_prefix,
    const utils::Str &additionalStatistics = ""
);

/**
 * initialization of program.unique_name that is used by file name generation for reporting and printing
 * it is the program's name with a suffix appended that represents the number of the run of the program
 *
 * objective of this all is that of a later run of the same program, the output files don't overwrite the earlier ones
 *
 * do this only if unique_output option is set; if not, just use the program's name and let files overwrite
 * when set, maintain a seed with the run number; the first run is version 1; the first run uses the program's name
 * the second and later use the version (2 or larger) as suffix to the program's name
 */
void report_init(
    quantum_program *programp,
    const quantum_platform &platform
);

} // namespace ql
