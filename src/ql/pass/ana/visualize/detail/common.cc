/** \file
 * Declaration of the visualizer's shared functionalities.
 */

#ifdef WITH_VISUALIZER

#include "common.h"

#include <iostream>
#include "ql/utils/json.h"
#include "ql/com/options.h"
#include "ql/ir/ir.h"

namespace ql {
namespace pass {
namespace ana {
namespace visualize {
namespace detail {

using namespace utils;

class GateCollector : public ir::RecursiveVisitor {
public:
    GateCollector(const ir::PlatformRef& platform, Vec<GateProperties>& gates) : ir::RecursiveVisitor(), platform(platform), gates(gates) {}

    void visit_node(ir::Node &node) override {}

    void visit_platform(ir::Platform &platform) override {}

    void visit_conditional_instruction(ir::ConditionalInstruction &cond_instr) override {
        if (!cond_instr.condition->as_bit_literal() || !cond_instr.condition->as_bit_literal()->value) {
            QL_FATAL("Visualizer doesn't support conditional instructions");
            return;
        }

        ir::RecursiveVisitor::visit_conditional_instruction(cond_instr);
    }

    void visit_structured(ir::Structured &structured) override {
        QL_FATAL("Visualizer doesn't support structured blocks (loops, if/else)");
    }

    void visit_custom_instruction(ir::CustomInstruction &custom_instr) override {
        utils::Vec<utils::Int> qubits;
        utils::Vec<utils::Int> cregs;

        for (const auto& op: custom_instr.instruction_type->template_operands) {
            if (auto ref = op->as_reference()) {
                if (ref->target.operator==(platform->qubits)) {
                    qubits.push_back(ref->indices[0].as<ir::IntLiteral>()->value);
                }

                if (ref->target->name == "creg") {
                    cregs.push_back(ref->indices[0].as<ir::IntLiteral>()->value);
                }
            }
        }

        for (const auto& op: custom_instr.operands) {
            if (auto ref = op->as_reference()) {
                if (ref->target.operator==(platform->qubits)) {
                    qubits.push_back(ref->indices[0].as<ir::IntLiteral>()->value);
                }

                if (ref->target->name == "creg") {
                    cregs.push_back(ref->indices[0].as<ir::IntLiteral>()->value);
                }
            }
        }

        GateProperties gate {
            custom_instr.instruction_type->name,
            qubits,
            cregs,
            {},
            utoi(custom_instr.instruction_type->duration),
            custom_instr.cycle,
            {},
            "UNDEFINED"
        };
        QL_DOUT("Adding gate: " << custom_instr.instruction_type->name << " " << qubits << " " << cregs << " " << custom_instr.cycle);

        gates.push_back(gate); // FIXME: does this preserve order? Or do we need to sort somehow afterwards?
    }

private:
    const ir::PlatformRef& platform;
    Vec<GateProperties>& gates;
};

Vec<GateProperties> parseGates(const ir::Ref &ir) {
    Vec<GateProperties> gates;

    GateCollector gateCollector(ir->platform, gates);
    ir->visit(gateCollector);
    return gates;
}

Int calculateAmountOfCycles(const Vec<GateProperties> &gates) {
    QL_DOUT("Calculating amount of cycles...");

    // Find the highest cycle in the gate vector.
    Int amountOfCycles = 0;
    for (const GateProperties &gate : gates) {
        if (gate.cycle + gate.durationInCycles > amountOfCycles) {
            amountOfCycles = gate.cycle + gate.durationInCycles;
        }
    }

    return amountOfCycles; 
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

        QL_IOUT("\tduration in cycles: " << gate.durationInCycles);
        QL_IOUT("\tcycle: " << gate.cycle);

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
