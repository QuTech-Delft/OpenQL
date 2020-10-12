/**
 * @file   eqasm_backend_cc.h
 * @date   201809xx
 * @author Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief  eqasm backend for the Central Controller
 * @remark based on cc_light_eqasm_compiler.h, commit f34c0d9
 */

#ifndef ARCH_CC_EQASM_BACKEND_CC_H
#define ARCH_CC_EQASM_BACKEND_CC_H

#include "codegen_cc.h"

#include <eqasm_compiler.h>
#include <kernel.h>
#include <platform.h>
#include <circuit.h>

#include <string>
#include <vector>

namespace ql
{
namespace arch
{

class eqasm_backend_cc : public eqasm_compiler
{
public:
    eqasm_backend_cc() = default;
    ~eqasm_backend_cc() = default;

    void compile(quantum_program* programp, const ql::quantum_platform &platform) override;
    // void compile(std::string prog_name, ql::circuit &ckt, ql::quantum_platform &platform) override;

private:
    std::string kernelLabel(ql::quantum_kernel &k);
    void codegenClassicalInstruction(ql::gate *classical_ins);
    void codegenKernelPrologue(ql::quantum_kernel &k);
    void codegenKernelEpilogue(ql::quantum_kernel &k);
    void codegenBundles(ql::ir::bundles_t &bundles, const ql::quantum_platform &platform);
    void loadHwSettings(const ql::quantum_platform &platform);

private: // vars
    codegen_cc codegen;
    int bundleIdx;
}; // class

} // arch
} // ql

#endif // ARCH_CC_EQASM_BACKEND_CC_H

