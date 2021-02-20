/** \file
 * Circuit (i.e. gate container) implementation.
 */

#include "circuit.h"

#include <iostream>

namespace ql {

using namespace utils;

void print(const circuit &c) {
    std::cout << "-------------------" << std::endl;
    for (auto gate : c) {
        std::cout << "   " << gate->qasm() << std::endl;
    }
    std::cout << "\n-------------------" << std::endl;
}

/**
 * generate qasm for a given circuit
 */
Str qasm(const circuit& c) {
    StrStrm ss;
    for (auto gate : c) {
        ss << gate->qasm() << "\n";
    }
    return ss.str();
}

Vec<circuit*> split_circuit(circuit &x) {
    QL_IOUT("circuit decomposition in basic blocks ... ");
    Vec<circuit *> cs;
    cs.push_back(new circuit());
    for (auto gate : x) {
        if ((gate->type() == __prepz_gate__) ||
            (gate->type() == __measure_gate__)) {
            cs.push_back(new circuit());
            cs.back()->push_back(gate);
            cs.push_back(new circuit());
        } else {
            cs.back()->push_back(gate);
        }
    }
    QL_IOUT("circuit decomposition done (" << cs.size() << ").");
    /*
       for (Int i=0; i<cs.size(); ++i)
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
Bool contains_measurements(const circuit &x) {
    for (auto gate : x) {
        if (gate->type() == __measure_gate__) {
            return true;
        }
        if (gate->type() == __prepz_gate__) {
            return true;
        }
    }
    return false;
}

} // namespace ql
