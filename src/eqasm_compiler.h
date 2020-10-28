/**
 * @file   eqasm_compiler.h
 * @date   07/2017
 * @author Nader Khammassi
 * @brief  executable qasm compiler interface
 */

#pragma once

#include "program.h"
#include "platform.h"

namespace ql {

typedef std::vector<std::string> eqasm_t;

class quantum_platform;

/**
 * eqasm compiler interface
 */
class eqasm_compiler {
public:
    eqasm_t eqasm_code;

    virtual ~eqasm_compiler() = default;

    /*
     * compile must be implemented by all compilation backends.
     */
    virtual void compile(ql::quantum_program *programp, const ql::quantum_platform &plat) = 0;

    /**
     * write eqasm code to file/stdout
     */
    virtual void write_eqasm(const std::string &file_name="");

    /**
     * write traces
     */
    virtual void write_traces(const std::string &file_name="");

};

} // namespace ql
