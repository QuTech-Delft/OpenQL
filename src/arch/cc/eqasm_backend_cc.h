/**
 * @file   eqasm_backend_cc.h
 * @date   201809xx
 * @author Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief  eqasm backend for the Central Controller
 * @remark based on cc_light_eqasm_compiler.h, commit f34c0d9
 */

#pragma once

#include "codegen_cc.h"

#include <eqasm_compiler.h>
#include <kernel.h>
#include <platform.h>
#include <circuit.h>
#include <ir.h>

#include <string>
#include <vector>

namespace ql {

class eqasm_backend_cc : public eqasm_compiler
{
public:
    eqasm_backend_cc() = default;
    ~eqasm_backend_cc() = default;

    void compile(quantum_program *program, const quantum_platform &platform) override;

private:
    std::string kernelLabel(quantum_kernel &k);
    void codegenClassicalInstruction(gate *classical_ins);
    void codegenKernelPrologue(quantum_kernel &k);
    void codegenKernelEpilogue(quantum_kernel &k);
    void codegenBundles(ir::bundles_t &bundles, const quantum_platform &platform);
    void loadHwSettings(const quantum_platform &platform);

private: // vars
    codegen_cc codegen;
    int bundleIdx;
}; // class

} // namespace ql
