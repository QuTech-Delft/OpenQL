/**
 * @file   quantumsim_eqasm_compiler.h
 * @date   03/2018
 * @author Imran Ashraf
 * @brief  quantumsim compiler implementation
 */

#ifndef QL_QUANTUMSIM_EQASM_COMPILER_H
#define QL_QUANTUMSIM_EQASM_COMPILER_H

#include <ql/platform.h>
#include <ql/ir.h>
#include <ql/circuit.h>
#include <ql/scheduler.h>
#include <ql/eqasm_compiler.h>

namespace ql
{
namespace arch
{

class quantumsim_eqasm_compiler : public eqasm_compiler
{
public:
    size_t num_qubits;
    size_t ns_per_cycle;

public:
    /*
     * compile qasm to quantumsim
     */
    void compile(std::string prog_name, ql::circuit& c, ql::quantum_platform& platform)
    {
        IOUT("Compiling qasm code ...");
        if (c.empty())
        {
            EOUT("empty circuit, eqasm compilation aborted !");
            return;
        }
        IOUT("Loading circuit (" <<  c.size() << " gates)...");

        std::string params[] = { "qubit_number", "cycle_time" };
        size_t p = 0;
        try
        {
            num_qubits      = platform.hardware_settings[params[p++]];
            ns_per_cycle    = platform.hardware_settings[params[p++]];
        }
        catch (json::exception &e)
        {
            throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while reading hardware settings : parameter '"+params[p-1]+"'\n\t"+ std::string(e.what()),false);
        }

        // schedule
        ql::ir::bundles_t bundles = quantumsim_schedule(prog_name, num_qubits, c, platform);

        // write scheduled bundles for quantumsim
        write_quantumsim_program(prog_name, num_qubits, bundles, platform);
    }

private:
    ql::ir::bundles_t quantumsim_schedule(  std::string prog_name, size_t nqubits,
            ql::circuit & ckt, ql::quantum_platform & platform)
    {
        IOUT("Scheduling Quantumsim instructions ...");
        Scheduler sched;
        sched.Init(ckt, platform, nqubits, 0); //no creg in quantumsim, so creg_count = 0
        std::string dot;
        ql::ir::bundles_t bundles = sched.schedule_asap(dot);

        IOUT("Scheduling Quantumsim instructions [Done].");
        return bundles;
    }

    void write_quantumsim_program( std::string prog_name, size_t num_qubits,
        ql::ir::bundles_t & bundles, ql::quantum_platform & platform)
    {
        IOUT("Writing scheduled Quantumsim program");
        ofstream fout;
        string qfname( ql::options::get("output_dir") + "/" + prog_name + "_quantumsim.py");
        IOUT("Writing scheduled Quantumsim program to " << qfname);
        fout.open( qfname, ios::binary);
        if ( fout.fail() )
        {
            EOUT("opening file " << qfname << std::endl
                     << "Make sure the output directory ("<< ql::options::get("output_dir") << ") exists");
            return;
        }

        fout << "# Quantumsim program generated OpenQL\n"
             << "# Please modify at your wil to obtain extra information from Quantumsim\n\n"
             << "import numpy as np\n"
             << "from quantumsim.circuit import Circuit\n"
             << "from quantumsim.circuit import uniform_noisy_sampler\n"
             << endl;

        fout << "\n# create a circuit\n";
        fout << "c = Circuit(title=\"" << prog_name << "\")\n\n";

        DOUT("Adding qubits to Quantumsim program");
        fout << "\n# add qubits\n";
        for (json::iterator it = platform.resources.begin(); it != platform.resources.end(); ++it)
        {
            std::string n = it.key();
            if( n == "qubits")
            {
                size_t count =  platform.resources["qubits"]["count"];
                if(count > num_qubits)
                {
                    EOUT("qubit count is more than the qubits available in the platform");
                    throw ql::exception("[x] error : qubit count is more than the qubits available in the platform",false);
                }
                // TODO simmilarly, check also if the qubits used in program are less than or equal to count

                auto & T1s = platform.resources["qubits"]["T1"];
                auto & T2s = platform.resources["qubits"]["T2"];

                for( size_t q=0; q<num_qubits; q++ ) // TODO should be for qubits used in program
                {
                    fout << "c.add_qubit(\"q" << q <<"\", " << T1s[q] << ", " << T2s[q] << ")\n" ;
                }
            }
        }

        DOUT("Adding Gates to Quantumsim program");
        fout << "\n# add gates\n";
        for ( ql::ir::bundle_t & abundle : bundles)
        {
            auto bcycle = abundle.start_cycle;

            std::stringstream ssbundles;
            for( auto secIt = abundle.parallel_sections.begin(); secIt != abundle.parallel_sections.end(); ++secIt )
            {
                for(auto insIt = secIt->begin(); insIt != secIt->end(); ++insIt )
                {
                    auto & iname = (*insIt)->name;
                    auto & operands = (*insIt)->operands;
                    if( iname == "measure")
                    {
                        auto op = operands.back();
                        ssbundles << "\nsampler = uniform_noisy_sampler(readout_error=0.03, seed=42)\n";
                        ssbundles << "c.add_qubit(\"m" << op <<"\")\n";
                        ssbundles << "c.add_measurement("
                                  << "\"q" << op <<"\", "
                                  << "time=" << bcycle << ", "
                                  << "output_bit=\"m" << op <<"\", "
                                  << "sampler=sampler"
                                  << ")\n" ;
                    }
                    else
                    {
                        ssbundles <<  "c.add_"<< iname << "(" ;
                        size_t noperands = operands.size();
                        if( noperands > 0 )
                        {
                            for(auto opit = operands.begin(); opit != operands.end()-1; opit++ )
                                ssbundles << "\"q" << *opit <<"\", ";
                            ssbundles << "\"q" << operands.back()<<"\"";
                        }
                        ssbundles << ", time=" << bcycle << ")" << endl;
                    }
                }
            }
            fout << ssbundles.str();
        }

        fout.close();
        IOUT("Writing scheduled Quantumsim program [Done]");
    }
};

} // arch
} // ql

#endif // QL_QUANTUMSIM_EQASM_COMPILER_H

