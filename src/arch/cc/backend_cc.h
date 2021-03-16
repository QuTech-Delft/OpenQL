/**
 * @file   arch/cc/backend_cc.h
 * @date   201809xx
 * @author Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief  eqasm backend for the Central Controller
 * @remark based on cc_light_eqasm_compiler.h, commit f34c0d9
 */

#pragma once

#include "types_cc.h"

#include "program.h"
#include "eqasm_compiler.h"
#include "ir.h"
#include "codegen_cc.h"

namespace ql {
namespace arch {
namespace cc {

class Backend : public eqasm_compiler {
public:
    Backend() = default;
    ~Backend() override = default;

    void compile(quantum_program *program, const quantum_platform &platform) override;

private:
    static Str kernelLabel(quantum_kernel &k);
    void codegenClassicalInstruction(gate *classical_ins);
    void codegenKernelPrologue(quantum_kernel &k);
    void codegenKernelEpilogue(quantum_kernel &k);
    void codegenBundles(ir::bundles_t &bundles, const quantum_platform &platform);
    void loadHwSettings(const quantum_platform &platform);

private: // vars
    Codegen codegen;
    Int bundleIdx;
}; // class

} // namespace cc
} // namespace arch
} // namespace ql
