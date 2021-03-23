/** \file
 * Utilities for writing report files.
 * 
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

#include "report.h"

#include "utils/num.h"
#include "utils/str.h"
#include "options.h"
#include "ir.h"

namespace ql {

using namespace utils;

/*
 * support functions for reporting statistics
 */
static UInt get_classical_operations_count(
    const circuit &c,
    const quantum_platform &platform
) {
    UInt classical_operations = 0;
    // DOUT("... reporting get_classical_operations_count");
    for (auto &gp : c) {
        switch (gp->type()) {
            case __classical_gate__:
                classical_operations++;
                break;
            case __wait_gate__:
                break;
            default:    // quantum gate
                break;
        }
    }
    // DOUT("... reporting get_classical_operations_count [done]");
    return classical_operations;
}

static UInt get_non_single_qubit_quantum_gates_count(
    const circuit &c,
    const quantum_platform &platform
) {
    UInt quantum_gates = 0;
    // DOUT("... reporting get_non_single_qubit_quantum_gates_count");
    for (auto &gp : c) {
        switch (gp->type()) {
            case __classical_gate__:
                break;
            case __wait_gate__:
                break;
            default:    // quantum gate
                if (gp->operands.size() > 1) {
                    quantum_gates++;
                }
                break;
        }
    }
    // DOUT("... reporting get_non_single_qubit_quantum_gates_count [done]");
    return quantum_gates;
}

static void get_qubit_usecount(
    const circuit &c,
    const quantum_platform &platform,
    Vec<UInt> &usecount
) {
    // DOUT("... reporting get_qubit_usecount");
    for (auto &gp : c) {
        switch (gp->type()) {
            case __classical_gate__:
            case __wait_gate__:
                break;
            default:    // quantum gate
                for (auto v : gp->operands) {
                    usecount[v]++;
                }
                break;
        }
    }
    // DOUT("... reporting get_qubit_usecount [done]");
}

static void get_qubit_usedcyclecount(
    const circuit &c,
    const quantum_platform &platform,
    Vec<UInt> &usedcyclecount
) {
    UInt  cycle_time = platform.cycle_time;

    // DOUT("... reporting get_qubit_usedcyclecount");
    for (auto &gp : c) {
        switch (gp->type()) {
            case __classical_gate__:
                break;
            case __wait_gate__:
                break;
            default:    // quantum gate
                for (auto v : gp->operands) {
                    usedcyclecount[v] += (gp->duration+cycle_time-1)/cycle_time;
                }
                break;
        }
    }
    // DOUT("... reporting get_qubit_usedcyclecount [done]");
}

static UInt get_quantum_gates_count(
    const circuit &c,
    const quantum_platform &platform
) {
    UInt quantum_gates = 0;
    // DOUT("... reporting get_quantum_gates_count");
    for (auto &gp : c) {
        switch (gp->type()) {
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

static UInt get_circuit_latency(
    const circuit &c,
    const quantum_platform &platform
) {
    UInt cycle_time = platform.cycle_time;
    UInt circuit_latency_result;
    // DOUT("... reporting get_circuit_latency");
    if (c.size() < 1) {
        circuit_latency_result = 0;
        // DOUT("In get_circuit_latency() result is 0 because circuit is empty");
    } else if (c.back()->cycle == MAX_CYCLE) {
        circuit_latency_result = 0;
        // DOUT("In get_circuit_latency() result is 0 because c.back()->cycle == MAX_CYCLE");
    } else {
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
static void report_write_qasm(
    const Str &fname,
    const quantum_program *programp,
    const quantum_platform &platform
) {
    // DOUT("... reporting report_write_qasm");
    StrStrm out_qasm;
    out_qasm << "version 1.0\n";
    out_qasm << "# this file has been automatically generated by the OpenQL compiler please do not modify it manually.\n";
    out_qasm << "qubits " << programp->qubit_count << "\n";

    Bool do_bundles = true;
    for (auto &kernel : programp->kernels) {
        do_bundles &= kernel.cycles_valid;
    }
    // DOUT("... reporting do_bundles=" << do_bundles);

    for (auto &kernel : programp->kernels) {
        if (do_bundles) {
            out_qasm << kernel.get_prologue();
            ir::bundles_t bundles = ir::bundler(kernel.c, platform.cycle_time);
            out_qasm << ir::qasm(bundles);
            out_qasm << kernel.get_epilogue();
        } else {
            out_qasm << kernel.qasm();
        }
    }
    OutFile(fname).write(out_qasm.str());
    // DOUT("... reporting report_write_qasm [done]");
}

Str toOperationString(Str op) {
    if (op == "add") {
        return "+";
    } else if (op == "sub") {
        return "-";
    } else if (op == "and") {
        return "&";
    } else if (op == "or") {
        return "|";
    } else if (op == "xor") {
        return "^";
    } else if (op == "eq") {
        return "==";
    } else if (op == "ne") {
        return "!=";
    } else if (op == "lt") {
        return "<";
    } else if (op == "gt") {
        return ">";
    } else if (op == "le") {
        return "<=";
    } else if (op == "ge") {
        return ">=";
    } else {
        QL_EOUT("Unknown binary operation '" << op);
        throw Exception("Unknown binary operation '" + op + "' !", false);
    }
}

static void report_write_c(
    const Str &fname,
    const quantum_program *programp,
    const quantum_platform &platform
) {
    QL_DOUT("... start writing c file");
    StrStrm out_c;
    out_c << "#pragma ckt 100001\n\
typedef struct {\n\
    char dummy; /* not accessed */\n\
} _qbit;        /* must never be used */\n\
typedef _qbit * qbit;\n\
#define ckt_q_qbit 100001\n\
\n\
#pragma map generate_hw\n\
void " << programp->name << "(){\n";

    out_c << "    qbit ";
    
    for (UInt i = 0; i < platform.get_qubit_number() - 1; i++) {
        out_c << "qc" << i << ",";
    }
    out_c << "qc" << platform.get_qubit_number()-1 << ";\n\n";

    if (programp->creg_count) {
        out_c << "    int ";
        for (UInt i = 0; i < programp->creg_count - 1; i++) {
            out_c << "rs" << i << ",";
        }
        out_c << "rs" << programp->creg_count-1 << ";\n\n";
    }
    
    for (auto kernel : programp->get_kernels()) {
         QL_DOUT("          Kernel name: " << kernel.get_name() << " with type = " << (int)kernel.type);
        
        switch (kernel.type) {
            case kernel_type_t::IF_START: 
                out_c << "    if(rs" << (kernel.br_condition->operands[0])->as_creg().id << " ";
                out_c << toOperationString(kernel.br_condition->operation_name);
                out_c << " rs" << (kernel.br_condition->operands[1])->as_creg().id << ") {\n";
                break;

            case kernel_type_t::FOR_START:
                out_c << "    for(int i=0; i < " << kernel.iterations << "; i++){\n";
                break;

            case kernel_type_t::DO_WHILE_START:
                out_c << "    do {\n";
                break;

            case kernel_type_t::ELSE_START:
                out_c << "    else {\n";
                break;

            case kernel_type_t::IF_END:
            case kernel_type_t::ELSE_END:
            case kernel_type_t::FOR_END:
                out_c << "    }\n";
                break;

            case kernel_type_t::DO_WHILE_END: 
                out_c << "    } while(rs" << (kernel.br_condition->operands[0])->as_creg().id << " ";
                out_c << toOperationString(kernel.br_condition->operation_name);
                out_c << " rs" << (kernel.br_condition->operands[1])->as_creg().id << ");\n";
                break;

            case kernel_type_t::STATIC: {        
                circuit circ = kernel.get_circuit();
                for (auto g : circ) {
                    //NOTE-rn: match gate name to an instruction in the config file, otherwise this will fail.
                    UInt p = g->name.find(' ');
                    Str gate_name = g->name.substr(0, p);

                    //NOTE-rn: measure gate returns type custom_gate so gate_type_t::__measure_gate__ cannot be used to check for measure gate
                    if (gate_name == "measure" && g->creg_operands.size()) {
                        out_c << "    rs" << g->creg_operands[0] << " = ";
                    }
                    if (gate_name == "ldi"  && g->creg_operands.size()) {
                        out_c << "    rs" << g->creg_operands[0] << " = ";
                        out_c << g->int_operand << ";\n";
                    } else {
                        out_c << "    " << gate_name << "(qc" << g->operands[0] << ");\n";
                    }
                } break;
            }

            default:
                break;
        }
    }

    out_c << "}\n";
    
    OutFile(fname).write(out_c.str());
    QL_DOUT("... writing c file [done]");
}

/*
 * composes the write file's name
 * that contains the program name and the extension
 */
static Str report_compose_write_name(
    const Str &unique_name,
    const Str &extension
) {
    StrStrm fname;
    fname << options::get("output_dir") << "/"
      << unique_name << extension;
    return fname.str();
}

/*
 * write the qasm
 * in a file with a name that contains the program unique name and the given extension
 */
static void write_qasm_extension(
    const quantum_program *programp,
    const quantum_platform &platform,
    const Str &extension
) {
    report_write_qasm(report_compose_write_name(programp->unique_name, extension), programp, platform);
}

static void write_c_extension(
    const quantum_program *programp,
    const quantum_platform &platform,
    const Str &extension
) {
    report_write_c(report_compose_write_name(programp->unique_name, extension), programp, platform);
}

/*
 * composes the report file's name
 * that contains the program name and the place from where the report is done
 */
static Str report_compose_report_name(
    const Str &unique_name,
    const Str &in_or_out,
    const Str &pass_name,
    const Str &extension
) {
    StrStrm fname;
    fname << options::get("output_dir") << "/"
      << unique_name << "_" << pass_name << "_" << in_or_out << "." << extension;
    return fname.str();
}

/**
 * Opens an appropriately-named report file for writing if write_report_files
 * if set to yes.
 */
ReportFile::ReportFile(
    const quantum_program *programp,
    const utils::Str &in_or_out,
    const utils::Str &pass_name
) {
    if (options::get("write_report_files") == "yes") {
        auto fname = report_compose_report_name(programp->unique_name, in_or_out, pass_name, "report");
        of.emplace(fname);
    }
}

/**
 * Writes a string to the report file.
 */
void ReportFile::write(const utils::Str &content) {
    if (of) {
        *of << content;
    }
}

/**
 * Writes kernel statistics for the given kernel to the report file.
 */
void ReportFile::write_kernel_statistics(
    const quantum_kernel &k,
    const quantum_platform &platform,
    const Str &comment_prefix
) {
    if (of) {
        report_kernel_statistics(of->unwrap(), k, platform, comment_prefix);
    }
}

/**
 * Writes combined statistics for the given vector of kernels to the report
 * file.
 */
void ReportFile::write_totals_statistics(
    const utils::Vec<quantum_kernel> &kernels,
    const quantum_platform &platform,
    const Str &comment_prefix
) {
    if (of) {
        report_totals_statistics(of->unwrap(), kernels, platform, comment_prefix);
    }
}

/**
 * Closes the report file.
 */
void ReportFile::close() {
    if (of) {
        of->close();
    }
}

/*
 * write the qasm
 * in a file with a name that contains the program unique name and an extension defined by the pass_name
 */
void write_qasm(
    const quantum_program *programp,
    const quantum_platform &platform,
    const Str &pass_name
) {
    Str extension;
    // next is ugly; must be done by built-in pass class option with different value for each concrete pass
    if (pass_name == "initialqasmwriter" || pass_name == "outputIR") extension = ".qasm";
    else if (pass_name == "scheduledqasmwriter" || pass_name == "outputIRscheduled") extension = "_scheduled.qasm";
    else if (pass_name == "lastqasmwriter") extension = "_last.qasm";
    else if (pass_name == "CPrinter") extension = ".c";
    else QL_FATAL("write_qasm: pass_name " << pass_name << " unknown; don't know which extension to generate");

    write_qasm_extension(programp, platform, extension);
}

void write_c(
    const quantum_program *programp,
    const quantum_platform &platform,
    const Str &pass_name
) {
    Str extension;
    // next is ugly; must be done by built-in pass class option with different value for each concrete pass
    if (pass_name == "CPrinter") {
        extension = ".c";
    } else {
        QL_FATAL("write_c: pass_name " << pass_name << " unknown; ");
    }

    write_c_extension(programp, platform, extension);
}

/*
 * reports the qasm
 * in a file with a name that contains the program name and the place from where the report is done
 */
void report_qasm(
    const quantum_program *programp,
    const quantum_platform &platform,
    const Str &in_or_out,
    const Str &pass_name
) {
    if (options::get("write_qasm_files") == "yes") {
        auto fname = report_compose_report_name(programp->unique_name, in_or_out, pass_name, "qasm");
        report_write_qasm(fname, programp, platform);
    }
}

/*
 * report given string which is assumed to be closed by an endl by the caller
 */
void report_string(
    std::ostream &os,
    const Str &s
) {
    // DOUT("... reporting string");
    if (options::get("write_report_files") != "yes") {
        // DOUT("... reporting string no report [done]");
        return;
    }

    os << s;
    // DOUT("... reporting string [done]");
}

/*
 * report statistics of the circuit of the given kernel
 */
void report_kernel_statistics(
    std::ostream &os,
    const quantum_kernel &k,
    const quantum_platform &platform,
    const Str &comment_prefix
) {
    if (options::get("write_report_files") != "yes") {
        return;
    }

    // DOUT("... reporting report_kernel_statistics");
    Vec<UInt> usecount;
    usecount.resize(platform.qubit_number, 0);
    get_qubit_usecount(k.c, platform, usecount);
    UInt qubits_used = 0; for (auto v: usecount) { if (v != 0) { qubits_used++; } }

    Vec<UInt> usedcyclecount;
    usedcyclecount.resize(platform.qubit_number, 0);
    get_qubit_usedcyclecount(k.c, platform, usedcyclecount);

    UInt  circuit_latency = get_circuit_latency(k.c, platform);
    os << comment_prefix << "kernel: " << k.name << "\n";
    os << comment_prefix << "----- circuit_latency: " << circuit_latency << "\n";
    os << comment_prefix << "----- quantum gates: " << get_quantum_gates_count(k.c, platform) << "\n";
    os << comment_prefix << "----- non single qubit gates: " << get_non_single_qubit_quantum_gates_count(k.c, platform) << "\n";
    os << comment_prefix << "----- classical operations: " << get_classical_operations_count(k.c, platform) << "\n";
    os << comment_prefix << "----- qubits used: " << qubits_used << "\n";
    os << comment_prefix << "----- qubit cycles use:" << usedcyclecount << "\n";
    // DOUT("... reporting report_kernel_statistics [done]");
}

/*
 * reports only the total statistics of the circuits of the given kernels
 */
void report_totals_statistics(
    std::ostream &os,
    const Vec<quantum_kernel> &kernels,
    const quantum_platform &platform,
    const Str &comment_prefix
) {
    if (options::get("write_report_files") != "yes") {
        return;
    }

    // DOUT("... reporting report_totals_statistics");
    // totals reporting, collect info from all kernels
    Vec<UInt> usecount;
    usecount.resize(platform.qubit_number, 0);
    UInt total_circuit_latency = 0;
    UInt total_classical_operations = 0;
    UInt total_quantum_gates = 0;
    UInt total_non_single_qubit_gates= 0;
    for (auto &k : kernels) {
        get_qubit_usecount(k.c, platform, usecount);

        total_circuit_latency += get_circuit_latency(k.c, platform);
        total_classical_operations += get_classical_operations_count(k.c, platform);
        total_quantum_gates += get_quantum_gates_count(k.c, platform);
        total_non_single_qubit_gates += get_non_single_qubit_quantum_gates_count(k.c, platform);
    }
    UInt qubits_used = 0; for (auto v: usecount) { if (v != 0) { qubits_used++; } }

    // report totals
    os << "\n";
    os << comment_prefix << "Total circuit_latency: " << total_circuit_latency << "\n";
    os << comment_prefix << "Total no. of quantum gates: " << total_quantum_gates << "\n";
    os << comment_prefix << "Total no. of non single qubit gates: " << total_non_single_qubit_gates << "\n";
    os << comment_prefix << "Total no. of classical operations: " << total_classical_operations << "\n";
    os << comment_prefix << "Qubits used: " << qubits_used << "\n";
    os << comment_prefix << "No. kernels: " << kernels.size() << "\n";
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
void report_statistics(
    const quantum_program *programp,
    const quantum_platform &platform,
    const Str &in_or_out,
    const Str &pass_name,
    const Str &comment_prefix,
    const Str &additionalStatistics
) {
    if (options::get("write_report_files") != "yes") {
        return;
    }

    // DOUT("... reporting report_statistics");
    auto rf = ReportFile(programp, in_or_out, pass_name);

    // per kernel reporting
    for (auto &k : programp->kernels) {
        rf.write_kernel_statistics(k, platform, comment_prefix);
    }

    // and total collecting and reporting
    rf.write_totals_statistics(programp->kernels, platform, comment_prefix);

    if (!additionalStatistics.empty()) {
        rf << " \n\n" << additionalStatistics;
    }

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
static UInt report_bump_unique_file_version(const quantum_program *programp) {
    Str version_file = QL_SS2S(options::get("output_dir") << "/" << programp->name << ".unique");

    // Retrieve old version number, if one exists.
    UInt vers = 0;
    if (is_file(version_file)) {
        InFile(version_file) >> vers;
    }

    // Increment to get new one.
    vers++;

    // Store version for a later run.
    OutFile(version_file) << vers;

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
void report_init(
    quantum_program *programp,
    const quantum_platform &platform
) {
    programp->unique_name = programp->name;
    if (options::get("unique_output") == "yes") {
        UInt vers = report_bump_unique_file_version(programp);
        if (vers > 1) {
            programp->unique_name = programp->name + to_string(vers);
            QL_DOUT("Unique program name after bump_unique_file_version: " << programp->unique_name << " based on version: " << vers);
        }
    }
}

} // namespace ql
