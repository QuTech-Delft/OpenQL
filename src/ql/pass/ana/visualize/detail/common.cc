/** \file
 * Declaration of the visualizer's shared functionalities.
 */

#ifdef WITH_VISUALIZER

#include "common.h"

#include <iostream>
#include "ql/utils/json.h"
#include "ql/com/options.h"

namespace ql {
namespace pass {
namespace ana {
namespace visualize {
namespace detail {

using namespace utils;

Vec<GateProperties> parseGates(const ir::ProgramRef &program) {
    Vec<GateProperties> gates;

    // Determine whether the program is scheduled or not. If not, the program
    // will be visualized sequentially. Otherwise we compute program-wide,
    // zero-referenced cycle numbers below (within the IR cycles start at
    // ir::FIRST_CYCLE and are referenced to the start of the kernel rather than
    // to the complete program).
    Bool cycles_valid = true;
    for (const auto &kernel : program->kernels) {
        cycles_valid &= kernel->cycles_valid;
        for (const auto &gate : kernel->gates) {
            if (gate->cycle < ir::FIRST_CYCLE || gate->cycle >= ir::MAX_CYCLE) {
                cycles_valid = false;
                break;
            }
        }
        if (!cycles_valid) break;
    }

    UInt kernel_cycle_offset = 0;
    for (const auto &kernel : program->kernels) {
        UInt kernel_duration = 0;
        for (const auto &gate : kernel->gates) {
            Vec<Int> operands;
            Vec<Int> creg_operands;
            for (const UInt operand : gate->operands) { operands.push_back(utoi(operand)); }
            for (const UInt operand : gate->creg_operands) { creg_operands.push_back(utoi(operand)); }
            GateProperties gateProperties {
                gate->name,
                operands,
                creg_operands,
                gate->swap_params,
                utoi(gate->duration),
                0,
                gate->type(),
                {},
                "UNDEFINED"
            };
            if (cycles_valid) {
                UInt end = program->platform->time_to_cycles(gate->duration) + gate->cycle;
                kernel_duration = max(kernel_duration, end) - ir::FIRST_CYCLE;
                gateProperties.cycle = utoi(gate->cycle + kernel_cycle_offset - ir::FIRST_CYCLE);
            } else {
                gateProperties.cycle = ir::MAX_CYCLE;
            }
            gates.push_back(gateProperties);
        }
        kernel_cycle_offset += kernel_duration;
    }

    return gates;
}

Int calculateAmountOfCycles(const Vec<GateProperties> &gates, const Int cycleDuration) {
    QL_DOUT("Calculating amount of cycles...");

    // Find the highest cycle in the gate vector.
    Int amountOfCycles = 0;
    for (const GateProperties &gate : gates) {
        if (gate.cycle == ir::MAX_CYCLE) {
            QL_IOUT("Found gate with undefined cycle index. All cycle data will be discarded and circuit will be visualized sequentially.");
            return ir::MAX_CYCLE;
        }

        if (gate.cycle > amountOfCycles)
            amountOfCycles = gate.cycle;
    }

    // The last gate requires a different approach, because it might have a
    // duration of multiple cycles. None of those cycles will show up as cycle
    // index on any other gate, so we need to calculate them seperately.
    const Int lastGateDuration = gates.at(gates.size() - 1).duration;
    const Int lastGateDurationInCycles = lastGateDuration / cycleDuration;
    if (lastGateDurationInCycles > 1)
        amountOfCycles += lastGateDurationInCycles - 1;

    // Cycles start at zero, so we add 1 to get the true amount of cycles.
    return amountOfCycles + 1; 
}

Int calculateAmountOfBits(const Vec<GateProperties> &gates, const Vec<Int> GateProperties::* operandType) {
    QL_DOUT("Calculating amount of bits...");

    // Find the maximum index of the operands.
    Int maxAmount = 0;
    for (const GateProperties &gate : gates) {
        Vec<Int>::const_iterator begin = (gate.*operandType).begin();
        const Vec<Int>::const_iterator end = (gate.*operandType).end();

        for (; begin != end; ++begin) {
            const Int number = *begin;
            if (number > maxAmount) maxAmount = number;
        }
    }

    // If maxAmount is at its original value, the list of operands for all the gates
    // was empty. This means there are no operands of the given type for these gates
    // and we return 0.
    if (maxAmount == 0) {
        return 0;
    } else {
        return 1 + maxAmount;
    }
}

Int calculateAmountOfGateOperands(const GateProperties &gate) {
    return utoi(gate.operands.size() + gate.creg_operands.size());
}

Vec<GateOperand> getGateOperands(const GateProperties &gate) {
    Vec<GateOperand> operands;

    for (const Int operand : gate.operands)      { operands.push_back({QUANTUM, operand});   }
    for (const Int operand : gate.creg_operands) { operands.push_back({CLASSICAL, operand}); }

    return operands;
}

Pair<GateOperand, GateOperand> calculateEdgeOperands(const Vec<GateOperand> &operands, const Int amountOfQubits) {
    if (operands.size() < 2) {
        QL_FATAL("Gate operands vector does not have multiple operands!");
    }

    GateOperand minOperand = operands[0];
    GateOperand maxOperand = operands[operands.size() - 1];
    for (const GateOperand &operand : operands) {
        const Int row = (operand.bitType == QUANTUM) ? operand.index : operand.index + amountOfQubits;
        if (row < minOperand.index) minOperand = operand;
        if (row > maxOperand.index) maxOperand = operand;
    }

    return {minOperand, maxOperand};
}

void fixMeasurementOperands(Vec<GateProperties> &gates) {
    QL_DOUT("Fixing measurement gates with no classical operand...");

    for (GateProperties &gate : gates) {
        // Check for a measurement gate without explicitly specified classical
        // operand.
        if (isMeasurement(gate)) {
            if (calculateAmountOfGateOperands(gate) == 1) {
                // Set classical measurement operand to the bit corresponding to
                // the measurements qubit index.
                QL_DOUT("Found measurement gate with no classical operand. Assuming default classical operand.");
                const Int cbit = gate.operands[0];
                gate.creg_operands.push_back(cbit);
            }
        }
    }
}

Bool isMeasurement(const GateProperties &gate) {
    //TODO: this method of checking for measurements is not robust and relies
    //      entirely on the user naming their instructions in a certain way!
    return (gate.name.find("measure") != Str::npos);
}

void printGates(const Vec<GateProperties> &gates) {
    for (const GateProperties &gate : gates) {
        QL_IOUT(gate.name);

        Str operands = "[";
        for (UInt i = 0; i < gate.operands.size(); i++) {
            operands += to_string(gate.operands[i]);
            if (i != gate.operands.size() - 1) operands += ", ";
        }
        QL_IOUT("\toperands: " << operands << "]");

        Str creg_operands = "[";
        for (UInt i = 0; i < gate.creg_operands.size(); i++) {
            creg_operands += to_string(gate.creg_operands[i]);
            if (i != gate.creg_operands.size() - 1) creg_operands += ", ";
        }
        QL_IOUT("\tcreg_operands: " << creg_operands << "]");

        QL_IOUT("\tduration: " << gate.duration);
        QL_IOUT("\tcycle: " << gate.cycle);
        QL_IOUT("\ttype: " << gate.type);

        Str codewords = "[";
        for (UInt i = 0; i < gate.codewords.size(); i++) {
            codewords += to_string(gate.codewords[i]);
            if (i != gate.codewords.size() - 1) codewords += ", ";
        }
        QL_IOUT("\tcodewords: " << codewords << "]");

        QL_IOUT("\tvisual_type: " << gate.visual_type);
    }
}

void printGatesShort(const Vec<GateProperties> &gates) {
    UInt maxGateNameLength = 0;
    UInt maxSwapStringLength = 0;
    UInt maxCycleStringLength = 0;
    UInt maxRealOperandsLength = 0;
    for (const GateProperties &gate : gates) {
        if (gate.name.length() > maxGateNameLength) {
            maxGateNameLength = gate.name.length();
        }
        if (to_string(gate.swap_params.part_of_swap).length() > maxSwapStringLength) {
            maxSwapStringLength = to_string(gate.swap_params.part_of_swap).length();
        }
        if (to_string(gate.cycle).length() > maxCycleStringLength) {
            maxCycleStringLength = to_string(gate.cycle).length();
        }
        Str rOperands = "[ "; for (const Int operand : gate.operands) {rOperands += std::to_string(operand) + " ";} rOperands += "]";
        if (rOperands.length() > maxRealOperandsLength) {
            maxRealOperandsLength = rOperands.length();
        }
    }
    const Int minSpacing = 3;
    for (const GateProperties &gate : gates) {
        Str rOperands = "[ "; for (const Int operand : gate.operands) {rOperands += std::to_string(operand) + " ";} rOperands += "]";
        Str vOperands = "[" + to_string(gate.swap_params.v0) + ", " + to_string(gate.swap_params.v1) + "]";

        Str nameSectionExtraSpacing;
        for (UInt i = 0; i < maxGateNameLength - gate.name.length() + minSpacing; i++) {
            nameSectionExtraSpacing += " ";
        }
        const Str nameSection = "gate: " + gate.name + nameSectionExtraSpacing;

        Str swapSectionExtraSpacing;
        for (UInt i = 0; i < maxSwapStringLength - to_string(gate.swap_params.part_of_swap).length() + minSpacing; i++) {
            swapSectionExtraSpacing += " ";
        }
        const Str swapSection = "part of swap: " + to_string(gate.swap_params.part_of_swap) + swapSectionExtraSpacing;

        Str cycleSectionExtraSpacing;
        for (UInt i = 0; i < maxCycleStringLength - to_string(gate.cycle).length() + minSpacing; i++) {
            cycleSectionExtraSpacing += " ";
        }
        const Str cycleSection = "cycle: " + to_string(gate.cycle) + cycleSectionExtraSpacing;

        Str realOperandsSectionExtraSpacing;
        for (UInt i = 0; i < maxRealOperandsLength - rOperands.length() + 1; i++) {
            realOperandsSectionExtraSpacing += " ";
        }
        const Str realOperandsSection = "real and virtual operands: " + rOperands + realOperandsSectionExtraSpacing;

        QL_IOUT(nameSection << swapSection << cycleSection << realOperandsSection << " and " << vOperands);
    }
}

} // namespace detail
} // namespace visualize
} // namespace ana
} // namespace pass
} // namespace ql

#endif //WITH_VISUALIZER
