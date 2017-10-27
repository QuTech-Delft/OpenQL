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

#include "json.h"
#include "utils.h"
#include "gate.h"
#include "optimizer.h"
#ifndef __disable_lemon__
#include "scheduler.h"
#endif // __disable_lemon__

namespace ql
{
// un-comment it to decompose
// #define DECOMPOSE

/**
 * quantum_kernel
 */
class quantum_kernel
{
public:

    quantum_kernel(std::string name, ql::quantum_platform& platform) : name(name), iterations(1)
    {
        gate_definition = platform.instruction_map;
        qubit_number = platform.qubit_number;
    }

    void loop(size_t it)
    {
        iterations = it;
    }

    void identity(size_t qubit)
    {
        gate("identity", {qubit} );
        // std::string gname("identity");
        // bool added = add_custom_gate_if_available( gname, {qubit} );
        // if(!added)
        // {
        //     c.push_back(new ql::identity(qubit));
        // }
    }

    void hadamard(size_t qubit)
    {
        gate("hadamard", {qubit} );
//         std::string gname("hadamard");
//         bool added = add_custom_gate_if_available( gname, {qubit} );
//         if(!added)
//         {
// #ifdef DECOMPOSE
//             c.push_back(new ql::ry90(qubit));
//             c.push_back(new ql::rx180(qubit));
// #else
//             c.push_back(new ql::hadamard(qubit));
// #endif
//         }
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
        gate("s", {qubit} );
        // std::string gname("s");
        // bool added = add_custom_gate_if_available( gname, {qubit} );
        // if(!added)
        // {
        //     c.push_back(new ql::phase(qubit));
        // }
    }

    void sdag(size_t qubit)
    {
        gate("sdag", {qubit} );
        // std::string gname("sdag");
        // bool added = add_custom_gate_if_available( gname, {qubit} );
        // if(!added)
        // {
        //     c.push_back(new ql::phasedag(qubit));
        // }
    }

    void t(size_t qubit)
    {
        gate("t", {qubit} );
        // std::string gname("t");
        // bool added = add_custom_gate_if_available( gname, {qubit} );
        // if(!added)
        // {
        //     c.push_back(new ql::t(qubit));
        // }
    }

    void tdag(size_t qubit)
    {
        gate("tdag", {qubit} );
        // std::string gname("tdag");
        // bool added = add_custom_gate_if_available( gname, {qubit} );
        // if(!added)
        // {
        //     c.push_back(new ql::tdag(qubit));
        // }
    }

    void x(size_t qubit)
    {
        gate("x", {qubit} );
//         std::string gname("x");
//         bool added = add_custom_gate_if_available( gname, {qubit} );
//         if(!added)
//         {
// #ifdef DECOMPOSE
//             c.push_back(new ql::rx180(qubit));
// #else
//             c.push_back(new ql::pauli_x(qubit));
// #endif
//         }
    }

    void y(size_t qubit)
    {
        gate("y", {qubit} );
//         std::string gname("y");
//         bool added = add_custom_gate_if_available( gname, {qubit} );
//         if(!added)
//         {
// #ifdef DECOMPOSE
//             c.push_back(new ql::ry180(qubit));
// #else
//             c.push_back(new ql::pauli_y(qubit));
// #endif
//         }
    }

    void z(size_t qubit)
    {
        gate("z", {qubit} );
//         std::string gname("z");
//         bool added = add_custom_gate_if_available( gname, {qubit} );
//         if(!added)
//         {
// #ifdef DECOMPOSE
//             c.push_back(new ql::ry180(qubit));
//             c.push_back(new ql::rx180(qubit));
// #else
//             c.push_back(new ql::pauli_z(qubit));
// #endif
//         }
    }

    void rx90(size_t qubit)
    {
        gate("rx90", {qubit} );
        // std::string gname("rx90");
        // bool added = add_custom_gate_if_available( gname, {qubit} );
        // if(!added)
        // {
        //     c.push_back(new ql::rx90(qubit));
        // }
    }

    void mrx90(size_t qubit)
    {
        gate("mrx90", {qubit} );
        // std::string gname("mrx90");
        // bool added = add_custom_gate_if_available( gname, {qubit} );
        // if(!added)
        // {
        //     c.push_back(new ql::mrx90(qubit));
        // }
    }

    void rx180(size_t qubit)
    {
        gate("rx180", {qubit} );
        // std::string gname("rx180");
        // bool added = add_custom_gate_if_available( gname, {qubit} );
        // if(!added)
        // {
        //     c.push_back(new ql::rx180(qubit));
        // }
    }

    void ry90(size_t qubit)
    {
        gate("ry90", {qubit} );
        // std::string gname("ry90");
        // bool added = add_custom_gate_if_available( gname, {qubit} );
        // if(!added)
        // {
        //     c.push_back(new ql::ry90(qubit));
        // }
    }

    void mry90(size_t qubit)
    {
        gate("mry90", {qubit} );
        // std::string gname("mry90");
        // bool added = add_custom_gate_if_available( gname, {qubit} );
        // if(!added)
        // {
        //     c.push_back(new ql::mry90(qubit));
        // }
    }

    void ry180(size_t qubit)
    {
        gate("ry180", {qubit} );
        // std::string gname("ry180");
        // bool added = add_custom_gate_if_available( gname, {qubit} );
        // if(!added)
        // {
        //     c.push_back(new ql::ry180(qubit));
        // }
    }

    void measure(size_t qubit)
    {
        gate("measure", {qubit} );
        // std::string gname("measure");
        // bool added = add_custom_gate_if_available( gname, {qubit} );
        // if(!added)
        // {
        //     c.push_back(new ql::measure(qubit));
        // }
    }

    void prepz(size_t qubit)
    {
        gate("prepz", {qubit} );
        // std::string gname("prepz");
        // bool added = add_custom_gate_if_available( gname, {qubit} );
        // if(!added)
        // {
        //     // std::cout << "loading default gate with duration : " << g->duration << std::endl;
        //     c.push_back(new ql::prepz(qubit));
        // }
    }

    void cnot(size_t qubit1, size_t qubit2)
    {
        gate("cnot", {qubit1, qubit2} );
        // std::string gname("cnot");
        // bool added = add_custom_gate_if_available( gname, {qubit1, qubit2} );
        // if(!added)
        // {
        //     c.push_back(new ql::cnot(qubit1, qubit2));
        // }
    }

    void cz(size_t qubit1, size_t qubit2)
    {
        gate("cz", {qubit1, qubit2} );
        // std::string gname("cz");
        // bool added = add_custom_gate_if_available( gname, {qubit1, qubit2} );
        // if(!added)
        // {
        //     c.push_back(new ql::cphase(qubit1, qubit2));
        // }
    }

    void cphase(size_t qubit1, size_t qubit2)
    {
        gate("cphase", {qubit1, qubit2} );
        // std::string gname("cphase");
        // bool added = add_custom_gate_if_available( gname, {qubit1, qubit2} );
        // if(!added)
        // {
        //     c.push_back(new ql::cphase(qubit1, qubit2));
        // }
    }

    void toffoli(size_t qubit1, size_t qubit2, size_t qubit3)
    {
        // TODO add custom gate check if needed
        c.push_back(new ql::toffoli(qubit1, qubit2, qubit3));
    }

    /**
     * add clifford
     */
    void clifford(int id, size_t qubit=0)
    {
        switch (id)
        {
        case 0 :
            break;                                          //  ['I']
        case 1 :
            ry90(qubit);
            rx90(qubit);
            break;                //  ['Y90', 'X90']
        case 2 :
            mrx90(qubit);
            mry90(qubit);
            break;              //  ['mX90', 'mY90']
        case 3 :
            rx180(qubit);
            break;                            //  ['X180']
        case 4 :
            mry90(qubit);
            mrx90(qubit);
            break;              //  ['mY90', 'mX90']
        case 5 :
            rx90(qubit);
            mry90(qubit);
            break;               //  ['X90', 'mY90']
        case 6 :
            ry180(qubit);
            break;                            //  ['Y180']
        case 7 :
            mry90(qubit);
            rx90(qubit);
            break;               //  ['mY90', 'X90']
        case 8 :
            rx90(qubit);
            ry90(qubit);
            break;                //  ['X90', 'Y90']
        case 9 :
            rx180(qubit);
            ry180(qubit);
            break;              //  ['X180', 'Y180']
        case 10:
            ry90(qubit);
            mrx90(qubit);
            break;               //  ['Y90', 'mX90']
        case 11:
            mrx90(qubit);
            ry90(qubit);
            break;               //  ['mX90', 'Y90']
        case 12:
            ry90(qubit);
            rx180(qubit);
            break;               //  ['Y90', 'X180']
        case 13:
            mrx90(qubit);
            break;                            //  ['mX90']
        case 14:
            rx90(qubit);
            mry90(qubit);
            mrx90(qubit);
            break; //  ['X90', 'mY90', 'mX90']
        case 15:
            mry90(qubit);
            break;                            //  ['mY90']
        case 16:
            rx90(qubit);
            break;                             //  ['X90']
        case 17:
            rx90(qubit);
            ry90(qubit);
            rx90(qubit);
            break;   //  ['X90', 'Y90', 'X90']
        case 18:
            mry90(qubit);
            rx180(qubit);
            break;              //  ['mY90', 'X180']
        case 19:
            rx90(qubit);
            ry180(qubit);
            break;               //  ['X90', 'Y180']
        case 20:
            rx90(qubit);
            mry90(qubit);
            rx90(qubit);
            break;  //  ['X90', 'mY90', 'X90']
        case 21:
            ry90(qubit);
            break;                             //  ['Y90']
        case 22:
            mrx90(qubit);
            ry180(qubit);
            break;              //  ['mX90', 'Y180']
        case 23:
            rx90(qubit);
            ry90(qubit);
            mrx90(qubit);
            break;  //  ['X90', 'Y90', 'mX90']
        default:
            break;
        }
    }

    bool add_default_gate_if_available(std::string gname, std::vector<size_t> qubits)
    {
    	bool result=false;

        bool is_one_qubit_gate = (gname == "identity") || (gname == "hadamard") || (gname == "s") || (gname == "sdag")
                        || (gname == "t") || (gname == "tdag") || (gname == "rx90") || (gname == "mrx90") || (gname == "rx180")
                        || (gname == "ry90") || (gname == "mry90") || (gname == "ry180")
                        || (gname == "measure") || (gname == "prez");

        bool is_two_qubit_gate = (gname == "cnot") || (gname == "cz") || (gname == "cphase");

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
        else
        {
            return false;
        }

             if( gname == "identity" )   { c.push_back(new ql::identity(qubits[0]) ); result = true; }
        else if( gname == "hadamard" )   { c.push_back(new ql::hadamard(qubits[0]) ); result = true; }
        else if( gname == "s" )          { c.push_back(new ql::phase(qubits[0]) ); result = true; }
        else if( gname == "sdag" )       { c.push_back(new ql::phasedag(qubits[0]) ); result = true; }
        else if( gname == "t" )          { c.push_back(new ql::t(qubits[0]) ); result = true; }
        else if( gname == "tdag" )       { c.push_back(new ql::tdag(qubits[0]) ); result = true; }
        else if( gname == "rx90" )       { c.push_back(new ql::rx90(qubits[0]) ); result = true; }
        else if( gname == "mrx90" )      { c.push_back(new ql::mrx90(qubits[0]) ); result = true; }
        else if( gname == "rx180" )      { c.push_back(new ql::rx180(qubits[0]) ); result = true; }
        else if( gname == "ry90" )       { c.push_back(new ql::ry90(qubits[0]) ); result = true; }
        else if( gname == "mry90" )      { c.push_back(new ql::mry90(qubits[0]) ); result = true; }
        else if( gname == "ry180" )      { c.push_back(new ql::ry180(qubits[0]) ); result = true; }
        else if( gname == "measure" )    { c.push_back(new ql::measure(qubits[0]) ); result = true; }
        else if( gname == "prepz" )      { c.push_back(new ql::prepz(qubits[0]) ); result = true; }
    	else if( gname == "cnot" )       { c.push_back(new ql::cnot(qubits[0], qubits[1]) ); result = true; }
    	else if( gname == "cz" )         { c.push_back(new ql::cphase(qubits[0], qubits[1]) ); result = true; }
    	else if( gname == "cphase" )     { c.push_back(new ql::cphase(qubits[0], qubits[1]) ); result = true; }

    	return result;
    }

    bool add_custom_gate_if_available(std::string & gname, std::vector<size_t> qubits)
    {
        bool added = false;
        // first check if a specialized custom gate is available
        std::string instr = gname + " ";
        for (size_t i=0; i<(qubits.size()-1); ++i)
            instr += "q" + std::to_string(qubits[i]) + ",";
        if(qubits.size() >= 1) // to make if work with gates without operands
            instr += "q" + std::to_string(qubits[qubits.size()-1]);

        std::map<std::string,custom_gate*>::iterator it = gate_definition.find(instr);
        if (it != gate_definition.end())
        {
            custom_gate* g = new custom_gate(*(it->second));
            for(auto & qubit : qubits)
                g->operands.push_back(qubit);
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
                added = true;
                c.push_back(g);
            }
        }

        if(added)
            DOUT("custom gate added for " << gname);
        else
            DOUT("custom gate not added for " << gname);

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

    // TODO optimized parameter passing
    bool add_decomposed_gate_if_available(std::string gate_name, std::vector<size_t> all_qubits)
    {
        bool added = false;
        std::string instr_parameterized = gate_name + " ";
        size_t i;
        for(i=0; i<all_qubits.size()-1; i++)
        {
            instr_parameterized += "%" + std::to_string(i) + " ";
        }
        if(all_qubits.size() >= 1)
        {
            instr_parameterized += "%" + std::to_string(i);
        }

        DOUT("instr_parameterized: " << instr_parameterized);

        // check for composite ins
        auto it = gate_definition.find(instr_parameterized);
        if( it != gate_definition.end() )
        {
            DOUT("composite gate found for " << instr_parameterized);
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
                bool custom_added = add_custom_gate_if_available(sub_ins_name, this_gate_qubits);
                if(!custom_added)
                {
                    // default gate check
                    DOUT("adding default gate for " << sub_ins_name);
                    bool default_available = add_default_gate_if_available(sub_ins_name, this_gate_qubits);
                    if( !default_available )
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
     * custom 1 qubit gate
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
    void gate(std::string gname, std::vector<size_t> qubits = {} )
    {
        for(auto & qno : qubits)
        {
            DOUT("qno : " << qno);
            if( qno < 0 || qno >= qubit_number )
            {   
                EOUT("Number of qubits in platform: " << std::to_string(qubit_number) << ", specified qubit numbers out of range for gate: '" << gname << "' with " << ql::utils::to_string(qubits,"qubits") );
                throw ql::exception("[x] error : ql::kernel::gate() : Number of qubits in platform: "+std::to_string(qubit_number)+", specified qubit numbers out of range for gate '"+gname+"' with " +ql::utils::to_string(qubits,"qubits")+" !",false);
            }
        }
        
        // check if composite gate is available
        // if not, check if a parameterized composite gate is available
        // if not, check if a specialized custom gate is available
        // if not, check if a parameterized custom gate is available
        // if not, check if a default gate is available
        // if not, then error

        str::lower_case(gname);
        DOUT("Adding gate : " << gname << " with " << ql::utils::to_string(qubits,"qubits"));

        DOUT("trying to add decomposed gate for: " << gname);
        // specialized/parameterized composite gate check
        bool decom_added = add_decomposed_gate_if_available(gname, qubits);
        if(decom_added)
        {
            DOUT("decomposed gates added for " << gname);
        }
        else
        {
            // specialized/parameterized custom gate check
            DOUT("adding custom gate for " << gname);
            bool custom_added = add_custom_gate_if_available( gname, qubits );
            if(!custom_added)
            {
                // default gate check (which is always parameterized)
            	DOUT("adding default gate for " << gname);

				bool default_available = add_default_gate_if_available(gname, qubits);
				if( !default_available )
                {
                	EOUT("unknown gate '" << gname << "' with " << ql::utils::to_string(qubits,"qubits") );
                	throw ql::exception("[x] error : ql::kernel::gate() : the gate '"+gname+"' with " +ql::utils::to_string(qubits,"qubits")+" is not supported by the target platform !",false);
                }
                else
                {
                    DOUT("default gate added for " << gname);
                }
            }
            else
            {
                DOUT("custom gate added for " << gname);
            }
        }
        DOUT("");
    }


    /**
     * qasm
     */
    std::string qasm()
    {
        std::stringstream ss;
        ss << "." << name;
        if (iterations > 1)
            ss << "(" << iterations << ") \n";
        else
            ss << "\n";
        for (size_t i=0; i<c.size(); ++i)
        {
            ss << "   " << c[i]->qasm() << "\n";
            // std::cout << c[i]->qasm() << std::endl;
        }
        return ss.str();
    }

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

    void schedule(size_t qubits, quantum_platform platform, std::string scheduler, std::string& sched_qasm, std::string& sched_dot, bool verbose=false)
    {
#ifndef __disable_lemon__
        if (verbose) COUT( scheduler << " scheduling the quantum kernel '" << name << "'...");

        Scheduler sched;
        sched.Init(qubits, c, platform, verbose);
        // sched.Print(verbose);
        // sched.PrintMatrix(verbose);
        // sched.PrintDot(verbose);

        if("ASAP" == scheduler)
        {
            // sched.PrintScheduleASAP();
            // sched.PrintDotScheduleASAP();
            // sched_dot = sched.GetDotScheduleASAP();
            // sched.PrintQASMScheduledASAP();
            sched_qasm = sched.GetQASMScheduledASAP();
        }
        else if("ALAP" == scheduler)
        {
            // sched.PrintScheduleALAP();
            // sched.PrintDotScheduleALAP();
            // sched_dot = sched.GetDotScheduleALAP();
            // sched.PrintQASMScheduledALAP();
            sched_qasm = sched.GetQASMScheduledALAP();
        }
        else
        {
            EOUT("Unknown scheduler");
        }
#endif // __disable_lemon__
    }

    std::vector<circuit*> split_circuit(circuit x, bool verbose=false)
    {
        if (verbose) COUT("circuit decomposition in basic blocks ... ");
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
        if (verbose) println("circuit decomposion done (" << cs.size() << ").");
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
            COUT(" |- qumis : \n" << i->second->micro_code());
        }
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

protected:

    std::string name;
    circuit     c;
    size_t      iterations;
    size_t      qubit_number;

    std::map<std::string,custom_gate*> gate_definition;
};




} // namespace ql

#endif // QL_KERNEL_H
