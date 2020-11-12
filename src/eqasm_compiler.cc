/** \file
 * Interface for architecture-specific executable QASM backends.
 */

#include "eqasm_compiler.h"

#include <fstream>

namespace ql {

using namespace utils;

/**
 * write eqasm code to file/stdout
 */
void eqasm_compiler::write_eqasm(const Str &file_name) {
    if (eqasm_code.empty()) {
        return;
    }
    if (file_name.empty()) {
        QL_PRINTLN("[c] eqasm code (" << eqasm_code.size() << " lines) :");
        for (Str l : eqasm_code) {
            std::cout << l << std::endl;
        }
    } else {
        // write to file
        std::ofstream file(file_name);
        if (file.is_open()) {
            QL_IOUT("writing eqasm code (" << eqasm_code.size() << " lines) to '"
                                           << file_name << "' ...");
            for (Str l : eqasm_code)
                file << l << std::endl;
            file.close();
        } else
            QL_EOUT("opening file '" << file_name << "' !");
    }
}

/**
 * write traces
 */
void eqasm_compiler::write_traces(const Str &file_name) {
}

};
