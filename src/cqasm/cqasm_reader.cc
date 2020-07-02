#if defined(WIN32)
#define __attribute__(A) /* do nothing on Windows */
#endif

#include <math.h>  //pow
#include <stdio.h>
#include <string>
#include <vector>
#include "libQasm.hpp"
#include "platform.h"
#include "kernel.h"
#include "program.h"
#include "cqasm_reader.h"

#define PI M_PI

namespace ql
{
    cqasm_reader::cqasm_reader(const ql::quantum_platform& q_platform, ql::quantum_program& q_program) :
        platform(q_platform), program(q_program), number_of_qubits(0), sub_circuits_default_nr(1)
    {
        //Empty
    }

    cqasm_reader::~cqasm_reader()
    {
        //Empty
    }

    void cqasm_reader::string2circuit(const std::string& cqasm_str)
    {
        libQasm *libqasm = new libQasm();
        libqasm->parse_string(cqasm_str.c_str());
        int result = libqasm->getParseResult();
        if (!result)
        {
            add_cqasm(libqasm->getQasmRepresentation());
        }
        else
        {
            FATAL("Error in parsing cqasm string '" << cqasm_str << "'");
        }
        delete libqasm;
    }

    void cqasm_reader::file2circuit(const std::string& cqasm_file_path)
    {
        libQasm *libqasm = new libQasm();
        libqasm->parse_file(cqasm_file_path.c_str());
        int result = libqasm->getParseResult();
        if (!result)
        {
            add_cqasm(libqasm->getQasmRepresentation());
        }
        else
        {
            FATAL("Error in parsing cqasm file '" << cqasm_file_path << "'");
        }
        delete libqasm;
    }

    void cqasm_reader::add_cqasm(compiler::QasmRepresentation cqasm_repr)
    {
        if (number_of_qubits != 0)
        {
            if (cqasm_repr.numQubits() != number_of_qubits)
            {
                FATAL("Adding cqasm circuits with different number of qubits to the same program");
            }
        }
        number_of_qubits = cqasm_repr.numQubits();

        if (cqasm_repr.getErrorModelType() != "None")
        {
            WOUT("Error model '" + cqasm_repr.getErrorModelType() + "' ignored");
        }

        for (auto subcircuit : cqasm_repr.getSubCircuits().getAllSubCircuits())
        {
            std::string sc_name = subcircuit.nameSubCircuit();
            // make the kernel name unique
            sc_name.append("_" + std::to_string(sub_circuits_default_nr++));
            int numIterations = subcircuit.numberIterations();

            //kernel_name must be unique
            ql::quantum_kernel kernel(sc_name, platform, number_of_qubits);
            for (auto ops_cluster : subcircuit.getOperationsCluster())
            {
                bool is_parallel = ops_cluster->isParallel();
                if (is_parallel)
                {
                    //are these supported by OpenQL??
                    WOUT("Parallel gates not supported, adding the gates in sequence");
                }

                for (auto ops : ops_cluster->getOperations())
                {
                   add_kernel_operation(kernel, *ops, number_of_qubits);
                }
            }

            // add the kernel to program
            if (numIterations > 1)
            {
                program.add_for(kernel, numIterations);
            }
            else
            {
                program.add(kernel);
            }
        }
    }

    void cqasm_reader::add_single_bit_kernel_operation(ql::quantum_kernel& kernel, const std::string& gate_type, const compiler::Operation& operation)
    {
        std::vector<size_t> qubits = operation.getQubitsInvolved().getSelectedQubits().getIndices();
        for (size_t qubit : qubits)
        {
            kernel.gate(gate_type, {qubit});
        }
    }

    void cqasm_reader::add_parameterized_single_bit_kernel_operation(ql::quantum_kernel &kernel, const std::string &gate_type, const compiler::Operation& operation)
    {
        double angle = operation.getRotationAngle();
        std::vector<size_t> qubits = operation.getQubitsInvolved().getSelectedQubits().getIndices();
        for (size_t qubit : qubits)
        {
            kernel.gate(gate_type, {qubit}, {}, 0, angle);
        }
    }

    void cqasm_reader::add_dual_bit_kernel_operation(ql::quantum_kernel& kernel, const std::string& gate_type, const compiler::Operation& operation)
    {
        size_t sgmq_indices = operation.getQubitsInvolved(1).getSelectedQubits().getIndices().size();
        for (size_t index = 0; index < sgmq_indices; index++)
        {
            size_t qubit1 = operation.getQubitsInvolved(1).getSelectedQubits().getIndices()[index];
            size_t qubit2 = operation.getQubitsInvolved(2).getSelectedQubits().getIndices()[index];
            kernel.gate(gate_type, {qubit1, qubit2});
        }
    }

    void cqasm_reader::add_parameterized_dual_bit_kernel_operation(ql::quantum_kernel& kernel, const std::string& gate_type, const compiler::Operation& operation)
    {
        double angle;
        std::string kernel_type(gate_type);
        if (kernel_type == "crk")
        {
            //convert crk to cr
            double k = operation.getRotationAngle();
            angle = PI/pow(2, k);
            kernel_type = "cr";
        }
        else
        {
            angle = operation.getRotationAngle();
        }
        size_t sgmq_indices = operation.getQubitsInvolved(1).getSelectedQubits().getIndices().size();
        for (size_t index = 0; index < sgmq_indices; index++)
        {
            size_t qubit1 = operation.getQubitsInvolved(1).getSelectedQubits().getIndices()[index];
            size_t qubit2 = operation.getQubitsInvolved(2).getSelectedQubits().getIndices()[index];
            kernel.gate(kernel_type, {qubit1, qubit2}, {}, 0, angle);
        }
    }

    void cqasm_reader::add_triple_bit_kernel_operation(ql::quantum_kernel& kernel, const std::string& gate_type, const compiler::Operation& operation)
    {
        size_t sgmq_indices = operation.getQubitsInvolved(1).getSelectedQubits().getIndices().size();
        for (size_t index = 0; index < sgmq_indices; index++)
        {
            size_t qubit1 = operation.getQubitsInvolved(1).getSelectedQubits().getIndices()[index];
            size_t qubit2 = operation.getQubitsInvolved(2).getSelectedQubits().getIndices()[index];
            size_t qubit3 = operation.getQubitsInvolved(3).getSelectedQubits().getIndices()[index];
            kernel.gate(gate_type, {qubit1, qubit2, qubit3});
        }
    }

    std::string cqasm_reader::translate_gate_type(const std::string& gate_type)
    {
        std::string kernel_type(gate_type);

        if (gate_type == "prep" || gate_type == "prep_z")
        {
            kernel_type = "prepz";
        }
        else if (gate_type == "prep_x" || gate_type == "prep_y")
        {
            //prepx, prepy
            kernel_type = gate_type.substr(0, 4) + gate_type.back();
        }
        else if (gate_type == "measure" )
        {
            kernel_type = "measz";
        }
        else if (gate_type == "measure_x" || gate_type == "measure_y"  || gate_type == "measure_z" )
        {
            //measx, measy, measz
            kernel_type = gate_type.substr(0, 4) + gate_type.back();
        }
        else if (gate_type == "x90" || gate_type == "y90")
        {
            //rx90, ry90
            kernel_type = "r" + gate_type;
        }
        else if (gate_type == "mx90" || gate_type == "my90")
        {
            //xm90, ym90
            kernel_type = gate_type.substr(1, 1) + gate_type.substr(0, 1) + gate_type.substr(2, 2);
        }
        return kernel_type;
    }

    void cqasm_reader::add_kernel_operation(ql::quantum_kernel& kernel, const compiler::Operation& operation, int number_of_qubits)
    {
        std::string gate_type = operation.getType();

        if (operation.isBitControlled())
        {
            //are these supported by OpenQL??
            EOUT("cQasm binary controlled gates not supported");
        }
        else if (gate_type == "measure" || gate_type == "prep" ||
                 gate_type == "measure_z" || gate_type == "measure_x" || gate_type == "measure_y" ||
                 gate_type == "prep_z" || gate_type == "prep_x" || gate_type == "prep_y" ||
                 gate_type == "i" || gate_type == "h" || gate_type == "x" || gate_type == "y" || gate_type == "z" ||
                 gate_type == "s" || gate_type == "sdag" || gate_type == "t" || gate_type == "tdag" ||
                 gate_type == "x90" || gate_type == "y90" || gate_type == "mx90" || gate_type == "my90")
        {
            add_single_bit_kernel_operation(kernel, translate_gate_type(gate_type), operation);
        }
        else if (gate_type == "rx" || gate_type == "ry" || gate_type == "rz")
        {
            add_parameterized_single_bit_kernel_operation(kernel, translate_gate_type(gate_type), operation);
        }
        else if (gate_type == "cnot" || gate_type == "cz" || gate_type == "swap")
        {
            add_dual_bit_kernel_operation(kernel, translate_gate_type(gate_type), operation);
        }
        else if (gate_type == "crk" || gate_type == "cr")
        {
            //are these supported by OpenQL??
            add_parameterized_dual_bit_kernel_operation(kernel, translate_gate_type(gate_type), operation);
        }
        else if (gate_type == "toffoli")
        {
            add_triple_bit_kernel_operation(kernel, translate_gate_type(gate_type), operation);
        }
        else if (gate_type == "measure_all")
        {
            for (size_t qubit = 0; int(qubit) < number_of_qubits; qubit++)
            {
                kernel.gate(translate_gate_type("measure_z"), qubit);
            }
        }
        else if (gate_type == "skip")
        {
            ///@note: skip instruction called, i.e., inserts empty cycles, possibly restarting filling cycles without waiting for all previous cycle instructions to be finished. That is, skip is different than wait that behaves as barrier+skip <X> cycles.
        }
        else if (gate_type == "wait")
        {
            size_t wait_time = operation.getWaitTime();
            kernel.gate(translate_gate_type("wait"), {}, {}, wait_time);
        }
        else if (gate_type == "display")
        {
            kernel.display();
        }
        else if (gate_type == "display_binary")
        {
            std::vector<size_t> classical_bits = operation.getDisplayBits().getSelectedBits().getIndices();
            //not supported by OpenQL??
        }
        else if (gate_type == "measure_parity")
        {
            auto measureParityProperties = operation.getMeasureParityQubitsAndAxis();
            std::vector<size_t> bits1 = measureParityProperties.first.first.getSelectedQubits().getIndices();
            std::string axis1 = measureParityProperties.second.first;
            std::vector<size_t> bits2 = measureParityProperties.first.second.getSelectedQubits().getIndices();
            std::string axis2 = measureParityProperties.second.second;
            //not supported by OpenQL??
        }
    }

    bool cqasm_reader::test_translate_gate_type()
    {
        bool result = true;

        assert(translate_gate_type("prep") == "prepz");
        assert(translate_gate_type("prep_z") == "prepz");
        assert(translate_gate_type("prep_x") == "prepx");
        assert(translate_gate_type("prep_y") == "prepy");
        assert(translate_gate_type("measure") == "measz");
        assert(translate_gate_type("measure_z") == "measz");
        assert(translate_gate_type("measure_x") == "measx");
        assert(translate_gate_type("measure_y") == "measy");
        assert(translate_gate_type("x90") == "rx90");
        assert(translate_gate_type("y90") == "ry90");
        assert(translate_gate_type("mx90") == "xm90");
        assert(translate_gate_type("my90") == "ym90");
        assert(translate_gate_type("i") == "i");
        assert(translate_gate_type("h") == "h");
        assert(translate_gate_type("s") == "s");
        assert(translate_gate_type("sdag") == "sdag");
        assert(translate_gate_type("t") == "t");
        assert(translate_gate_type("tdag") == "tdag");
        assert(translate_gate_type("x") == "x");
        assert(translate_gate_type("y") == "y");
        assert(translate_gate_type("z") == "z");
        assert(translate_gate_type("rx") == "rx");
        assert(translate_gate_type("ry") == "ry");
        assert(translate_gate_type("rz") == "rz");
        assert(translate_gate_type("toffoli") == "toffoli");
        assert(translate_gate_type("cnot") == "cnot");
        assert(translate_gate_type("cz") == "cz");
        assert(translate_gate_type("swap") == "swap");
        assert(translate_gate_type("crk") == "crk");
        assert(translate_gate_type("cr") == "cr");
        assert(translate_gate_type("display") == "display");
        assert(translate_gate_type("skip") == "skip");
        assert(translate_gate_type("wait") == "wait");
        assert(translate_gate_type("display_binary") == "display_binary");
        assert(translate_gate_type("measure_parity") == "measure_parity");

        return result;
    }

}
