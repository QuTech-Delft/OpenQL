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

    void compile(const ir::ProgramRef &program, const plat::PlatformRef &platform) override;

private:
    static Str kernelLabel(const ir::KernelRef &k);
    void codegenClassicalInstruction(const ir::GateRef &classical_ins);
    void codegenKernelPrologue(const ir::KernelRef &k);
    void codegenKernelEpilogue(const ir::KernelRef &k);
    void codegenBundles(ir::Bundles &bundles, const plat::PlatformRef &platform);
    void loadHwSettings(const plat::PlatformRef &platform);

private: // vars
    Codegen codegen;
    Int bundleIdx;
}; // class

} // namespace cc
} // namespace arch
} // namespace ql
