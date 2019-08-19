/**
 * @file   quantumsim_eqasm_compiler.h
 * @date   03/2018
 * @author Imran Ashraf
 * @brief  quantumsim compiler implementation
 */

#ifndef QL_QUANTUMSIM_EQASM_COMPILER_H
#define QL_QUANTUMSIM_EQASM_COMPILER_H

#include <platform.h>
#include <kernel.h>
#include <gate.h>
#include <circuit.h>
#include <ir.h>
#include <scheduler.h>
#include <eqasm_compiler.h>
#include <mapper.h>
#include <clifford.h>

namespace ql
{
namespace arch
{

class quantumsim_eqasm_compiler : public eqasm_compiler
{
public:
    size_t num_qubits;
    size_t ns_per_cycle;

private:

    void clifford_optimize(std::string prog_name, std::vector<quantum_kernel>& kernels, const ql::quantum_platform& platform, std::string opt)
    {
        if (ql::options::get(opt) == "no")
        {
            DOUT("Clifford optimization on program " << prog_name << " at " << opt << " not DONE");
            return;
        }

        ql::report::report_statistics(prog_name, kernels, platform, "in", opt);
        ql::report::report_qasm(prog_name, kernels, platform, "in", opt);

        Clifford cliff;
        for(auto &kernel : kernels)
        {
            cliff.Optimize(kernel, opt);
        }

        ql::report::report_statistics(prog_name, kernels, platform, "out", opt);
        ql::report::report_qasm(prog_name, kernels, platform, "out", opt);
    }

    void map(std::string& prog_name, std::vector<quantum_kernel>& kernels, const ql::quantum_platform& platform)
    {
        auto mapopt = ql::options::get("mapper");
        if (mapopt == "no" )
        {
            IOUT("Not mapping kernels");
            return;;
        }

        ql::report::report_statistics(prog_name, kernels, platform, "in", "mapper");
        ql::report::report_qasm(prog_name, kernels, platform, "in", "mapper");

        Mapper mapper;  // virgin mapper creation; for role of Init functions, see comment at top of mapper.h
        mapper.Init(platform); // platform specifies number of real qubits, i.e. locations for virtual qubits

        std::ofstream   ofs;
        ofs = ql::report::report_open(prog_name, "out", "mapper");
        for(auto &kernel : kernels)
        {
            IOUT("Mapping kernel: " << kernel.name);
            mapper.Map(kernel);
                            // kernel.qubit_count is number of virtual qubits, i.e. highest indexed qubit minus 1
                            // kernel.qubit_count is updated by Map to real highest index used minus -1
            kernel.bundles = mapper.Bundler(kernel);

            ql::report::report_kernel_statistics(ofs, kernel, platform);
        }
        ql::report::report_totals_statistics(ofs, kernels, platform);
        ql::report::report_close(ofs);

        ql::report::report_bundles(prog_name, kernels, platform, "out", "mapper");
    }

    ql::ir::bundles_t quantumsim_schedule_rc(ql::circuit & ckt, 
        const ql::quantum_platform & platform, size_t nqubits, size_t ncreg = 0)
    {
        IOUT("Resource constraint scheduling for quantumsim ...");
    
        scheduling_direction_t  direction;
        std::string schedopt = ql::options::get("scheduler");
        if ("ASAP" == schedopt)
        {
            direction = forward_scheduling;
        }
        else if ("ALAP" == schedopt)
        {
            direction = backward_scheduling;
        }
        else
        {
            EOUT("Unknown scheduler");
            throw ql::exception("Unknown scheduler!", false);
    
        }
        resource_manager_t rm(platform, direction);
    
        Scheduler sched;
        sched.init(ckt, platform, nqubits, ncreg);
        ql::ir::bundles_t bundles;
        std::string dot;
        if ("ASAP" == schedopt)
        {
            bundles = sched.schedule_asap(rm, platform, dot);
        }
        else if ("ALAP" == schedopt)
        {
            bundles = sched.schedule_alap(rm, platform, dot);
        }
        else
        {
            EOUT("Unknown scheduler");
            throw ql::exception("Unknown scheduler!", false);
    
        }
    
        IOUT("Resource constraint scheduling for quantumsim [Done].");
        return bundles;
    }

    void schedule(std::string& prog_name, std::vector<quantum_kernel>& kernels, const ql::quantum_platform& platform)
    {
        for(auto &kernel : kernels)
        {
            IOUT("Scheduling kernel: " << kernel.name);
            if (! kernel.c.empty())
            {
                auto num_creg = 0;  // quantumsim
                kernel.bundles = quantumsim_schedule_rc(kernel.c, platform, num_qubits, num_creg);
            }
        }

        ql::report::report_bundles(prog_name, kernels, platform, "out", "rcscheduler");
    }

public:
    /*
     * program-level compilation of qasm to cc_light_eqasm
     */
    void compile(std::string prog_name, ql::circuit& ckt, ql::quantum_platform& platform)
    {
        FATAL("quantumsim_eqasm_comiler::compile interface with circuit not supported");
    }

    /*
     * compile qasm to quantumsim
     */
    // program level compilation
    void compile(std::string prog_name, std::vector<quantum_kernel>& kernels, const ql::quantum_platform& platform)
    {
        IOUT("Compiling " << kernels.size() << " kernels to generate quantumsim eQASM ... ");

        std::string params[] = { "qubit_number", "cycle_time" };
        size_t p = 0;
        try
        {
            num_qubits      = platform.hardware_settings[params[p++]];
            ns_per_cycle    = platform.hardware_settings[params[p++]];
        }
        catch (json::exception &e)
        {
            throw ql::exception("[x] error : ql::quantumsim::compile() : error while reading hardware settings : parameter '"+params[p-1]+"'\n    "+ std::string(e.what()),false);
        }

        write_quantumsim_program(prog_name, num_qubits, kernels, platform, "");

        clifford_optimize(prog_name, kernels, platform, "clifford_premapper");
        map(prog_name, kernels, platform);

        clifford_optimize(prog_name, kernels, platform, "clifford_prescheduler");
        schedule(prog_name, kernels, platform);

        // write scheduled bundles for quantumsim
        write_quantumsim_program(prog_name, num_qubits, kernels, platform, "mapped");

        DOUT("Compiling CCLight eQASM [Done]");
    }

private:
    // write scheduled bundles for quantumsim
    void write_quantumsim_program( std::string prog_name, size_t num_qubits,
        std::vector<quantum_kernel>& kernels, const ql::quantum_platform & platform, std::string suffix)
    {
        IOUT("Writing scheduled Quantumsim program");
        ofstream fout;
        string qfname( ql::options::get("output_dir") + "/" + prog_name + "_quantumsim_" + suffix + ".py");
        IOUT("Writing scheduled Quantumsim program to " << qfname);
        fout.open( qfname, ios::binary);
        if ( fout.fail() )
        {
            EOUT("opening file " << qfname << std::endl
                     << "Make sure the output directory ("<< ql::options::get("output_dir") << ") exists");
            return;
        }

        fout << "# Quantumsim program generated OpenQL\n"
             << "# Please modify at your will to obtain extra information from Quantumsim\n\n";

        fout << "import numpy as np\n"
             << "from quantumsim.circuit import Circuit\n"
             << "from quantumsim.circuit import uniform_noisy_sampler\n"
			 << "from quantumsim.circuit import ButterflyGate\n"
             << endl;

        fout << "from quantumsim.circuit import IdlingGate as i\n"                         
             << "from quantumsim.circuit import RotateY as ry\n"                            
             << "from quantumsim.circuit import RotateX as rx\n"                            
             << "from quantumsim.circuit import RotateZ as rz\n"                            
             << "from quantumsim.circuit import Hadamard as h\n"                            
             << "from quantumsim.circuit import NoisyCPhase as cz\n"                             
             << "from quantumsim.circuit import CNOT as cnot\n"                             
             << "from quantumsim.circuit import Swap as swap\n"                             
             << "from quantumsim.circuit import CPhaseRotation as cr\n"                     
             << "from quantumsim.circuit import ConditionalGate as ConditionalGate\n"       
             << "from quantumsim.circuit import RotateEuler as RotateEuler\n"               
             << "from quantumsim.circuit import ResetGate as ResetGate\n"                   
             << "from quantumsim.circuit import Measurement as measure\n"                   
             << "import quantumsim.sparsedm as sparsedm\n"                                  
             << "\n"                                                                        
             << "# print('GPU is used:', sparsedm.using_gpu)\n"                             
             << "\n"                                                                        
             << "\n"                                                                        
             << "def t(q, time):\n"                                                         
             << "    return RotateEuler(q, time=time, theta=0, phi=np.pi/4, lamda=0)\n"     
             << "\n"                                                                        
             << "def tdag(q, time):\n"                                                      
             << "    return RotateEuler(q, time=time, theta=0, phi=-np.pi/4, lamda=0)\n"    
             << "\n"                                                                        
             << "def measure_z(q, time, sampler):\n"                                        
             << "    return measure(q, time, sampler)\n"                                    
             << "\n"                                                                        
             << "def z(q, time):\n"                                                         
             << "    return rz(q, time, angle=np.pi)\n"                                     
             << "\n"                                                                        
             << "def x(q, time, dephasing_axis, dephasing_angle):\n"                                                         
             << "    return rx(q, time, angle=np.pi, dephasing_axis=dephasing_axis, dephasing_angle=dephasing_angle)\n"                                     
             << "\n"                                                                        
             << "def y(q, time, dephasing_axis, dephasing_angle):\n"                                                         
             << "    return ry(q, time, angle=np.pi, dephasing_axis=dephasing_axis, dephasing_angle=dephasing_angle)\n"                                     
             << "\n"                                                                        
             << "def x90(q, time, dephasing_axis, dephasing_angle):\n"                                                      
             << "    return rx(q, time, angle=np.pi/2, dephasing_axis=dephasing_axis, dephasing_angle=dephasing_angle)\n"                                   
             << "\n"                                                                        
             << "def y90(q, time, dephasing_axis, dephasing_angle):\n"                                                      
             << "    return ry(q, time, angle=np.pi/2, dephasing_axis=dephasing_axis, dephasing_angle=dephasing_angle)\n"                                   
             << "\n"                                                                        
             << "def xm90(q, time, dephasing_axis, dephasing_angle):\n"                                                      
             << "    return rx(q, time, angle=-np.pi/2, dephasing_axis=dephasing_axis, dephasing_angle=dephasing_angle)\n"                                  
             << "\n"                                                                        
             << "def ym90(q, time, dephasing_axis, dephasing_angle):\n"                                                      
             << "    return ry(q, time, angle=-np.pi/2, dephasing_axis=dephasing_axis, dephasing_angle=dephasing_angle)\n"                                  
             << "\n"                                                                        
             << "def x45(q, time):\n"                                                      
             << "    return rx(q, time, angle=np.pi/4)\n"                                   
             << "\n"                                                                        
             << "def xm45(q, time):\n"                                                      
             << "    return rx(q, time, angle=-np.pi/4)\n"                                  
             << "\n"  
             //<< "def cz(q, time, dephase_var):\n"                                                         
             //<< "    return cphase(q, time, dephase_var=dephase_var)\n"                                     
             << "\n"                                                                       
             << "def prepz(q, time):\n"                                                    
             << "    return ResetGate(q, time, state=0)\n\n"                                
             << endl;

        fout << "\n# create a circuit\n";
        fout << "def circuit_generated(t1=np.inf, t2=np.inf, dephasing_axis=None, dephasing_angle=None, dephase_var=0, readout_error=0.0) :\n";
        fout << "    c = Circuit(title=\"" << prog_name << "\")\n";

        DOUT("Adding qubits to Quantumsim program");
        fout << "\n    # add qubits\n";
        json config;
        try
        {
            config = load_json(platform.configuration_file_name);
        }
        catch (json::exception e)
        {
            throw ql::exception("[x] error : ql::quantumsim_compiler::load() :  failed to load the hardware config file : malformed json file ! : \n    "+
                                std::string(e.what()),false);
        }

        // load qubit attributes
        json qubit_attributes = config["qubit_attributes"];
        if (qubit_attributes.is_null())
        {
            EOUT("qubit_attributes is not specified in the hardware config file !");
            throw ql::exception("[x] error: quantumsim_compiler: qubit_attributes is not specified in the hardware config file !",false);
        }
        json relaxation_times = qubit_attributes["relaxation_times"];
        if (relaxation_times.is_null())
        {
            EOUT("relaxation_times is not specified in the hardware config file !");
            throw ql::exception("[x] error: quantumsim_compiler: relaxation_times is not specified in the hardware config file !",false);
        }
        size_t count =  platform.hardware_settings["qubit_number"];

        // want to ignore unused qubits below
        MapperAssert (kernels.size() <= 1);
        std::vector<size_t> check_usecount;
        check_usecount.resize(count, 0);

        for (auto & gp: kernels.front().c)
        {
            switch(gp->type())
            {
            case __classical_gate__:
            case __wait_gate__:
                break;
            default:    // quantum gate
                for (auto v: gp->operands)
                {
                    check_usecount[v]++;
                }
                break;
            }
        }

        for (json::iterator it = relaxation_times.begin(); it != relaxation_times.end(); ++it)
        {
            size_t q = stoi(it.key());
            if (q >= count)
            {
                EOUT("qubit_attribute.relaxation_time.qubit number is not in qubits available in the platform");
                throw ql::exception("[x] error: qubit_attribute.relaxation_time.qubit number is not in qubits available in the platform",false);
            }
            if (check_usecount[q] == 0)
            {
                DOUT("... qubit " << q << " is not used; skipping it");
                continue;
            }
            auto & rt = it.value();
            if (rt.size() < 2)
            {
                EOUT("each qubit must have at least two relaxation times");
                throw ql::exception("[x] error: each qubit must have at least two relaxation times",false);
            }
            // fout << "    c.add_qubit(\"q" << q <<"\", " << rt[0] << ", " << rt[1] << ")\n" ;
            fout << "    c.add_qubit(\"q" << q << "\", t1=t1, t2=t2)\n" ;
        }

        DOUT("Adding Gates to Quantumsim program");
        {
            // global writes
            std::stringstream ssbundles;
            ssbundles << "\n    sampler = uniform_noisy_sampler(readout_error=readout_error, seed=42)\n";
            ssbundles << "\n    # add gates\n";
            fout << ssbundles.str();
        }
        for(auto &kernel : kernels)
        {
            DOUT("... adding gates, a new kernel");
            if (kernel.bundles.empty())
            {
                IOUT("No bundles for adding gates");
            }
            else
            {
                for ( ql::ir::bundle_t & abundle : kernel.bundles)
                {
                    DOUT("... adding gates, a new bundle");
                    auto bcycle = abundle.start_cycle;
        
                    std::stringstream ssbundles;
                    for( auto secIt = abundle.parallel_sections.begin(); secIt != abundle.parallel_sections.end(); ++secIt )
                    {
                        DOUT("... adding gates, a new section in a bundle");
                        for(auto insIt = secIt->begin(); insIt != secIt->end(); ++insIt )
                        {
                            auto & iname = (*insIt)->name;
                            auto & operands = (*insIt)->operands;
                            auto duration = (*insIt)->duration;     // duration in nano-seconds
                            // size_t operation_duration = std::ceil( static_cast<float>(duration) / ns_per_cycle);
                            if( iname == "measure")
                            {
                                DOUT("... adding gates, a measure");
                                auto op = operands.back();
								ssbundles << "    c.add_qubit(\"m" << op << "\")\n";
                                ssbundles << "    c.add_gate("
                                          << "ButterflyGate("
                                          << "\"q" << op <<"\", "
                                          << "time=" << ((bcycle-1)*ns_per_cycle) << ", "
                                          << "p_exc=0,"
                                          << "p_dec= 0.005)"
                                          << ")\n" ;
								ssbundles << "    c.add_measurement("
									<< "\"q" << op << "\", "
									<< "time=" << ((bcycle - 1)*ns_per_cycle) + (duration/4) << ", "
									<< "output_bit=\"m" << op << "\", "
									<< "sampler=sampler"
									<< ")\n";
								ssbundles << "    c.add_gate("
									<< "ButterflyGate("
									<< "\"q" << op << "\", "
									<< "time=" << ((bcycle - 1)*ns_per_cycle) + duration/2 << ", "
									<< "p_exc=0,"
									<< "p_dec= 0.015)"
									<< ")\n";

                            }
                            else if( iname == "y90" or iname == "ym90" or iname == "y" or iname == "x" or 
								iname == "x90" or iname == "xm90")
                            {
                                DOUT("... adding gates, another gate");
                                ssbundles <<  "    c.add_gate("<< iname << "(" ;
                                size_t noperands = operands.size();
                                if( noperands > 0 )
                                {
                                    for(auto opit = operands.begin(); opit != operands.end()-1; opit++ )
                                        ssbundles << "\"q" << *opit <<"\", ";
                                    ssbundles << "\"q" << operands.back()<<"\"";
                                }
                                ssbundles << ", time=" << ((bcycle - 1)*ns_per_cycle) + (duration/2) << ", dephasing_axis=dephasing_axis, dephasing_angle=dephasing_angle))" << endl;
                            }
                            else if( iname == "cz")
                            {
                               DOUT("... adding gates, another gate");
                                ssbundles <<  "    c.add_gate("<< iname << "(" ;
                                size_t noperands = operands.size();
                                if( noperands > 0 )
                                {
                                    for(auto opit = operands.begin(); opit != operands.end()-1; opit++ )
                                        ssbundles << "\"q" << *opit <<"\", ";
                                    ssbundles << "\"q" << operands.back()<<"\"";
                                }
                                ssbundles << ", time=" << ((bcycle - 1)*ns_per_cycle) + (duration/2) << ", dephase_var=dephase_var))" << endl;
                            }
                            else
                            {
                                DOUT("... adding gates, another gate");
                                ssbundles <<  "    c.add_gate("<< iname << "(" ;
                                size_t noperands = operands.size();
                                if( noperands > 0 )
                                {
                                    for(auto opit = operands.begin(); opit != operands.end()-1; opit++ )
                                        ssbundles << "\"q" << *opit <<"\", ";
                                    ssbundles << "\"q" << operands.back()<<"\"";
                                }
                                ssbundles << ", time=" << ((bcycle - 1)*ns_per_cycle) + (duration/2) << "))" << endl;
                            }
                        }
                    }
                    fout << ssbundles.str();
                }
				fout << "    return c";
				fout << "    \n\n";
            }
        }
        fout << "    return c";

        fout.close();
        IOUT("Writing scheduled Quantumsim program [Done]");
    }
};

} // arch
} // ql

#endif // QL_QUANTUMSIM_EQASM_COMPILER_H

