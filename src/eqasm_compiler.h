/** \file
 * Interface for architecture-specific executable QASM backends.
 */

#pragma once

#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "ql/plat/platform.h"
#include "ql/ir/ir.h"

namespace ql {

typedef utils::Vec<utils::Str> eqasm_t;

namespace plat {
class Platform;
}

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
    virtual void compile(const ir::ProgramRef &programp, const plat::PlatformRef &plat) = 0;

    /**
     * write eqasm code to file/stdout
     */
    virtual void write_eqasm(const utils::Str &file_name="");

    /**
     * write traces
     */
    virtual void write_traces(const utils::Str &file_name="");

};

} // namespace ql
