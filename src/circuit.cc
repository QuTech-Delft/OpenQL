#include "circuit.h"

#include <iostream>

namespace ql {

void print(const circuit& c) {
    std::cout << "-------------------" << std::endl;
    for (size_t i = 0; i < c.size(); i++) {
        std::cout << "   " << c[i]->qasm() << std::endl;
    }
    std::cout << "\n-------------------" << std::endl;
}

/**
 * generate qasm for a given circuit
 */
std::string qasm(const circuit& c) {
    std::stringstream ss;
    for (size_t i = 0; i < c.size(); ++i) {
        ss << c[i]->qasm() << "\n";
    }
    return ss.str();
}

std::vector<circuit*> split_circuit(circuit &x) {
    IOUT("circuit decomposition in basic blocks ... ");
    std::vector<circuit *> cs;
    cs.push_back(new circuit());
    for (size_t i = 0; i < x.size(); i++) {
        if ((x[i]->type() == __prepz_gate__) ||
            (x[i]->type() == __measure_gate__)) {
            cs.push_back(new circuit());
            cs.back()->push_back(x[i]);
            cs.push_back(new circuit());
        } else {
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
bool contains_measurements(const circuit &x) {
    for (size_t i = 0; i < x.size(); i++) {
        if (x[i]->type() == __measure_gate__)
            return true;
        if (x[i]->type() == __prepz_gate__)
            return true;
    }
    return false;
}

} // namespace ql
