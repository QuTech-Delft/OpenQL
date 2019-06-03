/**
 * @file   kernel.h
 * @date   04/2017
 * @author Nader Khammassi
 *         Imran Ashraf
 * @brief  openql kernel
 */

#ifndef QL_KERNEL_H
#define QL_KERNEL_H

#include <sstream>
#include <algorithm>
#include <iterator>

#include "ql/json.h"
#include "ql/utils.h"
#include "ql/options.h"
#include "ql/gate.h"
#include "ql/classical.h"
#include "ql/optimizer.h"
#include "ql/ir.h"

#define PI M_PI

#ifndef __disable_lemon__
#include "scheduler.h"
#endif // __disable_lemon__

namespace ql
{
enum class kernel_type_t
{
    STATIC,
    FOR_START, FOR_END,
    DO_WHILE_START, DO_WHILE_END,
    IF_START, IF_END,
    ELSE_START, ELSE_END
};

/**
 * quantum_kernel
 */
class quantum_kernel
{
public:

    quantum_kernel(std::string name) :
        name(name), iterations(1), type(kernel_type_t::STATIC) {}

    quantum_kernel(std::string name, ql::quantum_platform& platform,
                   size_t qcount, size_t ccount=0) :
        name(name), iterations(1), qubit_count(qcount),
        creg_count(ccount), type(kernel_type_t::STATIC)
    {
        gate_definition = platform.instruction_map;     // FIXME: confusing name change
        cycle_time = platform.cycle_time;
    }

    void set_static_loop_count(size_t it)
    {
        iterations = it;
    }

    void set_condition(operation & oper)
    {
        if( (oper.operands[0])->id >= creg_count || (oper.operands[1])->id >= creg_count)
        {
            EOUT("Out of range operand(s) for '" << oper.operation_name);
            throw ql::exception("Out of range operand(s) for '"+oper.operation_name+"' !",false);
        }

        if(oper.operation_type != ql::operation_type_t::RELATIONAL)
        {
            EOUT("Relational operator not used for conditional '" << oper.operation_name);
            throw ql::exception("Relational operator not used for conditional '"+oper.operation_name+"' !",false);
        }

        br_condition = oper;
    }

    void set_kernel_type(kernel_type_t typ)
    {
        type = typ;
    }

    /************************************************************************\
    | Gate shortcuts
    \************************************************************************/

    void identity(size_t qubit)
    {
        gate("identity", qubit );
    }

    void i(size_t qubit)
    {
        gate("identity", qubit );
    }

    void hadamard(size_t qubit)
    {
        gate("hadamard", qubit );
    }

    void h(size_t qubit)
    {
        hadamard(qubit);
    }

    void rx(size_t qubit, double angle)
    {
        std::string gname("rx");
        // to do : rotation decomposition
        c.push_back(new ql::rx(qubit,angle));
    }

    void ry(size_t qubit, double angle)
    {
        std::string gname("ry");
        // to do : rotation decomposition
        c.push_back(new ql::ry(qubit,angle));
    }

    void rz(size_t qubit, double angle)
    {
        std::string gname("rz");
        // to do : rotation decomposition
        c.push_back(new ql::rz(qubit,angle));
    }

    void s(size_t qubit)
    {
        gate("s", qubit );
    }

    void sdag(size_t qubit)
    {
        gate("sdag", qubit );
    }

    void t(size_t qubit)
    {
        gate("t", qubit );
    }

    void tdag(size_t qubit)
    {
        gate("tdag", qubit );
    }

    void x(size_t qubit)
    {
        gate("x", qubit );
    }

    void y(size_t qubit)
    {
        gate("y", qubit );
    }

    void z(size_t qubit)
    {
        gate("z", qubit );
    }

    void rx90(size_t qubit)
    {
        gate("rx90", qubit );
    }

    void mrx90(size_t qubit)
    {
        gate("mrx90", qubit );
    }

    void rx180(size_t qubit)
    {
        gate("rx180", qubit );
    }

    void ry90(size_t qubit)
    {
        gate("ry90", qubit );
    }

    void mry90(size_t qubit)
    {
        gate("mry90", qubit );
    }

    void ry180(size_t qubit)
    {
        gate("ry180", qubit );
    }

    void measure(size_t qubit)
    {
        gate("measure", qubit );
    }

    void prepz(size_t qubit)
    {
        gate("prepz", qubit );
    }

    void cnot(size_t qubit1, size_t qubit2)
    {
        gate("cnot", {qubit1, qubit2} );
    }

    void cz(size_t qubit1, size_t qubit2)
    {
        gate("cz", {qubit1, qubit2} );
    }

    void cphase(size_t qubit1, size_t qubit2)
    {
        gate("cphase", {qubit1, qubit2} );
    }

    void toffoli(size_t qubit1, size_t qubit2, size_t qubit3)
    {
        // TODO add custom gate check if needed
        c.push_back(new ql::toffoli(qubit1, qubit2, qubit3));
    }

    void swap(size_t qubit1, size_t qubit2)
    {
        gate("swap", {qubit1, qubit2} );
    }

    void wait(std::vector<size_t> qubits, size_t duration)
    {
        gate("wait", qubits, {}, duration );
    }

    void display()
    {
        c.push_back(new ql::display());
    }

    /**
     * add clifford
     */
    void clifford(int id, size_t qubit=0)
    {
        switch (id)
        {
        case 0 :
            break;              //  ['I']
        case 1 :
            ry90(qubit);
            rx90(qubit);
            break;              //  ['Y90', 'X90']
        case 2 :
            mrx90(qubit);
            mry90(qubit);
            break;              //  ['mX90', 'mY90']
        case 3 :
            rx180(qubit);
            break;              //  ['X180']
        case 4 :
            mry90(qubit);
            mrx90(qubit);
            break;              //  ['mY90', 'mX90']
        case 5 :
            rx90(qubit);
            mry90(qubit);
            break;              //  ['X90', 'mY90']
        case 6 :
            ry180(qubit);
            break;              //  ['Y180']
        case 7 :
            mry90(qubit);
            rx90(qubit);
            break;              //  ['mY90', 'X90']
        case 8 :
            rx90(qubit);
            ry90(qubit);
            break;              //  ['X90', 'Y90']
        case 9 :
            rx180(qubit);
            ry180(qubit);
            break;              //  ['X180', 'Y180']
        case 10:
            ry90(qubit);
            mrx90(qubit);
            break;              //  ['Y90', 'mX90']
        case 11:
            mrx90(qubit);
            ry90(qubit);
            break;              //  ['mX90', 'Y90']
        case 12:
            ry90(qubit);
            rx180(qubit);
            break;              //  ['Y90', 'X180']
        case 13:
            mrx90(qubit);
            break;              //  ['mX90']
        case 14:
            rx90(qubit);
            mry90(qubit);
            mrx90(qubit);
            break;              //  ['X90', 'mY90', 'mX90']
        case 15:
            mry90(qubit);
            break;              //  ['mY90']
        case 16:
            rx90(qubit);
            break;              //  ['X90']
        case 17:
            rx90(qubit);
            ry90(qubit);
            rx90(qubit);
            break;              //  ['X90', 'Y90', 'X90']
        case 18:
            mry90(qubit);
            rx180(qubit);
            break;              //  ['mY90', 'X180']
        case 19:
            rx90(qubit);
            ry180(qubit);
            break;              //  ['X90', 'Y180']
        case 20:
            rx90(qubit);
            mry90(qubit);
            rx90(qubit);
            break;              //  ['X90', 'mY90', 'X90']
        case 21:
            ry90(qubit);
            break;              //  ['Y90']
        case 22:
            mrx90(qubit);
            ry180(qubit);
            break;              //  ['mX90', 'Y180']
        case 23:
            rx90(qubit);
            ry90(qubit);
            mrx90(qubit);
            break;              //  ['X90', 'Y90', 'mX90']
        default:
            break;
        }
    }

    /************************************************************************\
    | Gate management
    \************************************************************************/

    bool add_default_gate_if_available(std::string gname, std::vector<size_t> qubits,
                                       std::vector<size_t> cregs = {}, size_t duration=0, double angle=0.0)
    {
        bool result=false;

        bool is_one_qubit_gate = (gname == "identity") || (gname == "i")
                                 || (gname == "hadamard") || (gname == "h")
                                 || (gname == "pauli_x") || (gname == "pauli_y") || (gname == "pauli_z")
                                 || (gname == "x") || (gname == "y") || (gname == "z")
                                 || (gname == "s") || (gname == "sdag")
                                 || (gname == "t") || (gname == "tdag")
                                 || (gname == "rx") || (gname == "ry") || (gname == "rz")
                                 || (gname == "rx90") || (gname == "mrx90") || (gname == "rx180")
                                 || (gname == "ry90") || (gname == "mry90") || (gname == "ry180")
                                 || (gname == "measure") || (gname == "prepz");

        bool is_two_qubit_gate = (gname == "cnot")
                                 || (gname == "cz") || (gname == "cphase")
                                 || (gname == "swap");

        bool is_multi_qubit_gate = (gname == "toffoli")
                                   || (gname == "wait") || (gname == "barrier");

        if(is_one_qubit_gate)
        {
            if( qubits.size() != 1 )
                return false;
        }
        else if(is_two_qubit_gate)
        {
            if( qubits.size() != 2 )
                return false;
            if( qubits[0] == qubits[1] )
                return false;
        }
        else if(is_multi_qubit_gate)
        {
            // by default wait will be applied to all qubits
        }
        else
        {
            return false;
        }

        if( gname == "identity" || gname == "i" )
        {
            c.push_back(new ql::identity(qubits[0]) );
            result = true;
        }
        else if( gname == "hadamard" || gname == "h" )
        {
            c.push_back(new ql::hadamard(qubits[0]) );
            result = true;
        }
        else if( gname == "pauli_x" || gname == "x" )
        {
            c.push_back(new ql::pauli_x(qubits[0]) );
            result = true;
        }
        else if( gname == "pauli_y" || gname == "y" )
        {
            c.push_back(new ql::pauli_y(qubits[0]) );
            result = true;
        }
        else if( gname == "pauli_z" || gname == "z" )
        {
            c.push_back(new ql::pauli_z(qubits[0]) );
            result = true;
        }
        else if( gname == "s" || gname == "phase" )
        {
            c.push_back(new ql::phase(qubits[0]) );
            result = true;
        }
        else if( gname == "sdag" || gname == "phasedag" )
        {
            c.push_back(new ql::phasedag(qubits[0]) );
            result = true;
        }
        else if( gname == "t" )
        {
            c.push_back(new ql::t(qubits[0]) );
            result = true;
        }
        else if( gname == "tdag" )
        {
            c.push_back(new ql::tdag(qubits[0]) );
            result = true;
        }
        else if( gname == "rx" )
        {
            c.push_back(new ql::rx(qubits[0], angle));
            result = true;
        }
        else if( gname == "ry" )
        {
            c.push_back(new ql::ry(qubits[0], angle));
            result = true;
        }
        else if( gname == "rz" )
        {
            c.push_back(new ql::rz(qubits[0], angle));
            result = true;
        }
        else if( gname == "rx90" )
        {
            c.push_back(new ql::rx90(qubits[0]) );
            result = true;
        }
        else if( gname == "mrx90" )
        {
            c.push_back(new ql::mrx90(qubits[0]) );
            result = true;
        }
        else if( gname == "rx180" )
        {
            c.push_back(new ql::rx180(qubits[0]) );
            result = true;
        }
        else if( gname == "ry90" )
        {
            c.push_back(new ql::ry90(qubits[0]) );
            result = true;
        }
        else if( gname == "mry90" )
        {
            c.push_back(new ql::mry90(qubits[0]) );
            result = true;
        }
        else if( gname == "ry180" )
        {
            c.push_back(new ql::ry180(qubits[0]) );
            result = true;
        }
        else if( gname == "measure" )
        {
            if(cregs.empty())
                c.push_back(new ql::measure(qubits[0]) );
            else
                c.push_back(new ql::measure(qubits[0], cregs[0]) );

            result = true;
        }
        else if( gname == "prepz" )
        {
            c.push_back(new ql::prepz(qubits[0]) );
            result = true;
        }
        else if( gname == "cnot" )
        {
            c.push_back(new ql::cnot(qubits[0], qubits[1]) );
            result = true;
        }
        else if( gname == "cz" || gname == "cphase" )
        {
            c.push_back(new ql::cphase(qubits[0], qubits[1]) );
            result = true;
        }
        else if( gname == "toffoli" )
            { c.push_back(new ql::toffoli(qubits[0], qubits[1], qubits[2]) ); result = true; }
        else if( gname == "swap" )       { c.push_back(new ql::swap(qubits[0], qubits[1]) ); result = true; }
        else if( gname == "barrier")
        {
            /*
            wait/barrier is applied on the qubits specified as arguments.
            if no qubits are specified, then wait/barrier is applied on all qubits
            */
            if(qubits.size() == 0) // i.e. no qubits specified
            {
                for(size_t q=0; q<qubit_count; q++)
                    qubits.push_back(q);
            }

            c.push_back(new ql::wait(qubits, 0, 0));
            result = true;
        }
        else if( gname == "wait")
        {
            /*
            wait/barrier is applied on the qubits specified as arguments.
            if no qubits are specified, then wait/barrier is applied on all qubits
            */
            if(qubits.size() == 0) // i.e. no qubits specified
            {
                for(size_t q=0; q<qubit_count; q++)
                    qubits.push_back(q);
            }

            size_t duration_in_cycles = std::ceil(static_cast<float>(duration)/cycle_time);
            c.push_back(new ql::wait(qubits, duration, duration_in_cycles));
            result = true;
        }
        else
            result = false;

        return result;
    }

    bool add_custom_gate_if_available(std::string & gname, std::vector<size_t> qubits,
                                      std::vector<size_t> cregs = {}, size_t duration=0, double angle=0.0)
    {
        bool added = false;

        // first check if a specialized custom gate is available
        std::string instr = gname + " ";
        if(qubits.size() > 0)
        {
            for (size_t i=0; i<(qubits.size()-1); ++i)
                instr += "q" + std::to_string(qubits[i]) + ",";
            if(qubits.size() >= 1) // to make if work with gates without operands
                instr += "q" + std::to_string(qubits[qubits.size()-1]);
        }

        std::map<std::string,custom_gate*>::iterator it = gate_definition.find(instr);
        if (it != gate_definition.end())
        {
            custom_gate* g = new custom_gate(*(it->second));
            for(auto & qubit : qubits)
                g->operands.push_back(qubit);
            for(auto & cop : cregs)
                g->creg_operands.push_back(cop);
            if(duration>0) g->duration = duration;
            g->angle = angle;
            added = true;
            c.push_back(g);
        }
        else
        {
            // otherwise, check if there is a parameterized custom gate (i.e. not specialized for arguments)
            std::map<std::string,custom_gate*>::iterator it = gate_definition.find(gname);
            if (it != gate_definition.end())
            {
                custom_gate* g = new custom_gate(*(it->second));
                for(auto & qubit : qubits)
                    g->operands.push_back(qubit);
                for(auto & cop : cregs)
                    g->creg_operands.push_back(cop);
                if(duration>0) g->duration = duration;
                g->angle = angle;
                added = true;
                c.push_back(g);
            }
        }

        if(added)
        {
            DOUT("custom gate added for " << gname);
        }
        else
        {
            DOUT("custom gate not added for " << gname);
        }

        return added;
    }

    void get_decomposed_ins( ql::composite_gate * gptr, std::vector<std::string> & sub_instructons )
    {
        auto & sub_gates = gptr->gs;
        DOUT("composite ins: " << gptr->name);
        for(auto & agate : sub_gates)
        {
            std::string & sub_ins = agate->name;
            DOUT("  sub ins: " << sub_ins);
            auto it = gate_definition.find(sub_ins);
            if( it != gate_definition.end() )
            {
                sub_instructons.push_back(sub_ins);
            }
            else
            {
                throw ql::exception("[x] error : ql::kernel::gate() : gate decomposition not available for '"+sub_ins+"'' in the target platform !",false);
            }
        }
    }

    bool add_spec_decomposed_gate_if_available(std::string gate_name,
            std::vector<size_t> all_qubits, std::vector<size_t> cregs = {})
    {
        bool added = false;
        DOUT("Checking if specialized decomposition is available for " << gate_name);
        std::string instr_parameterized = gate_name + " ";
        size_t i;
        if(all_qubits.size() > 0)
        {
            for(i=0; i<all_qubits.size()-1; i++)
            {
                instr_parameterized += "q" + std::to_string(all_qubits[i]) + " ";
            }
            if(all_qubits.size() >= 1)
            {
                instr_parameterized += "q" + std::to_string(all_qubits[i]);
            }
        }
        DOUT("decomposed specialized instruction name: " << instr_parameterized);

        auto it = gate_definition.find(instr_parameterized);
        if( it != gate_definition.end() )
        {
            DOUT("specialized composite gate found for " << instr_parameterized);
            composite_gate * gptr = (composite_gate *)(it->second);
            if( __composite_gate__ == gptr->type() )
            {
                DOUT("composite gate type");
            }
            else
            {
                DOUT("Not a composite gate type");
                return false;
            }


            std::vector<std::string> sub_instructons;
            get_decomposed_ins( gptr, sub_instructons );
            for(auto & sub_ins : sub_instructons)
            {
                DOUT("Adding sub ins: " << sub_ins);
                std::replace( sub_ins.begin(), sub_ins.end(), ',', ' ');
                DOUT(" after comma removal, sub ins: " << sub_ins);
                std::istringstream iss(sub_ins);

                std::vector<std::string> tokens{ std::istream_iterator<std::string>{iss},
                                                 std::istream_iterator<std::string>{} };

                std::vector<size_t> this_gate_qubits;
                std::string & sub_ins_name = tokens[0];

                for(size_t i=1; i<tokens.size(); i++)
                {
                    DOUT("tokens[i] : " << tokens[i]);
                    auto sub_str_token = tokens[i].substr(1);
                    DOUT("sub_str_token[i] : " << sub_str_token);
                    this_gate_qubits.push_back( stoi( tokens[i].substr(1) ) );
                }

                DOUT( ql::utils::to_string<size_t>(this_gate_qubits, "actual qubits of this gate:") );

                // custom gate check
                bool custom_added = add_custom_gate_if_available(sub_ins_name, this_gate_qubits, cregs);
                if(!custom_added)
                {
                    if(ql::options::get("use_default_gates") == "yes")
                    {
                        // default gate check
                        DOUT("adding default gate for " << sub_ins_name);
                        bool default_available = add_default_gate_if_available(sub_ins_name, this_gate_qubits, cregs);
                        if( default_available )
                        {
                            WOUT("added default gate '" << sub_ins_name << "' with " << ql::utils::to_string(this_gate_qubits,"qubits") );
                        }
                        else
                        {
                            EOUT("unknown gate '" << sub_ins_name << "' with " << ql::utils::to_string(this_gate_qubits,"qubits") );
                            throw ql::exception("[x] error : ql::kernel::gate() : the gate '"+sub_ins_name+"' with " +ql::utils::to_string(this_gate_qubits,"qubits")+" is not supported by the target platform !",false);
                        }
                    }
                    else
                    {
                        EOUT("unknown gate '" << sub_ins_name << "' with " << ql::utils::to_string(this_gate_qubits,"qubits") );
                        throw ql::exception("[x] error : ql::kernel::gate() : the gate '"+sub_ins_name+"' with " +ql::utils::to_string(this_gate_qubits,"qubits")+" is not supported by the target platform !",false);
                    }
                }
            }
            added = true;
        }
        else
        {
            DOUT("composite gate not found for " << instr_parameterized);
        }

        return added;
    }


    bool add_param_decomposed_gate_if_available(std::string gate_name,
            std::vector<size_t> all_qubits, std::vector<size_t> cregs = {})
    {
        bool added = false;
        DOUT("Checking if parameterized decomposition is available for " << gate_name);
        std::string instr_parameterized = gate_name + " ";
        size_t i;
        if(all_qubits.size() > 0)
        {
            for(i=0; i<all_qubits.size()-1; i++)
            {
                instr_parameterized += "%" + std::to_string(i) + " ";
            }
            if(all_qubits.size() >= 1)
            {
                instr_parameterized += "%" + std::to_string(i);
            }
        }
        DOUT("decomposed parameterized instruction name: " << instr_parameterized);

        // check for composite ins
        auto it = gate_definition.find(instr_parameterized);
        if( it != gate_definition.end() )
        {
            DOUT("parameterized composite gate found for " << instr_parameterized);
            composite_gate * gptr = (composite_gate *)(it->second);
            if( __composite_gate__ == gptr->type() )
            {
                DOUT("composite gate type");
            }
            else
            {
                DOUT("Not a composite gate type");
                return false;
            }

            std::vector<std::string> sub_instructons;
            get_decomposed_ins( gptr, sub_instructons );
            for(auto & sub_ins : sub_instructons)
            {
                DOUT("Adding sub ins: " << sub_ins);
                std::replace( sub_ins.begin(), sub_ins.end(), ',', ' ');
                DOUT(" after comma removal, sub ins: " << sub_ins);
                std::istringstream iss(sub_ins);

                std::vector<std::string> tokens{ std::istream_iterator<std::string>{iss},
                                                 std::istream_iterator<std::string>{} };

                std::vector<size_t> this_gate_qubits;
                std::string & sub_ins_name = tokens[0];

                for(size_t i=1; i<tokens.size(); i++)
                {
                    this_gate_qubits.push_back( all_qubits[ stoi( tokens[i].substr(1) ) ] );
                }

                DOUT( ql::utils::to_string<size_t>(this_gate_qubits, "actual qubits of this gate:") );

                // custom gate check
                bool custom_added = add_custom_gate_if_available(sub_ins_name, this_gate_qubits, cregs);
                if(!custom_added)
                {
                    if(ql::options::get("use_default_gates") == "yes")
                    {
                        // default gate check
                        DOUT("adding default gate for " << sub_ins_name);
                        bool default_available = add_default_gate_if_available(sub_ins_name, this_gate_qubits, cregs);
                        if( default_available )
                        {
                            WOUT("added default gate '" << sub_ins_name << "' with " << ql::utils::to_string(this_gate_qubits,"qubits") );
                        }
                        else
                        {
                            EOUT("unknown gate '" << sub_ins_name << "' with " << ql::utils::to_string(this_gate_qubits,"qubits") );
                            throw ql::exception("[x] error : ql::kernel::gate() : the gate '"+sub_ins_name+"' with " +ql::utils::to_string(this_gate_qubits,"qubits")+" is not supported by the target platform !",false);
                        }
                    }
                    else
                    {
                        EOUT("unknown gate '" << sub_ins_name << "' with " << ql::utils::to_string(this_gate_qubits,"qubits") );
                        throw ql::exception("[x] error : ql::kernel::gate() : the gate '"+sub_ins_name+"' with " +ql::utils::to_string(this_gate_qubits,"qubits")+" is not supported by the target platform !",false);
                    }
                }
            }
            added = true;
        }
        else
        {
            DOUT("composite gate not found for " << instr_parameterized);
        }
        return added;
    }


    /**
     * custom 1 qubit gate.
     */
    void gate(std::string gname, size_t q0)
    {
        gate(gname, std::vector<size_t> {q0});
    }

    /**
     * custom 2 qubits gate
     */
    void gate(std::string gname, size_t q0, size_t q1)
    {
        gate(gname, std::vector<size_t> {q0, q1});
    }


    /**
     * custom gate with arbitrary number of operands
     */
    void gate(std::string gname, std::vector<size_t> qubits = {},
              std::vector<size_t> cregs = {}, size_t duration=0, double angle = 0.0)
    {
        for(auto & qno : qubits)
        {
            if( qno >= qubit_count )
            {
                EOUT("Number of qubits in platform: " << std::to_string(qubit_count) << ", specified qubit numbers out of range for gate: '" << gname << "' with " << ql::utils::to_string(qubits,"qubits") );
                throw ql::exception("[x] error : ql::kernel::gate() : Number of qubits in platform: "+std::to_string(qubit_count)+", specified qubit numbers out of range for gate '"+gname+"' with " +ql::utils::to_string(qubits,"qubits")+" !",false);
            }
        }

        for(auto & cno : cregs)
        {
            if( cno >= creg_count )
            {
                EOUT("Out of range operand(s) for '" << gname << "' with " << ql::utils::to_string(cregs,"cregs") );
                throw ql::exception("Out of range operand(s) for '"+gname+"' with " +ql::utils::to_string(cregs,"cregs")+" !",false);
            }
        }

        // check if specialized composite gate is available
        // if not, check if parameterized composite gate is available
        // if not, check if a specialized custom gate is available
        // if not, check if a parameterized custom gate is available
        // if not, check if a default gate is available
        // if not, then error

        str::lower_case(gname);
        DOUT("Adding gate : " << gname << " with " << ql::utils::to_string(qubits,"qubits"));

        // specialized composite gate check
        DOUT("trying to add specialized decomposed gate for: " << gname);
        bool spec_decom_added = add_spec_decomposed_gate_if_available(gname, qubits);
        if(spec_decom_added)
        {
            DOUT("specialized decomposed gates added for " << gname);
        }
        else
        {
            // parameterized composite gate check
            DOUT("trying to add parameterized decomposed gate for: " << gname);
            bool param_decom_added = add_param_decomposed_gate_if_available(gname, qubits);
            if(param_decom_added)
            {
                DOUT("decomposed gates added for " << gname);
            }
            else
            {
                // specialized/parameterized custom gate check
                DOUT("adding custom gate for " << gname);
                bool custom_added = add_custom_gate_if_available(gname, qubits, cregs, duration, angle);
                if(!custom_added)
                {
                    if(ql::options::get("use_default_gates") == "yes")
                    {
                        // default gate check (which is always parameterized)
                        DOUT("adding default gate for " << gname);

                        bool default_available = add_default_gate_if_available(gname, qubits, cregs, duration);
                        if( default_available )
                        {
                            WOUT("default gate added for " << gname);
                        }
                        else
                        {
                            EOUT("unknown gate '" << gname << "' with " << ql::utils::to_string(qubits,"qubits") );
                            throw ql::exception("[x] error : ql::kernel::gate() : the gate '"+gname+"' with " +ql::utils::to_string(qubits,"qubits")+" is not supported by the target platform !",false);
                        }
                    }
                    else
                    {
                        EOUT("unknown gate '" << gname << "' with " << ql::utils::to_string(qubits,"qubits") );
                        throw ql::exception("[x] error : ql::kernel::gate() : the gate '"+gname+"' with " +ql::utils::to_string(qubits,"qubits")+" is not supported by the target platform !",false);
                    }
                }
                else
                {
                    DOUT("custom gate added for " << gname);
                }
            }
        }
        DOUT("");
    }

    // FIXME: is this really QASM, or CC-light eQASM?
    // FIXME: create a separate QASM backend?
    std::string get_prologue()
    {
        std::stringstream ss;
        ss << "." << name << "\n";
        // ss << name << ":\n";

        if(type == kernel_type_t::IF_START)
        {
            ss << "    b" << br_condition.inv_operation_name <<" r" << (br_condition.operands[0])->id
               <<", r" << (br_condition.operands[1])->id << ", " << name << "_end\n";
        }

        if(type == kernel_type_t::ELSE_START)
        {
            ss << "    b" << br_condition.operation_name <<" r" << (br_condition.operands[0])->id
               <<", r" << (br_condition.operands[1])->id << ", " << name << "_end\n";
        }

        if(type == kernel_type_t::FOR_START)
        {
            // TODO for now r29, r30, r31 are used, fix it
            ss << "    ldi r29" <<", " << iterations << "\n";
            ss << "    ldi r30" <<", " << 1 << "\n";
            ss << "    ldi r31" <<", " << 0 << "\n";
        }

        return ss.str();
    }

    std::string get_epilogue()
    {
        std::stringstream ss;

        if(type == kernel_type_t::DO_WHILE_END)
        {
            ss << "    b" << br_condition.operation_name <<" r" << (br_condition.operands[0])->id
               <<", r" << (br_condition.operands[1])->id << ", " << name << "_start\n";
        }

        if(type == kernel_type_t::FOR_END)
        {
            std::string kname(name);
            std::replace( kname.begin(), kname.end(), '_', ' ');
            std::istringstream iss(kname);
            std::vector<std::string> tokens{ std::istream_iterator<std::string>{iss},
                                             std::istream_iterator<std::string>{} };

            // TODO for now r29, r30, r31 are used, fix it
            ss << "    add r31, r31, r30\n";
            ss << "    blt r31, r29, " << tokens[0] << "\n";
        }

        return ss.str();
    }

    /**
     * qasm
     */
    std::string qasm()
    {
        std::stringstream ss;

        ss << get_prologue();

        for(size_t i=0; i<c.size(); ++i)
        {
            ss << "    " << c[i]->qasm() << "\n";
        }

        ss << get_epilogue();

        return  ss.str();
    }

    void classical(creg& destination, operation & oper)
    {
        // check sanity of destination
        if(destination.id >= creg_count)
        {
            EOUT("Out of range operand(s) for '" << oper.operation_name);
            throw ql::exception("Out of range operand(s) for '"+oper.operation_name+"' !",false);
        }

        // check sanity of other operands
        for(auto &op : oper.operands)
        {
            if(op->type() == operand_type_t::CREG)
            {
                if(op->id >= creg_count)
                {
                    EOUT("Out of range operand(s) for '" << oper.operation_name);
                    throw ql::exception("Out of range operand(s) for '"+oper.operation_name+"' !",false);
                }
            }
        }

        c.push_back(new ql::classical(destination, oper));
    }

    void classical(std::string operation)
    {
        c.push_back(new ql::classical(operation));
    }

#if OPT_MICRO_CODE
    /**
     * micro code
     */
    std::string micro_code()
    {
        std::stringstream ss;
        // ss << "." << name;
        // if (iterations > 1)
        // ss << "(" << iterations << ")\n";
        // else
        // ss << "\n";
        for (size_t i=0; i<c.size(); ++i)
        {
            ss << c[i]->micro_code() << "\n";
            // std::cout << c[i]->qasm() << std::endl;
        }
        return ss.str();
    }
#endif

    void optimize()
    {
        ql::rotations_merging rm;
        if (contains_measurements(c))
        {
            // decompose the circuit
            std::vector<circuit*> cs = split_circuit(c);
            std::vector<circuit > cs_opt;
            for (size_t i=0; i<cs.size(); ++i)
            {
                if (!contains_measurements(*cs[i]))
                {
                    circuit opt = rm.optimize(*cs[i]);
                    cs_opt.push_back(opt);
                }
                else
                    cs_opt.push_back(*cs[i]);
            }
            // for (int i=0; i<cs_opt.size(); ++i)
            // print(cs_opt[i]);
            c.clear( );
            for (size_t i=0; i<cs_opt.size(); ++i)
                for (size_t j=0; j<cs_opt[i].size(); j++)
                    c.push_back(cs_opt[i][j]);
        }
        else
        {
            c = rm.optimize(c);
        }

    }

    void decompose_toffoli()
    {
        DOUT("decompose_toffoli()");
        for( auto cit = c.begin(); cit != c.end(); ++cit )
        {
            auto g = *cit;
            ql::gate_type_t gtype = g->type();
            std::vector<size_t> goperands = g->operands;

            ql::quantum_kernel toff_kernel("toff_kernel");
            toff_kernel.gate_definition = gate_definition;
            toff_kernel.qubit_count = qubit_count;
            toff_kernel.cycle_time = cycle_time;

            if( __toffoli_gate__ == gtype )
            {
                size_t cq1 = goperands[0];
                size_t cq2 = goperands[1];
                size_t tq = goperands[2];
                auto opt = ql::options::get("decompose_toffoli");
                if ( opt == "AM" )
                {
                    toff_kernel.controlled_cnot_AM(tq, cq1, cq2);
                }
                else
                {
                    toff_kernel.controlled_cnot_NC(tq, cq1, cq2);
                }
                ql::circuit& toff_ckt = toff_kernel.get_circuit();
                cit = c.erase(cit);
                cit = c.insert(cit, toff_ckt.begin(), toff_ckt.end());
            }
        }
        DOUT("decompose_toffoli() [Done] ");
    }

    void schedule(quantum_platform platform, std::string& sched_qasm,
        std::string & dot, std::string& sched_dot)
    {
        std::string scheduler = ql::options::get("scheduler");
        std::string scheduler_uniform = ql::options::get("scheduler_uniform");
        std::string kqasm("");

#ifndef __disable_lemon__
        IOUT( scheduler << " scheduling the quantum kernel '" << name << "'...");

        Scheduler sched;
        sched.init(c, platform, qubit_count, creg_count);

        if(ql::options::get("print_dot_graphs") == "yes")
        {
            sched.get_dot(dot);
        }

        
        if("ASAP" == scheduler)
        {
            if ("yes" == scheduler_uniform)
            {
                EOUT("Uniform scheduling not supported with ASAP; please turn on ALAP to perform uniform scheduling");     // FIXME: FATAL?
            }
            else if ("no" == scheduler_uniform)
            {
                ql::ir::bundles_t bundles = sched.schedule_asap(sched_dot);
                kqasm = ql::ir::qasm(bundles);
            }
            else
            {
                EOUT("Unknown scheduler_uniform option value");
            }
        }
        else if("ALAP" == scheduler)
        {
            if ("yes" == scheduler_uniform)
            {
                ql::ir::bundles_t bundles = sched.schedule_alap_uniform();
                kqasm = ql::ir::qasm(bundles);
            }
            else if ("no" == scheduler_uniform)
            {
                ql::ir::bundles_t bundles = sched.schedule_alap(sched_dot);
                kqasm = ql::ir::qasm(bundles);
            }
            else
            {
                EOUT("Unknown scheduler_uniform option value");
            }
        }
        else
        {
            EOUT("Unknown scheduler");
            throw ql::exception("Unknown scheduler!", false);
        }

        sched_qasm = get_prologue() + kqasm + get_epilogue();

#endif // __disable_lemon__
    }

    std::vector<circuit*> split_circuit(circuit x)
    {
        IOUT("circuit decomposition in basic blocks ... ");
        std::vector<circuit*> cs;
        cs.push_back(new circuit());
        for (size_t i=0; i<x.size(); i++)
        {
            if ((x[i]->type() == __prepz_gate__) || (x[i]->type() == __measure_gate__))
            {
                cs.push_back(new circuit());
                cs.back()->push_back(x[i]);
                cs.push_back(new circuit());
            }
            else
            {
                cs.back()->push_back(x[i]);
            }
        }
        IOUT("circuit decomposion done (" << cs.size() << ").");
        /*
           for (int i=0; i<cs.size(); ++i)
           {
           println(" |-- circuit " << i);
           print(*(cs[i]));
           }
         */
        return cs;
    }

    /**
     * detect measurements and qubit preparations
     */
    bool contains_measurements(circuit x)
    {
        for (size_t i=0; i<x.size(); i++)
        {
            if (x[i]->type() == __measure_gate__)
                return true;
            if (x[i]->type() == __prepz_gate__)
                return true;
        }
        return false;
    }

    /**
     * detect unoptimizable gates
     */
    bool contains_unoptimizable_gates(circuit x)
    {
        for (size_t i=0; i<x.size(); i++)
        {
            if (x[i]->type() == __measure_gate__)
                return true;
            if (x[i]->type() == __prepz_gate__)
                return true;
            if (!(x[i]->optimization_enabled))
                return true;
        }
        return false;
    }

    /**
     * load custom instructions from a json file
     */
    int load_custom_instructions(std::string file_name="instructions.json")
    {
        load_instructions(gate_definition,file_name);
        return 0;
    }

    /**
     * debug
     */
    void print_gates_definition()
    {
        for (std::map<std::string,custom_gate*>::iterator i=gate_definition.begin(); i!=gate_definition.end(); i++)
        {
            COUT("[-] gate '" << i->first << "'");
#if OPT_MICRO_CODE
            COUT(" |- qumis : \n" << i->second->micro_code());
#endif
        }
    }

    std::string get_gates_definition()
    {
        std::stringstream ss;

        for (std::map<std::string,custom_gate*>::iterator i=gate_definition.begin(); i!=gate_definition.end(); i++)
        {
            ss << i->first << '\n';
        }
        return ss.str();
    }

    /**
     * name getter
     */
    std::string get_name()
    {
        return name;
    }

    /**
     * circuit getter
     */
    circuit& get_circuit()
    {
        return c;
    }

    /************************************************************************\
    | Controlled gates
    \************************************************************************/

    void controlled_x(size_t tq, size_t cq)
    {
        // from: https://arxiv.org/pdf/1206.0758v3.pdf
        // A meet-in-the-middle algorithm for fast synthesis
        // of depth-optimal quantum circuits
        cnot(cq, tq);
    }
    void controlled_y(size_t tq, size_t cq)
    {
        // from: https://arxiv.org/pdf/1206.0758v3.pdf
        // A meet-in-the-middle algorithm for fast synthesis
        // of depth-optimal quantum circuits
        sdag(tq);
        cnot(cq, tq);
        s(tq);
    }
    void controlled_z(size_t tq, size_t cq)
    {
        // from: https://arxiv.org/pdf/1206.0758v3.pdf
        // A meet-in-the-middle algorithm for fast synthesis
        // of depth-optimal quantum circuits
        hadamard(tq);
        cnot(cq, tq);
        hadamard(tq);
    }
    void controlled_h(size_t tq, size_t cq)
    {
        // from: https://arxiv.org/pdf/1206.0758v3.pdf
        // A meet-in-the-middle algorithm for fast synthesis
        // of depth-optimal quantum circuits
        s(tq);
        hadamard(tq);
        t(tq);
        cnot(cq, tq);
        tdag(tq);
        hadamard(tq);
        sdag(tq);
    }
    void controlled_i(size_t tq, size_t cq)
    {
        // well, basically you dont need to do anything for it :‑)
    }

    void controlled_s(size_t tq, size_t cq)
    {
        // cphase(cq, tq);

        // from: https://arxiv.org/pdf/1206.0758v3.pdf
        // A meet-in-the-middle algorithm for fast synthesis
        // of depth-optimal quantum circuits

        cnot(tq, cq);
        tdag(cq);
        cnot(tq, cq);
        t(cq);
        t(tq);
    }

    void controlled_sdag(size_t tq, size_t cq)
    {
        // based on: https://arxiv.org/pdf/1206.0758v3.pdf
        // A meet-in-the-middle algorithm for fast synthesis
        // of depth-optimal quantum circuits

        tdag(cq);
        tdag(tq);
        cnot(tq, cq);
        t(cq);
        cnot(tq, cq);
    }

    void controlled_t(size_t tq, size_t cq, size_t aq)
    {
        WOUT("Controlled-T implementation requires an ancilla");
        WOUT("At the moment, Qubit 0 is used as ancilla");
        WOUT("This will change when Qubit allocater is implemented");
        // from: https://arxiv.org/pdf/1206.0758v3.pdf
        // A meet-in-the-middle algorithm for fast synthesis
        // of depth-optimal quantum circuits
        cnot(cq, tq);
        hadamard(aq);
        sdag(cq);
        cnot(tq, aq);
        cnot(aq, cq);
        t(cq);
        tdag(aq);
        cnot(tq, cq);
        cnot(tq, aq);
        t(cq);
        tdag(aq);
        cnot(aq, cq);
        h(cq);
        t(cq);
        h(cq);
        cnot(aq, cq);
        tdag(cq);
        t(aq);
        cnot(tq, aq);
        cnot(tq, cq);
        t(aq);
        tdag(cq);
        cnot(aq, cq);
        s(cq);
        cnot(tq, aq);
        cnot(cq, tq);
        h(aq);
    }

    void controlled_tdag(size_t tq, size_t cq, size_t aq)
    {
        WOUT("Controlled-Tdag implementation requires an ancilla");
        WOUT("At the moment, Qubit 0 is used as ancilla");
        WOUT("This will change when Qubit allocater is implemented");
        // from: https://arxiv.org/pdf/1206.0758v3.pdf
        // A meet-in-the-middle algorithm for fast synthesis
        // of depth-optimal quantum circuits
        h(aq);
        cnot(cq, tq);
        sdag(cq);
        cnot(tq, aq);
        cnot(aq, cq);
        t(cq);
        cnot(tq, cq);
        tdag(aq);
        cnot(tq, aq);
        t(cq);
        tdag(aq);
        cnot(aq, cq);
        h(cq);
        tdag(cq);
        h(cq);
        cnot(aq, cq);
        tdag(cq);
        t(aq);
        cnot(tq, aq);
        cnot(tq, cq);
        tdag(cq);
        t(aq);
        cnot(aq, cq);
        s(cq);
        cnot(tq, aq);
        cnot(cq, tq);
        hadamard(aq);
    }

    void controlled_ix(size_t tq, size_t cq)
    {
        // from: https://arxiv.org/pdf/1210.0974.pdf
        // Quantum circuits of T-depth one
        cnot(cq, tq);
        s(cq);
    }

    // toffoli decomposition
    // from: https://arxiv.org/pdf/1210.0974.pdf
    // Quantum circuits of T-depth one
    void controlled_cnot_AM(size_t tq, size_t cq1, size_t cq2)
    {
        h(tq);
        t(cq1);
        t(cq2);
        t(tq);
        cnot(cq2, cq1);
        cnot(tq, cq2);
        cnot(cq1, tq);
        tdag(cq2);
        cnot(cq1, cq2);
        tdag(cq1);
        tdag(cq2);
        tdag(tq);
        cnot(tq, cq2);
        cnot(cq1, tq);
        cnot(cq2, cq1);
        h(tq);
    }

    // toffoli decomposition
    // Neilsen and Chuang
    void controlled_cnot_NC(size_t tq, size_t cq1, size_t cq2)
    {
        h(tq);
        cnot(cq2,tq);
        tdag(tq);
        cnot(cq1,tq);
        t(tq);
        cnot(cq2,tq);
        tdag(tq);
        cnot(cq1,tq);
        tdag(cq2);
        t(tq);
        cnot(cq1,cq2);
        h(tq);
        tdag(cq2);
        cnot(cq1,cq2);
        t(cq1);
        s(cq2);
    }

    void controlled_swap(size_t tq1, size_t tq2, size_t cq)
    {
        // from: https://arxiv.org/pdf/1210.0974.pdf
        // Quantum circuits of T-depth one
        cnot(tq2, tq1);
        cnot(cq, tq1);
        h(tq2);
        t(cq);
        tdag(tq1);
        t(tq2);
        cnot(tq2, tq1);
        cnot(cq, tq2);
        t(tq1);
        cnot(cq, tq1);
        tdag(tq2);
        tdag(tq1);
        cnot(cq, tq2);
        cnot(tq2, tq1);
        t(tq1);
        h(tq2);
        cnot(tq2, tq1);
    }
    void controlled_rx(size_t tq, size_t cq, double theta)
    {
        rx(tq, theta/2);
        cz(cq, tq);
        rx(tq, -theta/2);
        cz(cq, tq);
    }
    void controlled_ry(size_t tq, size_t cq, double theta)
    {
        ry(tq, theta/2);
        cnot(cq, tq);
        ry(tq, -theta/2);
        cnot(cq, tq);
    }
    void controlled_rz(size_t tq, size_t cq, double theta)
    {
        rz(tq, theta/2);
        cnot(cq, tq);
        rz(tq, -theta/2);
        cnot(cq, tq);
    }


    void controlled_single(ql::quantum_kernel *k, size_t control_qubit, size_t ancilla_qubit)
    {
        ql::circuit& ckt = k->get_circuit();
        for( auto & g : ckt )
        {
            std::string gname = g->name;
            ql::gate_type_t gtype = g->type();
            std::vector<size_t> goperands = g->operands;
            DOUT("Generating controlled gate for " << gname);
            DOUT("Type : " << gtype);
            if( __pauli_x_gate__ == gtype  || __rx180_gate__ == gtype )
            {
                size_t tq = goperands[0];
                size_t cq = control_qubit;
                controlled_x(tq, cq);
            }
            else if( __pauli_y_gate__ == gtype  || __ry180_gate__ == gtype )
            {
                size_t tq = goperands[0];
                size_t cq = control_qubit;
                controlled_y(tq, cq);
            }
            else if( __pauli_z_gate__ == gtype )
            {
                size_t tq = goperands[0];
                size_t cq = control_qubit;
                controlled_z(tq, cq);
            }
            else if( __hadamard_gate__ == gtype )
            {
                size_t tq = goperands[0];
                size_t cq = control_qubit;
                controlled_h(tq, cq);
            }
            else if( __identity_gate__ == gtype )
            {
                size_t tq = goperands[0];
                size_t cq = control_qubit;
                controlled_i(tq, cq);
            }
            else if( __t_gate__ == gtype )
            {
                size_t tq = goperands[0];
                size_t cq = control_qubit;
                size_t aq = ancilla_qubit;
                controlled_t(tq, cq, aq);
            }
            else if( __tdag_gate__ == gtype )
            {
                size_t tq = goperands[0];
                size_t cq = control_qubit;
                size_t aq = ancilla_qubit;
                controlled_tdag(tq, cq, aq);
            }
            else if( __phase_gate__ == gtype )
            {
                size_t tq = goperands[0];
                size_t cq = control_qubit;
                controlled_s(tq, cq);
            }
            else if( __phasedag_gate__ == gtype )
            {
                size_t tq = goperands[0];
                size_t cq = control_qubit;
                controlled_sdag(tq, cq);
            }
            else if( __cnot_gate__ == gtype )
            {
                size_t cq1 = goperands[0];
                size_t cq2 = control_qubit;
                size_t tq = goperands[1];

                auto opt = ql::options::get("decompose_toffoli");
                if ( opt == "AM" )
                {
                    controlled_cnot_AM(tq, cq1, cq2);
                }
                else if ( opt == "NC" )
                {
                    controlled_cnot_NC(tq, cq1, cq2);
                }
                else
                {
                    toffoli(cq1, cq2, tq);
                }
            }
            else if( __swap_gate__ == gtype )
            {
                size_t tq1 = goperands[0];
                size_t tq2 = goperands[1];
                size_t cq = control_qubit;
                controlled_swap(tq1, tq2, cq);
            }
            else if( __rx_gate__ == gtype )
            {
                size_t tq = goperands[0];
                size_t cq = control_qubit;
                controlled_rx(tq, cq, g->angle);
            }
            else if( __ry_gate__ == gtype )
            {
                size_t tq = goperands[0];
                size_t cq = control_qubit;
                controlled_ry(tq, cq, g->angle);
            }
            else if( __rz_gate__ == gtype )
            {
                size_t tq = goperands[0];
                size_t cq = control_qubit;
                controlled_rz(tq, cq, g->angle);
            }
            else if( __rx90_gate__ == gtype )
            {
                size_t tq = goperands[0];
                size_t cq = control_qubit;
                controlled_rx(tq, cq, PI/2);
            }
            else if( __mrx90_gate__ == gtype )
            {
                size_t tq = goperands[0];
                size_t cq = control_qubit;
                controlled_rx(tq, cq, -1*PI/2);
            }
            else if( __rx180_gate__ == gtype )
            {
                size_t tq = goperands[0];
                size_t cq = control_qubit;
                controlled_rx(tq, cq, PI);
                // controlled_x(tq, cq);
            }
            else if( __ry90_gate__ == gtype )
            {
                size_t tq = goperands[0];
                size_t cq = control_qubit;
                controlled_ry(tq, cq, PI/4);
            }
            else if( __mry90_gate__ == gtype )
            {
                size_t tq = goperands[0];
                size_t cq = control_qubit;
                controlled_ry(tq, cq, -1*PI/4);
            }
            else if( __ry180_gate__ == gtype )
            {
                size_t tq = goperands[0];
                size_t cq = control_qubit;
                controlled_ry(tq, cq, PI);
                // controlled_y(tq, cq);
            }
            else
            {
                EOUT("Controlled version of gate '" << gname << "' not defined !");
                throw ql::exception("[x] error : ql::kernel::controlled : Controlled version of gate '"+gname+"' not defined ! ",false);
            }
        }
    }

    void controlled(ql::quantum_kernel *k,
                    std::vector<size_t> control_qubits,
                    std::vector<size_t> ancilla_qubits
                   )
    {
        DOUT("Generating controlled kernel ... ");
        int ncq = control_qubits.size();
        int naq = ancilla_qubits.size();

        if( ncq == 0 )
        {
            EOUT("At least one control_qubits should be specified !");
            throw ql::exception("[x] error : ql::kernel::controlled : At least one control_qubits should be specified !",false);
        }
        else if( ncq == 1 )
        {
            //                      control               ancilla
            controlled_single(k, control_qubits[0], ancilla_qubits[0]);
        }
        else if( ncq > 1 )
        {
            // Network implementing C^n(U) operation
            // - based on Fig. 4.10, p.p 185, Nielson & Chuang
            // - requires as many ancilla/work qubits as control qubits
            if(naq == ncq)
            {
                toffoli(control_qubits[0], control_qubits[1], ancilla_qubits[0]);

                for(int n=0; n<=naq-3; n++)
                {
                    toffoli(control_qubits[n+2], ancilla_qubits[n], ancilla_qubits[n+1]);
                }

                //                      control               ancilla
                controlled_single(k, ancilla_qubits[naq-2], ancilla_qubits[naq-1]);

                for(int n=naq-3; n>=0; n--)
                {
                    toffoli(control_qubits[n+2], ancilla_qubits[n], ancilla_qubits[n+1]);
                }

                toffoli(control_qubits[0], control_qubits[1], ancilla_qubits[0]);
            }
            else
            {
                EOUT("No. of control qubits should be equal to No. of ancilla qubits!");
                throw ql::exception("[x] error : ql::kernel::controlled : No. of control qubits should be equal to No. of ancilla qubits!",false);
            }
        }

        DOUT("Generating controlled kernel [Done]");
    }

    void conjugate(ql::quantum_kernel *k)
    {
        COUT("Generating conjugate kernel");
        ql::circuit& ckt = k->get_circuit();
        for( auto rgit = ckt.rbegin(); rgit != ckt.rend(); ++rgit )
        {
            auto g = *rgit;
            std::string gname = g->name;
            ql::gate_type_t gtype = g->type();
            DOUT("Generating conjugate gate for " << gname);
            DOUT("Type : " << gtype);
            if( __pauli_x_gate__ == gtype  || __rx180_gate__ == gtype )
            {
                gate("x", g->operands, {}, g->duration, g->angle);
            }
            else if( __pauli_y_gate__ == gtype  || __ry180_gate__ == gtype )
            {
                gate("y", g->operands, {}, g->duration, g->angle);
            }
            else if( __pauli_z_gate__ == gtype )
            {
                gate("z", g->operands, {}, g->duration, g->angle);
            }
            else if( __hadamard_gate__ == gtype )
            {
                gate("hadamard", g->operands, {}, g->duration, g->angle);
            }
            else if( __identity_gate__ == gtype )
            {
                gate("identity", g->operands, {}, g->duration, g->angle);
            }
            else if( __t_gate__ == gtype )
            {
                gate("tdag", g->operands, {}, g->duration, g->angle);
            }
            else if( __tdag_gate__ == gtype )
            {
                gate("t", g->operands, {}, g->duration, g->angle);
            }
            else if( __phase_gate__ == gtype )
            {
                gate("sdag", g->operands, {}, g->duration, g->angle);
            }
            else if( __phasedag_gate__ == gtype )
            {
                gate("s", g->operands, {}, g->duration, g->angle);
            }
            else if( __cnot_gate__ == gtype )
            {
                gate("cnot", g->operands, {}, g->duration, g->angle);
            }
            else if( __swap_gate__ == gtype )
            {
                gate("swap", g->operands, {}, g->duration, g->angle);
            }
            else if( __rx_gate__ == gtype )
            {
                gate("rx", g->operands, {}, g->duration, -(g->angle) );
            }
            else if( __ry_gate__ == gtype )
            {
                gate("ry", g->operands, {}, g->duration, -(g->angle) );
            }
            else if( __rz_gate__ == gtype )
            {
                gate("rz", g->operands, {}, g->duration, -(g->angle) );
            }
            else if( __rx90_gate__ == gtype )
            {
                gate("mrx90", g->operands, {}, g->duration, g->angle);
            }
            else if( __mrx90_gate__ == gtype )
            {
                gate("rx90", g->operands, {}, g->duration, g->angle);
            }
            else if( __rx180_gate__ == gtype )
            {
                gate("x", g->operands, {}, g->duration, g->angle);
            }
            else if( __ry90_gate__ == gtype )
            {
                gate("mry90", g->operands, {}, g->duration, g->angle);
            }
            else if( __mry90_gate__ == gtype )
            {
                gate("ry90", g->operands, {}, g->duration, g->angle);
            }
            else if( __ry180_gate__ == gtype )
            {
                gate("y", g->operands, {}, g->duration, g->angle);
            }
            else if( __cphase_gate__ == gtype )
            {
                gate("cphase", g->operands, {}, g->duration, g->angle);
            }
            else if( __toffoli_gate__ == gtype )
            {
                gate("toffoli", g->operands, {}, g->duration, g->angle);
            }
            else
            {
                EOUT("Conjugate version of gate '" << gname << "' not defined !");
                throw ql::exception("[x] error : ql::kernel::conjugate : Conjugate version of gate '"+gname+"' not defined ! ",false);
            }
        }
        COUT("Generating conjugate kernel [Done]");
    }

public:
    std::string   name;
    circuit       c;
    size_t        iterations;
    size_t        qubit_count;
    size_t        creg_count;
    size_t        cycle_time;
    kernel_type_t type;
    operation     br_condition;
    std::map<std::string,custom_gate*> gate_definition;     // FIXME: consider using instruction_map_t
};


} // namespace ql

#endif // QL_KERNEL_H
