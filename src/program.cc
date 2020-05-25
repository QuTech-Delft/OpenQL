/**
 * @file   program.cc
 * @date   04/2017
 * @author Nader Khammassi
 *         Imran Ashraf
 * @brief  openql program
 */

#include "program.h"

#include <utils.h>
#include <options.h>
#include <interactionMatrix.h>
#include <scheduler.h>
#include <optimizer.h>
#include <decompose_toffoli.h>
#include <arch/cbox/cbox_eqasm_compiler.h>
#include <arch/cc_light/cc_light_eqasm_compiler.h>
#include <arch/cc/eqasm_backend_cc.h>

static unsigned long phi_node_count = 0;    // FIXME: number across quantum_program instances

namespace ql
{

    /**
     * @brief   Quantum program constructor
     * @param   n   Name of the program
     */
quantum_program::quantum_program(std::string n)
    : name(n)
{
    DOUT("Constructor for quantum_program:  " << n);
}
    
quantum_program::quantum_program(std::string n, quantum_platform platf, size_t nqubits, size_t ncregs)
        : name(n), platform(platf), qubit_count(nqubits), creg_count(ncregs)
{
    default_config = true;
    needs_backend_compiler = true;
    eqasm_compiler_name = platform.eqasm_compiler_name;
    backend_compiler    = NULL;
    if (eqasm_compiler_name =="")
    {
        FATAL("eqasm compiler name must be specified in the hardware configuration file !");
    }
    else if (eqasm_compiler_name == "none")
    {
        needs_backend_compiler = false;
    }
    else if (eqasm_compiler_name == "qx")
    {
        // at the moment no qx specific thing is done
        needs_backend_compiler = false;;
    }
    else if (eqasm_compiler_name == "qumis_compiler")
    {
        backend_compiler = new ql::arch::cbox_eqasm_compiler();
        assert(backend_compiler);
    }
    else if (eqasm_compiler_name == "cc_light_compiler" )
    {
        backend_compiler = new ql::arch::cc_light_eqasm_compiler();
        assert(backend_compiler);
    }
    else if (eqasm_compiler_name == "eqasm_backend_cc" )
    {
        backend_compiler = new ql::arch::eqasm_backend_cc();
        assert(backend_compiler);
    }
    else
    {
        FATAL("the '" << eqasm_compiler_name << "' eqasm compiler backend is not suported !");
    }

    if(qubit_count > platform.qubit_number)
    {
        FATAL("number of qubits requested in program '" + std::to_string(qubit_count) + "' is greater than the qubits available in platform '" + std::to_string(platform.qubit_number) + "'" );
    }

    // report/write_qasm initialization
    ql::report_init(this, platform);

}

void quantum_program::add(ql::quantum_kernel &k)
{
    // check sanity of supplied qubit/classical operands for each gate
    /** @todo-RN: move this check to a backend platform validation pass for the target platform 
    ql::circuit& kc = k.get_circuit();
    for( auto & g : kc )
    {
        auto & gate_operands = g->operands;
        auto & gname = g->name;
        auto gtype = g->type();
        for(auto & op : gate_operands)
        {
            if(
                ((gtype == __classical_gate__) && (op >= creg_count)) ||
                ((gtype != __classical_gate__) && (op >= qubit_count))
              )
            {
                 FATAL("Out of range operand(s) for operation: '" << gname <<
                        "' (op=" << op <<
                        ", qubit_count=" << qubit_count <<
                        ", creg_count=" << creg_count <<
                        ")");
            }
        }
    }*/

    for (auto kernel : kernels)
    {
        if(kernel.name == k.name)
        {
            FATAL("Cannot add kernel. Duplicate kernel name: " << k.name);
        }
    }
    // if sane, now add kernel to list of kernels
    kernels.push_back(k);
}

void quantum_program::add_program(ql::quantum_program p)
{
    for(auto & k : p.kernels)
    {
        add(k);
    }
}

void quantum_program::add_if(ql::quantum_kernel &k, ql::operation & cond)
{
    // phi node
    ql::quantum_kernel kphi1(k.name+"_if", platform, qubit_count, creg_count);
    kphi1.set_kernel_type(ql::kernel_type_t::IF_START);
    kphi1.set_condition(cond);
    kernels.push_back(kphi1);

    add(k);

    // phi node
    ql::quantum_kernel kphi2(k.name+"_if_end", platform, qubit_count, creg_count);
    kphi2.set_kernel_type(ql::kernel_type_t::IF_END);
    kphi2.set_condition(cond);
    kernels.push_back(kphi2);
}

void quantum_program::add_if(ql::quantum_program p, ql::operation & cond)
{
    // phi node
    ql::quantum_kernel kphi1(p.name+"_if", platform, qubit_count, creg_count);
    kphi1.set_kernel_type(ql::kernel_type_t::IF_START);
    kphi1.set_condition(cond);
    kernels.push_back(kphi1);

    add_program(p);

    // phi node
    ql::quantum_kernel kphi2(p.name+"_if_end", platform, qubit_count, creg_count);
    kphi2.set_kernel_type(ql::kernel_type_t::IF_END);
    kphi2.set_condition(cond);
    kernels.push_back(kphi2);
}

void quantum_program::add_if_else(ql::quantum_kernel &k_if, ql::quantum_kernel &k_else, ql::operation & cond)
{
    ql::quantum_kernel kphi1(k_if.name+"_if"+ std::to_string(phi_node_count), platform, qubit_count, creg_count);
    kphi1.set_kernel_type(ql::kernel_type_t::IF_START);
    kphi1.set_condition(cond);
    kernels.push_back(kphi1);

    add(k_if);

    // phi node
    ql::quantum_kernel kphi2(k_if.name+"_if"+ std::to_string(phi_node_count) +"_end", platform, qubit_count, creg_count);
    kphi2.set_kernel_type(ql::kernel_type_t::IF_END);
    kphi2.set_condition(cond);
    kernels.push_back(kphi2);


    // phi node
    ql::quantum_kernel kphi3(k_else.name+"_else" + std::to_string(phi_node_count), platform, qubit_count, creg_count);
    kphi3.set_kernel_type(ql::kernel_type_t::ELSE_START);
    kphi3.set_condition(cond);
    kernels.push_back(kphi3);

    add(k_else);

    // phi node
    ql::quantum_kernel kphi4(k_else.name+"_else" + std::to_string(phi_node_count)+"_end", platform, qubit_count, creg_count);
    kphi4.set_kernel_type(ql::kernel_type_t::ELSE_END);
    kphi4.set_condition(cond);
    kernels.push_back(kphi4);

    phi_node_count++;
}

void quantum_program::add_if_else(ql::quantum_program &p_if, ql::quantum_program &p_else, ql::operation & cond)
{
    ql::quantum_kernel kphi1(p_if.name+"_if"+ std::to_string(phi_node_count), platform, qubit_count, creg_count);
    kphi1.set_kernel_type(ql::kernel_type_t::IF_START);
    kphi1.set_condition(cond);
    kernels.push_back(kphi1);

    add_program(p_if);

    // phi node
    ql::quantum_kernel kphi2(p_if.name+"_if"+ std::to_string(phi_node_count) +"_end", platform, qubit_count, creg_count);
    kphi2.set_kernel_type(ql::kernel_type_t::IF_END);
    kphi2.set_condition(cond);
    kernels.push_back(kphi2);


    // phi node
    ql::quantum_kernel kphi3(p_else.name+"_else" + std::to_string(phi_node_count), platform, qubit_count, creg_count);
    kphi3.set_kernel_type(ql::kernel_type_t::ELSE_START);
    kphi3.set_condition(cond);
    kernels.push_back(kphi3);

    add_program(p_else);

    // phi node
    ql::quantum_kernel kphi4(p_else.name+"_else" + std::to_string(phi_node_count)+"_end", platform, qubit_count, creg_count);
    kphi4.set_kernel_type(ql::kernel_type_t::ELSE_END);
    kphi4.set_condition(cond);
    kernels.push_back(kphi4);

    phi_node_count++;
}

void quantum_program::add_do_while(ql::quantum_kernel &k, ql::operation & cond)
{
    // phi node
    ql::quantum_kernel kphi1(k.name+"_do_while"+ std::to_string(phi_node_count) +"_start", platform, qubit_count, creg_count);
    kphi1.set_kernel_type(ql::kernel_type_t::DO_WHILE_START);
    kphi1.set_condition(cond);
    kernels.push_back(kphi1);

    add(k);

    // phi node
    ql::quantum_kernel kphi2(k.name+"_do_while" + std::to_string(phi_node_count), platform, qubit_count, creg_count);
    kphi2.set_kernel_type(ql::kernel_type_t::DO_WHILE_END);
    kphi2.set_condition(cond);
    kernels.push_back(kphi2);
    phi_node_count++;
}

void quantum_program::add_do_while(ql::quantum_program p, ql::operation & cond)
{
    // phi node
    ql::quantum_kernel kphi1(p.name+"_do_while"+ std::to_string(phi_node_count) +"_start", platform, qubit_count, creg_count);
    kphi1.set_kernel_type(ql::kernel_type_t::DO_WHILE_START);
    kphi1.set_condition(cond);
    kernels.push_back(kphi1);

    add_program(p);

    // phi node
    ql::quantum_kernel kphi2(p.name+"_do_while" + std::to_string(phi_node_count), platform, qubit_count, creg_count);
    kphi2.set_kernel_type(ql::kernel_type_t::DO_WHILE_END);
    kphi2.set_condition(cond);
    kernels.push_back(kphi2);
    phi_node_count++;
}

void quantum_program::add_for(ql::quantum_kernel &k, size_t iterations)
{
    // phi node
    ql::quantum_kernel kphi1(k.name+"_for"+ std::to_string(phi_node_count) +"_start", platform, qubit_count, creg_count);
    kphi1.set_kernel_type(ql::kernel_type_t::FOR_START);
    kphi1.iterations = iterations;
    kernels.push_back(kphi1);

    k.iterations = iterations;
    add(k);

    // phi node
    ql::quantum_kernel kphi2(k.name+"_for" + std::to_string(phi_node_count) +"_end", platform, qubit_count, creg_count);
    kphi2.set_kernel_type(ql::kernel_type_t::FOR_END);
    kernels.push_back(kphi2);
    phi_node_count++;
}

void quantum_program::add_for(ql::quantum_program p, size_t iterations)
{
    bool nested_for = false;
    for(auto & k : p.kernels)
    {
        if(k.type == ql::kernel_type_t::FOR_START)
            nested_for = true;
    }
    if(nested_for)
    {
        EOUT("Nested for not yet implemented !");
        throw ql::exception("Error: Nested for not yet implemented !",false);
    }

    if(iterations>0) // as otherwise it will be optimized away
    {
        // phi node
        ql::quantum_kernel kphi1(p.name+"_for"+ std::to_string(phi_node_count) +"_start", platform, qubit_count, creg_count);
        kphi1.set_kernel_type(ql::kernel_type_t::FOR_START);
        kphi1.iterations = iterations;
        kernels.push_back(kphi1);

        // phi node
        ql::quantum_kernel kphi2(p.name, platform, qubit_count, creg_count);
        kphi2.set_kernel_type(ql::kernel_type_t::STATIC);
        kernels.push_back(kphi2);

        add_program(p);

        // phi node
        ql::quantum_kernel kphi3(p.name+"_for" + std::to_string(phi_node_count) +"_end", platform, qubit_count, creg_count);
        kphi3.set_kernel_type(ql::kernel_type_t::FOR_END);
        kernels.push_back(kphi3);
        phi_node_count++;
    }
}

void quantum_program::set_config_file(std::string file_name)
{
    config_file_name = file_name;
    default_config   = false;
}

void quantum_program::set_platform(quantum_platform & platform)
{
    this->platform = platform;
}

int quantum_program::compile()
{
    IOUT("compiling " << name << " ...");
    WOUT("compiling " << name << " ...");
    if (kernels.empty())
    {
        FATAL("compiling a program with no kernels !");
    }

    // from here on front-end passes

    // writer pass of the initial qasm file (program.qasm)
    ql::write_qasm(this, platform, "initialqasmwriter");

    // rotation_optimize pass
    rotation_optimize(this, platform, "rotation_optimize");

    // decompose_toffoli pass
    ql::decompose_toffoli(this, platform, "decompose_toffoli");

    // prescheduler pass
    ql::schedule(this, platform, "prescheduler");

    // writer pass of the scheduled qasm file (program_scheduled.qasm)
    ql::write_qasm(this, platform, "scheduledqasmwriter");


    // from here on backend passes

    DOUT("eqasm_compiler_name: " << eqasm_compiler_name);

    if (!needs_backend_compiler)
    {
        WOUT("The eqasm compiler attribute indicated that no backend passes are needed.");
        return 0;
    }

    if (backend_compiler == NULL)
    {
        EOUT("No known eqasm compiler has been specified in the configuration file.");
        return 0;
    }
    else
    {
        DOUT("About to call backend_compiler->compile for " << eqasm_compiler_name);
        backend_compiler->compile(this, platform);
        DOUT("Returned from call backend_compiler->compile for " << eqasm_compiler_name);
    }

    if (sweep_points.size())
    {
        std::stringstream ss_swpts;
        ss_swpts << "{ \"measurement_points\" : [";
        for (size_t i=0; i<sweep_points.size()-1; i++)
            ss_swpts << sweep_points[i] << ", ";
        ss_swpts << sweep_points[sweep_points.size()-1] << "] }";
        std::string config = ss_swpts.str();
        if (default_config)
        {
            std::stringstream ss_config;
            ss_config << ql::options::get("output_dir") << "/" << unique_name << "_config.json";
            std::string conf_file_name = ss_config.str();
            IOUT("writing sweep points to '" << conf_file_name << "'...");
            ql::utils::write_file(conf_file_name, config);
        }
        else
        {
            std::stringstream ss_config;
            ss_config << ql::options::get("output_dir") << "/" << config_file_name;
            std::string conf_file_name = ss_config.str();
            IOUT("writing sweep points to '" << conf_file_name << "'...");
            ql::utils::write_file(conf_file_name, config);
        }
    }
    else
    {
        IOUT("sweep points file not generated as sweep point array is empty !");
    }
    IOUT("compilation of program '" << name << "' done.");

    return 0;
}

void quantum_program::print_interaction_matrix()
{
    IOUT("printing interaction matrix...");

    for (auto k : kernels)
    {
        InteractionMatrix imat( k.get_circuit(), qubit_count);
        string mstr = imat.getString();
        std::cout << mstr << std::endl;
    }
}

void quantum_program::write_interaction_matrix()
{
    for (auto k : kernels)
    {
        InteractionMatrix imat( k.get_circuit(), qubit_count);
        string mstr = imat.getString();

        string fname = ql::options::get("output_dir") + "/" + k.get_name() + "InteractionMatrix.dat";
        IOUT("writing interaction matrix to '" << fname << "' ...");
        ql::utils::write_file(fname, mstr);
    }
}

void quantum_program::set_sweep_points(float * swpts, size_t size)
{
    sweep_points.clear();
    for (size_t i=0; i<size; ++i)
        sweep_points.push_back(swpts[i]);
}

} // ql
