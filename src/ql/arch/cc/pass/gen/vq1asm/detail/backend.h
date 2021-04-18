/**
 * @file   arch/cc/pass/gen/vq1asm/detail/backend.h
 * @date   201809xx
 * @author Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief  eqasm backend for the Central Controller
 * @remark based on cc_light_eqasm_compiler.h, commit f34c0d9
 */

#pragma once

#include "types.h"

#include "ql/ir/ir.h"
#include "options.h"
#include "codegen.h"

namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {
namespace detail {

class Backend {
public:
    Backend() = default;

    void compile(const ir::ProgramRef &program, const OptionsRef &options);

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

} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
