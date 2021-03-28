/** \file
 * Circuit (i.e. gate container) implementation.
 */

#include "circuit.h"

#include <iostream>

namespace ql {
namespace ir {

using namespace utils;

void print(const Circuit &c) {
    std::cout << "-------------------" << std::endl;
    for (auto gate : c) {
        std::cout << "   " << gate->qasm() << std::endl;
    }
    std::cout << "\n-------------------" << std::endl;
}

/**
 * generate qasm for a given circuit
 */
Str qasm(const Circuit &c) {
    StrStrm ss;
    for (auto gate : c) {
        ss << gate->qasm() << "\n";
    }
    return ss.str();
}

Vec<Circuit> split_circuit(Circuit &x) {
    QL_IOUT("circuit decomposition in basic blocks ... ");
    Vec<Circuit> cs;
    cs.emplace_back();
    for (auto gate : x) {
        if ((gate->type() == GateType::PREP_Z) ||
            (gate->type() == GateType::MEASURE)) {
            cs.emplace_back();
            cs.back().add(gate);
            cs.emplace_back();
        } else {
            cs.back().add(gate);
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
Bool contains_measurements(const Circuit &x) {
    for (auto gate : x) {
        if (gate->type() == GateType::MEASURE) {
            return true;
        }
        if (gate->type() == GateType::PREP_Z) {
            return true;
        }
    }
    return false;
}

} // namespace ir
} // namespace ql
