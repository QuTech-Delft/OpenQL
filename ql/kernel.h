/**
 * @file   kernel.h
 * @date   04/2017
 * @author Nader Khammassi
 *         Imran Ashraf
 * @brief  openql kernel
 */

#ifndef QL_KERNEL_H
#define QL_KERNEL_H

#include "json.h"
#include "utils.h"
#include "gate.h"
#include "optimizer.h"
#include "dependenceGraph.h"

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

    quantum_kernel(std::string name) : name(name), iterations(1)
    {
    }

    void loop(size_t it)
    {
        iterations = it;
    }

    void identity(size_t qubit)
    {
        c.push_back(new ql::identity(qubit));
    }

    void hadamard(size_t qubit)
    {
        // qubit not used (default : q0)
#ifdef DECOMPOSE
        c.push_back(new ql::ry90(qubit));
        c.push_back(new ql::rx180(qubit));
#else
        c.push_back(new ql::hadamard(qubit));
#endif
    }

    void s(size_t qubit)
    {
        // qubit not used (default : q0)
        c.push_back(new ql::phase(qubit));
    }

    void sdag(size_t qubit)
    {
        // qubit not used (default : q0)
        c.push_back(new ql::phasedag(qubit));
    }

    void x(size_t qubit)
    {
        // qubit not used (default : q0)
#ifdef DECOMPOSE
        c.push_back(new ql::rx180(qubit));
#else
        c.push_back(new ql::pauli_x(qubit));
#endif
    }

    void y(size_t qubit)
    {
        // qubit not used (default : q0)
#ifdef DECOMPOSE
        c.push_back(new ql::ry180(qubit));
#else
        c.push_back(new ql::pauli_y(qubit));
#endif
    }

    void z(size_t qubit)
    {
        // qubit not used (default : q0)
#ifdef DECOMPOSE
        c.push_back(new ql::ry180(qubit));
        c.push_back(new ql::rx180(qubit));
#else
        c.push_back(new ql::pauli_z(qubit));
#endif
    }


    void rx90(size_t qubit)
    {
        // qubit not used (default : q0)
        c.push_back(new ql::rx90(qubit));
    }

    void mrx90(size_t qubit)
    {
        // qubit not used (default : q0)
        c.push_back(new ql::mrx90(qubit));
    }

    void rx180(size_t qubit)
    {
        // qubit not used (default : q0)
        c.push_back(new ql::rx180(qubit));
    }

    void ry90(size_t qubit)
    {
        // qubit not used (default : q0)
        c.push_back(new ql::ry90(qubit));
    }

    void mry90(size_t qubit)
    {
        // qubit not used (default : q0)
        c.push_back(new ql::mry90(qubit));
    }

    void ry180(size_t qubit)
    {
        // qubit not used (default : q0)
        c.push_back(new ql::ry180(qubit));
    }

    void measure(size_t qubit)
    {
        // qubit not used (default : q0)
        c.push_back(new ql::measure(qubit));
    }

    void prepz(size_t qubit)
    {
        // qubit not used (default : q0)
        c.push_back(new ql::prepz(qubit));
    }

    void cnot(size_t qubit1, size_t qubit2)
    {
        c.push_back(new ql::cnot(qubit1, qubit2));
    }

    void cphase(size_t qubit1, size_t qubit2)
    {
        c.push_back(new ql::cphase(qubit1, qubit2));
    }

    void toffoli(size_t qubit1, size_t qubit2, size_t qubit3)
    {
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

    /**
     * custom single qubit gate
     */
    void gate(std::string name, size_t qubit)
    {
        std::map<std::string,custom_gate*>::iterator it = gate_definition.find(name);
        if (it != gate_definition.end())
        {
            custom_gate * g = new custom_gate(*(it->second));
            g->operands.push_back(qubit);
            c.push_back(g);
        }
        else
            println("[x] error : unknown gate '" << name << "' !");
    }

    /**
     * custom gate with arbitrary number of operands
     */
    void gate(std::string name, std::vector<size_t> qubits)
    {
        std::map<std::string,custom_gate*>::iterator it = gate_definition.find(name);
        if (it != gate_definition.end())
        {
            custom_gate * g = new custom_gate(*(it->second));
            g->operands = qubits;
            c.push_back(g);
        }
        else
            println("[x] error : unknown gate '" << name << "' !");
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
            ss << c[i]->qasm() << "\n";
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

    void schedule(size_t nqubits, bool verbose=false)
    {
        if (verbose) println("scheduling the quantum kernel '" << name << "'...");
        DependGraph dg;
        dg.Init(c, nqubits, verbose);
        // dg.Print();
        // dg.PrintMatrix();
        dg.PrintDot();

        // dg.PrintScheduleASAP();
        dg.PrintDotScheduleASAP();
        dg.PrintQASMScheduledASAP();

        // dg.PrintScheduleALAP();
        dg.PrintQASMScheduledALAP();
    }

    std::vector<circuit*> split_circuit(circuit x, bool verbose=false)
    {
        if (verbose) println("circuit decomposition in basic blocks ... ");
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
            println("[-] gate '" << i->first << "'");
            println(" |- qumis : \n" << i->second->micro_code());
        }
    }

protected:

    std::string name;
    circuit     c;
    size_t      iterations;

    std::map<std::string,custom_gate*> gate_definition;
};




} // namespace ql

#endif // QL_KERNEL_H
