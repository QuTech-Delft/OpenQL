/**
 * @file   arch/cc/backend_cc.h
 * @date   201809xx
 * @author Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief  eqasm backend for the Central Controller
 * @remark based on cc_light_eqasm_compiler.h, commit f34c0d9
 */

#pragma once

#include "types_cc.h"

#include "ql/ir/ir.h"
#include "eqasm_compiler.h"
#include "codegen_cc.h"

namespace ql {
namespace arch {
namespace cc {

class Backend : public eqasm_compiler {
public:
    Backend() = default;
    ~Backend() override = default;

    void compile(ir::Program &program, const quantum_platform &platform) override;

private:
    static Str kernelLabel(ir::Kernel &k);
    void codegenClassicalInstruction(ir::Gate &classical_ins);
    void codegenKernelPrologue(ir::Kernel &k);
    void codegenKernelEpilogue(ir::Kernel &k);
    void codegenBundles(ir::Bundles &bundles, const quantum_platform &platform);
    void loadHwSettings(const quantum_platform &platform);

private: // vars
    Codegen codegen;
    Int bundleIdx;
}; // class

} // namespace cc
} // namespace arch
} // namespace ql
