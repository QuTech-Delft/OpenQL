/**
 * @file   circuit.h
 * @date   11/2016
 * @author Nader Khammassi
 * @brief  gate container implementation
 */

#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <vector>
#include <iostream>

#include "gate.h"

namespace ql
{

    typedef std::vector<gate*> circuit;


    void print(circuit& c)
    {
        std::cout << "-------------------" << std::endl;
        for (size_t i=0; i<c.size(); i++)
        	std::cout << "   " << c[i]->qasm() << std::endl;
        std::cout << "\n-------------------" << std::endl;
    }

    /**
     * generate qasm for a give circuit
     */
    std::string qasm(ql::circuit c)
    {
        std::stringstream ss;
        for (size_t i=0; i<c.size(); ++i)
        {
            ss << c[i]->qasm() << "\n";
            // COUT(c[i]->qasm());
        }
        return ss.str();
    }

}

#endif // CIRCUIT_H
