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

#define __print_circuit__ 0

namespace ql
{
typedef std::vector<gate*> circuit;


void print(circuit& c)
{
#if __print_circuit__
    std::cout << "-------------------" << std::endl;
    for (int i=0; i<c.size(); i++)
        std::cout << "   " << c[i]->qasm();
    std::cout << "\n-------------------" << std::endl;
#endif // __print_circuit__
}
}

#endif // CIRCUIT_H
