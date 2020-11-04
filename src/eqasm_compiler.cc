#include "eqasm_compiler.h"

#include <fstream>

namespace ql {

/**
 * write eqasm code to file/stdout
 */
void eqasm_compiler::write_eqasm(const std::string &file_name) {
    if (eqasm_code.empty()) {
        return;
    }
    if (file_name.empty()) {
        QL_PRINTLN("[c] eqasm code (" << eqasm_code.size() << " lines) :");
        for (std::string l : eqasm_code) {
            std::cout << l << std::endl;
        }
    } else {
        // write to file
        std::ofstream file(file_name);
        if (file.is_open()) {
            QL_IOUT("writing eqasm code (" << eqasm_code.size() << " lines) to '"
                                           << file_name << "' ...");
            for (std::string l : eqasm_code)
                file << l << std::endl;
            file.close();
        } else
            QL_EOUT("opening file '" << file_name << "' !");
    }
}

/**
 * write traces
 */
void eqasm_compiler::write_traces(const std::string &file_name) {
}

};
