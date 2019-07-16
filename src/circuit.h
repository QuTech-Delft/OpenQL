/**
 * @file   circuit.h
 * @date   11/2016
 * @author Nader Khammassi
 * @brief  circuit (i.e. gate container) implementation
 */

#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <vector>
#include <iostream>

#include "gate.h"

namespace ql
{

    typedef std::vector<gate*> circuit;


    inline void print(circuit& c)
    {
        std::cout << "-------------------" << std::endl;
        for (size_t i=0; i<c.size(); i++)
        	std::cout << "   " << c[i]->qasm() << std::endl;
        std::cout << "\n-------------------" << std::endl;
    }

    /**
     * generate qasm for a given circuit
     */
    inline std::string qasm(circuit& c)
    {
        std::stringstream ss;
        for (size_t i=0; i<c.size(); ++i)
        {
            ss << c[i]->qasm() << "\n";
        }
        return ss.str();
    }

    inline std::vector<circuit*> split_circuit(circuit &x)
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
        IOUT("circuit decomposition done (" << cs.size() << ").");
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
    inline bool contains_measurements(circuit &x)
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

#if OPT_UNFINISHED_OPTIMIZATION
    /**
     * detect unoptimizable gates
     */
    inline bool contains_unoptimizable_gates(circuit &x)
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
#endif


}

#endif // CIRCUIT_H
