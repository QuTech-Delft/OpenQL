#include "kernel.h"

#include <sstream>
#include <algorithm>
#include <iterator>


#define K_PI 3.141592653589793238462643383279502884197169399375105820974944592307816406L
#include "json.h"
#include "utils.h"
#include "options.h"
#include "gate.h"
#include "classical.h"
#include "ir.h"
#include "unitary.h"
#include "platform.h"

namespace ql {

// source: https://stackoverflow.com/questions/994593/how-to-do-an-integer-log2-in-c user Todd Lehman
static int uint64_log2(uint64_t n) {
#define S(k) if (n >= (UINT64_C(1) << k)) { i += k; n >>= k; }
    int i = -(n == 0); S(32); S(16); S(8); S(4); S(2); S(1); return i;
#undef S
}


quantum_kernel::quantum_kernel(const std::string &name) :
    name(name), iterations(1), type(kernel_type_t::STATIC)
{
}

quantum_kernel::quantum_kernel(
    const std::string &name,
    const ql::quantum_platform& platform,
    size_t qcount,
    size_t ccount
) :
    name(name),
    iterations(1),
    qubit_count(qcount),
    creg_count(ccount),
    type(kernel_type_t::STATIC)
{
    instruction_map = platform.instruction_map;
    cycle_time = platform.cycle_time;
    cycles_valid = true;
    // FIXME: check qubit_count and creg_count against platform
    // FIXME: what is the reason we can specify qubit_count and creg_count here anyway
}

void quantum_kernel::set_condition(const operation &oper) {
    if ((oper.operands[0])->id >= creg_count || (oper.operands[1]->id >= creg_count)) {
        EOUT("Out of range operand(s) for '" << oper.operation_name);
        throw ql::exception("Out of range operand(s) for '"+oper.operation_name+"' !",false);
    }

    if (oper.operation_type != ql::operation_type_t::RELATIONAL) {
        EOUT("Relational operator not used for conditional '" << oper.operation_name);
        throw ql::exception("Relational operator not used for conditional '"+oper.operation_name+"' !",false);
    }

    br_condition = oper;
}

void quantum_kernel::set_kernel_type(kernel_type_t typ) {
    type = typ;
}

std::string quantum_kernel::get_gates_definition() const {
    std::stringstream ss;

    for (auto i = instruction_map.begin(); i != instruction_map.end(); i++) {
        ss << i->first << std::endl;
    }
    return ss.str();
}

std::string quantum_kernel::get_name() const {
    return name;
}

circuit &quantum_kernel::get_circuit() {
    return c;
}

const circuit &quantum_kernel::get_circuit() const {
    return c;
}

void quantum_kernel::identity(size_t qubit) {
    gate("identity", qubit);
}

void quantum_kernel::i(size_t qubit) {
    gate("identity", qubit);
}

void quantum_kernel::hadamard(size_t qubit) {
    gate("hadamard", qubit);
}

void quantum_kernel::h(size_t qubit) {
    hadamard(qubit);
}

void quantum_kernel::rx(size_t qubit, double angle) {
    c.push_back(new ql::rx(qubit,angle));
    cycles_valid = false;
}

void quantum_kernel::ry(size_t qubit, double angle) {
    c.push_back(new ql::ry(qubit,angle));
    cycles_valid = false;
}

void quantum_kernel::rz(size_t qubit, double angle) {
    c.push_back(new ql::rz(qubit,angle));
    cycles_valid = false;
}

void quantum_kernel::s(size_t qubit) {
    gate("s", qubit);
}

void quantum_kernel::sdag(size_t qubit) {
    gate("sdag", qubit);
}

void quantum_kernel::t(size_t qubit) {
    gate("t", qubit);
}

void quantum_kernel::tdag(size_t qubit) {
    gate("tdag", qubit);
}

void quantum_kernel::x(size_t qubit) {
    gate("x", qubit);
}

void quantum_kernel::y(size_t qubit) {
    gate("y", qubit);
}

void quantum_kernel::z(size_t qubit) {
    gate("z", qubit);
}

void quantum_kernel::rx90(size_t qubit) {
    gate("rx90", qubit);
}

void quantum_kernel::mrx90(size_t qubit) {
    gate("mrx90", qubit);
}

void quantum_kernel::rx180(size_t qubit) {
    gate("rx180", qubit);
}

void quantum_kernel::ry90(size_t qubit) {
    gate("ry90", qubit);
}

void quantum_kernel::mry90(size_t qubit) {
    gate("mry90", qubit);
}

void quantum_kernel::ry180(size_t qubit) {
    gate("ry180", qubit);
}

void quantum_kernel::measure(size_t qubit) {
    gate("measure", qubit);
}

void quantum_kernel::prepz(size_t qubit) {
    gate("prepz", qubit);
}

void quantum_kernel::cnot(size_t qubit1, size_t qubit2) {
    gate("cnot", {qubit1, qubit2});
}

void quantum_kernel::cz(size_t qubit1, size_t qubit2) {
    gate("cz", {qubit1, qubit2});
}

void quantum_kernel::cphase(size_t qubit1, size_t qubit2) {
    gate("cphase", {qubit1, qubit2});
}

void quantum_kernel::toffoli(size_t qubit1, size_t qubit2, size_t qubit3) {
    // TODO add custom gate check if needed
    c.push_back(new ql::toffoli(qubit1, qubit2, qubit3));
    cycles_valid = false;
}

void quantum_kernel::swap(size_t qubit1, size_t qubit2) {
    gate("swap", {qubit1, qubit2});
}

void quantum_kernel::wait(const std::vector<size_t> &qubits, size_t duration) {
    gate("wait", qubits, {}, duration);
}

void quantum_kernel::display() {
    c.push_back(new ql::display());
    cycles_valid = false;
}

void quantum_kernel::clifford(int id, size_t qubit) {
    switch (id) {
        case 0 :
            break;              //  ['I']
        case 1 :
            ry90(qubit);
            rx90(qubit);
            break;              //  ['Y90', 'X90']
        case 2 :
            mrx90(qubit);
            mry90(qubit);
            break;              //  ['mX90', 'mY90']
        case 3 :
            rx180(qubit);
            break;              //  ['X180']
        case 4 :
            mry90(qubit);
            mrx90(qubit);
            break;              //  ['mY90', 'mX90']
        case 5 :
            rx90(qubit);
            mry90(qubit);
            break;              //  ['X90', 'mY90']
        case 6 :
            ry180(qubit);
            break;              //  ['Y180']
        case 7 :
            mry90(qubit);
            rx90(qubit);
            break;              //  ['mY90', 'X90']
        case 8 :
            rx90(qubit);
            ry90(qubit);
            break;              //  ['X90', 'Y90']
        case 9 :
            rx180(qubit);
            ry180(qubit);
            break;              //  ['X180', 'Y180']
        case 10:
            ry90(qubit);
            mrx90(qubit);
            break;              //  ['Y90', 'mX90']
        case 11:
            mrx90(qubit);
            ry90(qubit);
            break;              //  ['mX90', 'Y90']
        case 12:
            ry90(qubit);
            rx180(qubit);
            break;              //  ['Y90', 'X180']
        case 13:
            mrx90(qubit);
            break;              //  ['mX90']
        case 14:
            rx90(qubit);
            mry90(qubit);
            mrx90(qubit);
            break;              //  ['X90', 'mY90', 'mX90']
        case 15:
            mry90(qubit);
            break;              //  ['mY90']
        case 16:
            rx90(qubit);
            break;              //  ['X90']
        case 17:
            rx90(qubit);
            ry90(qubit);
            rx90(qubit);
            break;              //  ['X90', 'Y90', 'X90']
        case 18:
            mry90(qubit);
            rx180(qubit);
            break;              //  ['mY90', 'X180']
        case 19:
            rx90(qubit);
            ry180(qubit);
            break;              //  ['X90', 'Y180']
        case 20:
            rx90(qubit);
            mry90(qubit);
            rx90(qubit);
            break;              //  ['X90', 'mY90', 'X90']
        case 21:
            ry90(qubit);
            break;              //  ['Y90']
        case 22:
            mrx90(qubit);
            ry180(qubit);
            break;              //  ['mX90', 'Y180']
        case 23:
            rx90(qubit);
            ry90(qubit);
            mrx90(qubit);
            break;              //  ['X90', 'Y90', 'mX90']
        default:
            break;
    }
}

bool quantum_kernel::add_default_gate_if_available(
    const std::string &gname,
    const std::vector<size_t> &qubits,
    const std::vector<size_t> &cregs,
    size_t duration,
    double angle
) {
    bool result = false;

    bool is_one_qubit_gate = (gname == "identity") || (gname == "i")
                             || (gname == "hadamard") || (gname == "h")
                             || (gname == "pauli_x") || (gname == "pauli_y") || (gname == "pauli_z")
                             || (gname == "x") || (gname == "y") || (gname == "z")
                             || (gname == "s") || (gname == "sdag")
                             || (gname == "t") || (gname == "tdag")
                             || (gname == "rx") || (gname == "ry") || (gname == "rz")
                             || (gname == "rx90") || (gname == "mrx90") || (gname == "rx180")
                             || (gname == "ry90") || (gname == "mry90") || (gname == "ry180")
                             || (gname == "measure") || (gname == "prepz");

    bool is_two_qubit_gate = (gname == "cnot")
                             || (gname == "cz") || (gname == "cphase")
                             || (gname == "swap");

    bool is_multi_qubit_gate = (gname == "toffoli")
                               || (gname == "wait") || (gname == "barrier");

    if (is_one_qubit_gate) {
        if (qubits.size() != 1) {
            return false;
        }
    } else if (is_two_qubit_gate) {
        if (qubits.size() != 2) {
            return false;
        }
        if (qubits[0] == qubits[1]) {
            return false;
        }
    } else if (!is_multi_qubit_gate) {
        return false;
    }

    if (gname == "identity" || gname == "i") {
        c.push_back(new ql::identity(qubits[0]));
        result = true;
    } else if (gname == "hadamard" || gname == "h") {
        c.push_back(new ql::hadamard(qubits[0]));
        result = true;
    } else if (gname == "pauli_x" || gname == "x") {
        c.push_back(new ql::pauli_x(qubits[0]));
        result = true;
    } else if( gname == "pauli_y" || gname == "y") {
        c.push_back(new ql::pauli_y(qubits[0]));
        result = true;
    } else if (gname == "pauli_z" || gname == "z") {
        c.push_back(new ql::pauli_z(qubits[0]));
        result = true;
    } else if (gname == "s" || gname == "phase") {
        c.push_back(new ql::phase(qubits[0]));
        result = true;
    } else if (gname == "sdag" || gname == "phasedag") {
        c.push_back(new ql::phasedag(qubits[0]));
        result = true;
    } else if (gname == "t") {
        c.push_back(new ql::t(qubits[0]));
        result = true;
    } else if (gname == "tdag") {
        c.push_back(new ql::tdag(qubits[0]));
        result = true;
    } else if (gname == "rx") {
        c.push_back(new ql::rx(qubits[0], angle));
        result = true;
    } else if (gname == "ry") {
        c.push_back(new ql::ry(qubits[0], angle));
        result = true;
    } else if( gname == "rz") {
        c.push_back(new ql::rz(qubits[0], angle));
        result = true;
    } else if (gname == "rx90") {
        c.push_back(new ql::rx90(qubits[0]));
        result = true;
    } else if (gname == "mrx90") {
        c.push_back(new ql::mrx90(qubits[0]));
        result = true;
    } else if (gname == "rx180") {
        c.push_back(new ql::rx180(qubits[0]));
        result = true;
    } else if (gname == "ry90") {
        c.push_back(new ql::ry90(qubits[0]));
        result = true;
    } else if (gname == "mry90") {
        c.push_back(new ql::mry90(qubits[0]));
        result = true;
    } else if (gname == "ry180") {
        c.push_back(new ql::ry180(qubits[0]));
        result = true;
    } else if (gname == "measure") {
        if (cregs.empty()) {
            c.push_back(new ql::measure(qubits[0]));
        } else {
            c.push_back(new ql::measure(qubits[0], cregs[0]));
        }
        result = true;
    } else if (gname == "prepz") {
        c.push_back(new ql::prepz(qubits[0]));
        result = true;
    } else if (gname == "cnot") {
        c.push_back(new ql::cnot(qubits[0], qubits[1]));
        result = true;
    } else if (gname == "cz" || gname == "cphase") {
        c.push_back(new ql::cphase(qubits[0], qubits[1]) );
        result = true;
    } else if (gname == "toffoli") {
        c.push_back(new ql::toffoli(qubits[0], qubits[1], qubits[2]));
        result = true;
    } else if (gname == "swap") {
        c.push_back(new ql::swap(qubits[0], qubits[1]));
        result = true;
    } else if (gname == "barrier") {
        /*
        wait/barrier is applied on the qubits specified as arguments.
        if no qubits are specified, then wait/barrier is applied on all qubits
        */
        if (qubits.empty()) {
            std::vector<size_t> all_qubits;
            for (size_t q = 0; q < qubit_count; q++) {
                all_qubits.push_back(q);
            }
            c.push_back(new ql::wait(all_qubits, 0, 0));
        } else {
            c.push_back(new ql::wait(qubits, 0, 0));
        }
        result = true;
    } else if (gname == "wait") {
        /*
        wait/barrier is applied on the qubits specified as arguments.
        if no qubits are specified, then wait/barrier is applied on all qubits
        */
        size_t duration_in_cycles = std::ceil(static_cast<float>(duration) / cycle_time);
        if (qubits.empty()) {
            std::vector<size_t> all_qubits;
            for (size_t q = 0; q < qubit_count; q++) {
                all_qubits.push_back(q);
            }
            c.push_back(new ql::wait(all_qubits, duration, duration_in_cycles));
        } else {
            c.push_back(new ql::wait(qubits, duration, duration_in_cycles));
        }
        result = true;
    } else {
        result = false;
    }

    if (result) {
        cycles_valid = false;
    }

    return result;
}

// if a specialized custom gate ("e.g. cz q0,q4") is available, add it to circuit and return true
// if a parameterized custom gate ("e.g. cz") is available, add it to circuit and return true
//
// note that there is no check for the found gate being a composite gate
bool quantum_kernel::add_custom_gate_if_available(
    const std::string &gname,
    const std::vector<size_t> &qubits,
    const std::vector<size_t> &cregs,
    size_t duration,
    double angle
) {
#if OPT_DECOMPOSE_WAIT_BARRIER  // hack to skip wait/barrier
    if (gname=="wait" || gname=="barrier") {
        return false;   // return, so a default gate will be attempted
    }
#endif
    // construct canonical name
    std::string instr = "";
    for (auto qubit : qubits) {
        if (!instr.empty()) {
            instr += ",";
        }
        instr += "q" + std::to_string(qubit);
    }
    instr = gname + " " + instr;

    // first check if a specialized custom gate is available
    // a specialized custom gate is of the form: "cz q0 q3"
    instruction_map_t::iterator it = instruction_map.find(instr);
    if (it == instruction_map.end()) {
        it = instruction_map.find(gname);
    }
    if (it == instruction_map.end()) {
        DOUT("custom gate not added for " << gname);
        return false;
    }

    custom_gate *g = new custom_gate(*(it->second));
    for (auto qubit : qubits) {
        g->operands.push_back(qubit);
    }
    for (auto &cop : cregs) {
        g->creg_operands.push_back(cop);
    }
    if (duration > 0) {
        g->duration = duration;
    }
    g->angle = angle;
    c.push_back(g);

    DOUT("custom gate added for " << gname);
    cycles_valid = false;
    return true;
}

// FIXME: move to class composite_gate?
// return the subinstructions of a composite gate
// while doing, test whether the subinstructions have a definition (so they cannot be specialized or default ones!)
void quantum_kernel::get_decomposed_ins(
    const ql::composite_gate *gptr,
    std::vector<std::string> &sub_instructions
) const {
    auto &sub_gates = gptr->gs;
    DOUT("composite ins: " << gptr->name);
    for (auto &agate : sub_gates) {
        std::string &sub_ins = agate->name;
        DOUT("  sub ins: " << sub_ins);
        auto it = instruction_map.find(sub_ins);
        if (it != instruction_map.end()) {
            sub_instructions.push_back(sub_ins);
        } else {
            throw ql::exception("[x] error : ql::kernel::gate() : gate decomposition not available for '" + sub_ins + "'' in the target platform !", false);
        }
    }
}

// if specialized composed gate: "e.g. cz q0,q3" available, with composition of subinstructions, return true
//      also check each subinstruction for presence of a custom_gate (or a default gate)
// otherwise, return false
// don't add anything to circuit
//
// add specialized decomposed gate, example JSON definition: "cl_14 q1": ["rx90 %0", "rym90 %0", "rxm90 %0"]
bool quantum_kernel::add_spec_decomposed_gate_if_available(
    const std::string &gate_name,
    const std::vector<size_t> &all_qubits,
    const std::vector<size_t> &cregs
) {
    bool added = false;
    DOUT("Checking if specialized decomposition is available for " << gate_name);

    // construct canonical name
    std::string instr_parameterized = "";
    for (auto qubit : all_qubits) {
        if (!instr_parameterized.empty()) {
            instr_parameterized += ",";
        }
        instr_parameterized += "q" + std::to_string(qubit);
    }
    instr_parameterized = gate_name + " " + instr_parameterized;
    DOUT("specialized instruction name: " << instr_parameterized);

    // find the name
    auto it = instruction_map.find(instr_parameterized);
    if (it != instruction_map.end()) {
        // check gate type
        DOUT("specialized composite gate found for " << instr_parameterized);
        composite_gate * gptr = (composite_gate *)(it->second);
        if (gptr->type() == __composite_gate__) {
            DOUT("composite gate type");
        } else {
            DOUT("not a composite gate type");
            return false;
        }

        // perform decomposition
        std::vector<std::string> sub_instructions;
        get_decomposed_ins(gptr, sub_instructions);
        for (auto &sub_ins : sub_instructions) {
            // extract name and qubits
            DOUT("Adding sub ins: " << sub_ins);
            std::replace(sub_ins.begin(), sub_ins.end(), ',', ' ');    // FIXME: perform all conversions in sanitize_instruction_name()
            DOUT(" after comma removal, sub ins: " << sub_ins);
            std::istringstream iss(sub_ins);

            std::vector<std::string> tokens{
                std::istream_iterator<std::string>{iss},
                std::istream_iterator<std::string>{}
            };

            std::vector<size_t> this_gate_qubits;
            std::string & sub_ins_name = tokens[0];

            for (size_t i = 1; i < tokens.size(); i++) {
                DOUT("tokens[i] : " << tokens[i]);
                auto sub_str_token = tokens[i].substr(1);
                DOUT("sub_str_token[i] : " << sub_str_token);
                this_gate_qubits.push_back(stoi(tokens[i].substr(1)));
            }

            DOUT( ql::utils::to_string<size_t>(this_gate_qubits, "actual qubits of this gate:"));

            // custom gate check
            // when found, custom_added is true, and the expanded subinstruction was added to the circuit
            bool custom_added = add_custom_gate_if_available(sub_ins_name, this_gate_qubits, cregs);
            if (!custom_added) {
                if(ql::options::get("use_default_gates") == "yes") {
                    // default gate check
                    DOUT("adding default gate for " << sub_ins_name);
                    bool default_available = add_default_gate_if_available(sub_ins_name, this_gate_qubits, cregs);
                    if (default_available) {
                        WOUT("added default gate '" << sub_ins_name << "' with " << ql::utils::to_string(this_gate_qubits,"qubits") );
                    } else {
                        EOUT("unknown gate '" << sub_ins_name << "' with " << ql::utils::to_string(this_gate_qubits,"qubits") );
                        throw ql::exception("[x] error : ql::kernel::gate() : the gate '"+sub_ins_name+"' with " +ql::utils::to_string(this_gate_qubits,"qubits")+" is not supported by the target platform !",false);
                    }
                } else {
                    EOUT("unknown gate '" << sub_ins_name << "' with " << ql::utils::to_string(this_gate_qubits,"qubits") );
                    throw ql::exception("[x] error : ql::kernel::gate() : the gate '"+sub_ins_name+"' with " +ql::utils::to_string(this_gate_qubits,"qubits")+" is not supported by the target platform !",false);
                }
            }
        }
        added = true;
    } else {
        DOUT("composite gate not found for " << instr_parameterized);
    }

    return added;
}

// if composite gate: "e.g. cz %0 %1" available, return true;
//      also check each subinstruction for availability as a custom gate (or default gate)
// if not, return false
// don't add anything to circuit
//
// add parameterized decomposed gate, example JSON definition: "cl_14 %0": ["rx90 %0", "rym90 %0", "rxm90 %0"]
bool quantum_kernel::add_param_decomposed_gate_if_available(
    const std::string &gate_name,
    const std::vector<size_t> &all_qubits,
    const std::vector<size_t> &cregs
) {
    bool added = false;
    DOUT("Checking if parameterized composite gate is available for " << gate_name);

    // construct instruction name from gate_name and actual qubit parameters
    std::string instr_parameterized = "";
    for (size_t i = 0; i < all_qubits.size(); i++) {
        if (!instr_parameterized.empty()) {
            instr_parameterized += ",";
        }
        instr_parameterized += "%" + std::to_string(i);
    }
    instr_parameterized = gate_name + " " + instr_parameterized;
    DOUT("parameterized instruction name: " << instr_parameterized);

    // check for composite ins
    auto it = instruction_map.find(instr_parameterized);
    if (it != instruction_map.end()) {
        DOUT("parameterized gate found for " << instr_parameterized);
        composite_gate * gptr = (composite_gate *)(it->second);
        if (gptr->type() == __composite_gate__) {
            DOUT("composite gate type");
        } else {
            DOUT("Not a composite gate type");
            return false;
        }

        std::vector<std::string> sub_instructions;
        get_decomposed_ins(gptr, sub_instructions);
        for (auto &sub_ins : sub_instructions) {
            DOUT("Adding sub ins: " << sub_ins);
            std::replace( sub_ins.begin(), sub_ins.end(), ',', ' ');
            DOUT(" after comma removal, sub ins: " << sub_ins);

            // tokenize sub_ins into sub_ins_name and this_gate_qubits
            // FIXME: similar code in add_spec_decomposed_gate_if_available()
            std::istringstream iss(sub_ins);
            std::vector<std::string> tokens{ std::istream_iterator<std::string>{iss},
                                             std::istream_iterator<std::string>{} };

            std::vector<size_t> this_gate_qubits;
            std::string & sub_ins_name = tokens[0];

            for (size_t i = 1; i < tokens.size(); i++) {
                auto sub_str_token = tokens[i].substr(1);   // example: tokens[i] equals "%1" -> sub_str_token equals "1"
                size_t qubit_idx = stoi(sub_str_token);
                if (qubit_idx >= all_qubits.size()) {
                    FATAL("Illegal qubit parameter index " << sub_str_token
                                                           << " exceeds actual number of parameters given (" << all_qubits.size()
                                                           << ") while adding sub ins '" << sub_ins
                                                           << "' in parameterized instruction '" << instr_parameterized << "'");
                }
                this_gate_qubits.push_back(all_qubits[qubit_idx]);
            }
            DOUT( ql::utils::to_string<size_t>(this_gate_qubits, "actual qubits of this gate:") );

            // FIXME: following code block exists several times in this file
            // custom gate check
            // when found, custom_added is true, and the expanded subinstruction was added to the circuit
            bool custom_added = add_custom_gate_if_available(sub_ins_name, this_gate_qubits, cregs);
            if (!custom_added)
            {
                if (ql::options::get("use_default_gates") == "yes") {
                    // default gate check
                    DOUT("adding default gate for " << sub_ins_name);
                    bool default_available = add_default_gate_if_available(sub_ins_name, this_gate_qubits, cregs);
                    if (default_available) {
                        WOUT("added default gate '" << sub_ins_name << "' with " << ql::utils::to_string(this_gate_qubits,"qubits") );
                    } else {
                        EOUT("unknown gate '" << sub_ins_name << "' with " << ql::utils::to_string(this_gate_qubits,"qubits") );
                        throw ql::exception("[x] error : ql::kernel::gate() : the gate '"+sub_ins_name+"' with " +ql::utils::to_string(this_gate_qubits,"qubits")+" is not supported by the target platform !",false);
                    }
                } else {
                    EOUT("unknown gate '" << sub_ins_name << "' with " << ql::utils::to_string(this_gate_qubits,"qubits") );
                    throw ql::exception("[x] error : ql::kernel::gate() : the gate '"+sub_ins_name+"' with " +ql::utils::to_string(this_gate_qubits,"qubits")+" is not supported by the target platform !",false);
                }
            }
        }
        added = true;
    } else {
        DOUT("composite gate not found for " << instr_parameterized);
    }
    return added;
}

void quantum_kernel::gate(const std::string &gname, size_t q0) {
    gate(gname, std::vector<size_t> {q0});
}

void quantum_kernel::gate(const std::string &gname, size_t q0, size_t q1) {
    gate(gname, std::vector<size_t> {q0, q1});
}

/**
 * custom gate with arbitrary number of operands
 * check qubit/creg indices against platform parameters; fail fatally if an index is out of range
 * find matching gate in kernel's and platform's gate_definition; when no match, fail
 * return the gate (or its decomposition) by appending it to kernel.c, the current kernel's circuit
 */
void quantum_kernel::gate(
    const std::string &gname,
    const std::vector<size_t> &qubits,
    const std::vector<size_t> &cregs,
    size_t duration,
    double angle
) {
    /// @todo-rn: move these check to a platform-specific backend after qubits are initialized
    for (auto &qno : qubits) {
        if (qno >= qubit_count) {
            FATAL("Number of qubits in platform: " << std::to_string(qubit_count) << ", specified qubit numbers out of range for gate: '" << gname << "' with " << ql::utils::to_string(qubits,"qubits") );
        }
    }

    for (auto & cno : cregs) {
        if (cno >= creg_count) {
            FATAL("Out of range operand(s) for '" << gname << "' with " << ql::utils::to_string(cregs,"cregs") );
        }
    }

    if (!gate_nonfatal(gname, qubits, cregs, duration, angle)) {
        FATAL("Unknown gate '" << gname << "' with " << ql::utils::to_string(qubits,"qubits") );
    }
}

/**
 * custom gate with arbitrary number of operands
 * as gate above but return whether gate was successfully matched in gate_definition, next to gate in kernel.c
 */
bool quantum_kernel::gate_nonfatal(
    const std::string &gname,
    const std::vector<size_t> &qubits,
    const std::vector<size_t> &cregs,
    size_t duration,
    double angle
) {
    bool added = false;
    // check if specialized composite gate is available
    // if not, check if parameterized composite gate is available
    // if not, check if a specialized custom gate is available
    // if not, check if a parameterized custom gate is available
    // if not, check if a default gate is available
    // if not, then error

    auto gname_lower = utils::to_lower(gname);
    DOUT("Adding gate : " << gname_lower << " with " << ql::utils::to_string(qubits, "qubits"));

    // specialized composite gate check
    DOUT("trying to add specialized composite gate for: " << gname_lower);
    bool spec_decom_added = add_spec_decomposed_gate_if_available(gname_lower, qubits);
    if (spec_decom_added) {
        added = true;
        DOUT("specialized decomposed gates added for " << gname_lower);
    } else {
        // parameterized composite gate check
        DOUT("trying to add parameterized composite gate for: " << gname_lower);
        bool param_decom_added = add_param_decomposed_gate_if_available(gname_lower, qubits);
        if (param_decom_added) {
            added = true;
            DOUT("decomposed gates added for " << gname_lower);
        } else {
            // specialized/parameterized custom gate check
            DOUT("adding custom gate for " << gname_lower);
            // when found, custom_added is true, and the gate was added to the circuit
            bool custom_added = add_custom_gate_if_available(gname_lower, qubits, cregs, duration, angle);
            if (custom_added) {
                added = true;
                DOUT("custom gate added for " << gname_lower);
            } else {
                if (ql::options::get("use_default_gates") == "yes") {
                    // default gate check (which is always parameterized)
                    DOUT("adding default gate for " << gname_lower);

                    bool default_available = add_default_gate_if_available(gname_lower, qubits, cregs, duration);
                    if (default_available) {
                        added = true;
                        DOUT("default gate added for " << gname_lower);   // FIXME: used to be WOUT, but that gives a warning for every "wait" and spams the log
                    }
                }
            }
        }
    }
    if (added) {
        cycles_valid = false;
    }
    return added;
}

// to add unitary to kernel
void quantum_kernel::gate(
    const ql::unitary &u,
    const std::vector<size_t> &qubits
) {
    double u_size = uint64_log2((int)u.size())/2;
    if (u_size != qubits.size()) {
        EOUT("Unitary " << u.name <<" has been applied to the wrong number of qubits! " << qubits.size() << " and not " << u_size);
        throw ql::exception("Unitary '"+u.name+"' has been applied to the wrong number of qubits. Cannot be added to kernel! "  + std::to_string(qubits.size()) +" and not "+ std::to_string(u_size), false);
    }
    for (uint64_t i = 0; i < qubits.size()-1; i++) {
        for (uint64_t j = i + 1; j < qubits.size(); j++) {
            if (qubits[i] == qubits[j]) {
                EOUT("Qubit numbers used more than once in Unitary: " << u.name << ". Double qubit is number " << qubits[j]);
                throw ql::exception("Qubit numbers used more than once in Unitary: " + u.name + ". Double qubit is number " + std::to_string(qubits[j]), false);
            }
        }
    }
    // applying unitary to gates
    COUT("Applying unitary '" << u.name << "' to " << ql::utils::to_string(qubits, "qubits: ") );
    if (u.is_decomposed) {
        DOUT("Adding decomposed unitary to kernel ...");
        IOUT("The list is this many items long: " << u.instructionlist.size());
        //COUT("Instructionlist" << ql::utils::to_string(u.instructionlist));
        cycles_valid = false;
        int end_index = recursiveRelationsForUnitaryDecomposition(u,qubits, u_size, 0);
        DOUT("Total number of gates added: " << end_index);
    } else {
        EOUT("Unitary " << u.name <<" not decomposed. Cannot be added to kernel!");
        throw ql::exception("Unitary '"+u.name+"' not decomposed. Cannot be added to kernel!", false);
    }
}

//recursive gate count function
//n is number of qubits
//i is the start point for the instructionlist
int quantum_kernel::recursiveRelationsForUnitaryDecomposition(
    const ql::unitary &u,
    const std::vector<size_t> &qubits,
    int n,
    int i
) {
    // DOUT("Adding a new unitary starting at index: "<< i << ", to " << n << ql::utils::to_string(qubits, " qubits: "));
    if (n > 1) {
        // Need to be checked here because it changes the structure of the decomposition.
        // This checks whether the first qubit is affected, if not, it applies a unitary to the all qubits except the first one.
        int numberforcontrolledrotation = std::pow(2, n - 1);                     //number of gates per rotation

        // code for last one not affected
        if (u.instructionlist[i] == 100.0) {
            DOUT("[kernel.h] Optimization: last qubit is not affected, skip one step in the recursion. New start_index: " << i+1);
            std::vector<size_t> subvector(qubits.begin() + 1, qubits.end());
            return recursiveRelationsForUnitaryDecomposition(u, subvector, n - 1, i + 1) + 1; // for the number 10.0
        } else if (u.instructionlist[i] == 200.0) {
            std::vector<size_t> subvector(qubits.begin(), qubits.end() - 1);

            // This is a special case of only demultiplexing
            if (u.instructionlist[i+1] == 300.0) {

                // Two numbers that aren't rotation gate angles
                int start_counter = i + 2;
                DOUT("[kernel.h] Optimization: first qubit not affected, skip one step in the recursion. New start_index: " << start_counter);

                return recursiveRelationsForUnitaryDecomposition(u, subvector, n - 1, start_counter) + 2; //for the numbers 20 and 30
            } else {
                int start_counter = i + 1;
                DOUT("[kernel.h] Optimization: only demultiplexing will be performed. New start_index: " << start_counter);

                start_counter += recursiveRelationsForUnitaryDecomposition(u, subvector, n - 1, start_counter);
                multicontrolled_rz(u.instructionlist, start_counter, start_counter + numberforcontrolledrotation - 1, qubits);
                start_counter += numberforcontrolledrotation; //multicontrolled rotation always has the same number of gates
                start_counter += recursiveRelationsForUnitaryDecomposition(u, subvector, n - 1, start_counter);
                return start_counter - i;
            }
        } else {
            // The new qubit vector that is passed to the recursive function
            std::vector<size_t> subvector(qubits.begin(), qubits.end() - 1);
            int start_counter = i;
            start_counter += recursiveRelationsForUnitaryDecomposition(u, subvector, n - 1, start_counter);
            multicontrolled_rz(u.instructionlist, start_counter, start_counter + numberforcontrolledrotation - 1, qubits);
            start_counter += numberforcontrolledrotation;
            start_counter += recursiveRelationsForUnitaryDecomposition(u, subvector, n - 1, start_counter);
            multicontrolled_ry(u.instructionlist, start_counter, start_counter + numberforcontrolledrotation - 1, qubits);
            start_counter += numberforcontrolledrotation;
            start_counter += recursiveRelationsForUnitaryDecomposition(u, subvector, n - 1, start_counter);
            multicontrolled_rz(u.instructionlist, start_counter, start_counter + numberforcontrolledrotation - 1, qubits);
            start_counter += numberforcontrolledrotation;
            start_counter += recursiveRelationsForUnitaryDecomposition(u, subvector, n - 1, start_counter);
            return start_counter -i; //it is just the total
        }
    } else { //n=1
        // DOUT("Adding the zyz decomposition gates at index: "<< i);
        // zyz gates happen on the only qubit in the list.
        c.push_back(new ql::rz(qubits.back(), u.instructionlist[i]));
        c.push_back(new ql::ry(qubits.back(), u.instructionlist[i + 1]));
        c.push_back(new ql::rz(qubits.back(), u.instructionlist[i + 2]));
        // How many gates this took
        return 3;
    }
}

//controlled qubit is the first in the list.
void quantum_kernel::multicontrolled_rz(
    const std::vector<double> &instruction_list,
    int start_index,
    int end_index,
    const std::vector<size_t> &qubits
) {
    // DOUT("Adding a multicontrolled rz-gate at start index " << start_index << ", to " << ql::utils::to_string(qubits, "qubits: "));
    int idx;
    //The first one is always controlled from the last to the first qubit.
    c.push_back(new ql::rz(qubits.back(),-instruction_list[start_index]));
    c.push_back(new ql::cnot(qubits[0], qubits.back()));
    for (int i = 1; i < end_index - start_index; i++) {
        idx = uint64_log2(((i)^((i)>>1))^((i+1)^((i+1)>>1)));
        c.push_back(new ql::rz(qubits.back(),-instruction_list[i+start_index]));
        c.push_back(new ql::cnot(qubits[idx], qubits.back()));
    }
    // The last one is always controlled from the next qubit to the first qubit
    c.push_back(new ql::rz(qubits.back(),-instruction_list[end_index]));
    c.push_back(new ql::cnot(qubits.end()[-2], qubits.back()));
    cycles_valid = false;
}

//controlled qubit is the first in the list.
void quantum_kernel::multicontrolled_ry(
    const std::vector<double> &instruction_list,
    int start_index,
    int end_index,
    const std::vector<size_t> &qubits
) {
    // DOUT("Adding a multicontrolled ry-gate at start index "<< start_index << ", to " << ql::utils::to_string(qubits, "qubits: "));
    int idx;

    //The first one is always controlled from the last to the first qubit.
    c.push_back(new ql::ry(qubits.back(),-instruction_list[start_index]));
    c.push_back(new ql::cnot(qubits[0], qubits.back()));

    for (int i = 1; i < end_index - start_index; i++) {
        idx = uint64_log2 (((i)^((i)>>1))^((i+1)^((i+1)>>1)));
        c.push_back(new ql::ry(qubits.back(),-instruction_list[i+start_index]));
        c.push_back(new ql::cnot(qubits[idx], qubits.back()));
    }
    // Last one is controlled from the next qubit to the first one.
    c.push_back(new ql::ry(qubits.back(),-instruction_list[end_index]));
    c.push_back(new ql::cnot(qubits.end()[-2], qubits.back()));
    cycles_valid = false;
}

/**
 * qasm output
 */
// FIXME: create a separate QASM backend?
std::string quantum_kernel::get_prologue() const  {
    std::stringstream ss;
    ss << "\n";
    ss << "." << name << "\n";
    // ss << name << ":\n";

    if (type == kernel_type_t::IF_START) {
        ss << "    b" << br_condition.inv_operation_name <<" r" << (br_condition.operands[0])->id
           <<", r" << (br_condition.operands[1])->id << ", " << name << "_end\n";
    }

    if (type == kernel_type_t::ELSE_START) {
        ss << "    b" << br_condition.operation_name <<" r" << (br_condition.operands[0])->id
           <<", r" << (br_condition.operands[1])->id << ", " << name << "_end\n";
    }

    if (type == kernel_type_t::FOR_START) {
        // TODO for now r29, r30, r31 are used, fix it
        ss << "    ldi r29" <<", " << iterations << "\n";
        ss << "    ldi r30" <<", " << 1 << "\n";
        ss << "    ldi r31" <<", " << 0 << "\n";
    }

    return ss.str();
}

std::string quantum_kernel::get_epilogue() const {
    std::stringstream ss;

    if (type == kernel_type_t::DO_WHILE_END) {
        ss << "    b" << br_condition.operation_name <<" r" << (br_condition.operands[0])->id
           <<", r" << (br_condition.operands[1])->id << ", " << name << "_start\n";
    }

    if (type == kernel_type_t::FOR_END) {
        std::string kname(name);
        std::replace( kname.begin(), kname.end(), '_', ' ');
        std::istringstream iss(kname);
        std::vector<std::string> tokens{ std::istream_iterator<std::string>{iss},
                                         std::istream_iterator<std::string>{} };

        // TODO for now r29, r30, r31 are used, fix it
        ss << "    add r31, r31, r30\n";
        ss << "    blt r31, r29, " << tokens[0] << "\n";
    }

    return ss.str();
}

std::string quantum_kernel::qasm() const {
    std::stringstream ss;

    ss << get_prologue();

    for (size_t i = 0; i < c.size(); ++i) {
        ss << "    " << c[i]->qasm() << "\n";
    }

    ss << get_epilogue();

    return  ss.str();
}

/**
 * classical gate
 */
void quantum_kernel::classical(const creg &destination, const operation &oper) {
    // check sanity of destination
    if (destination.id >= creg_count) {
        EOUT("Out of range operand(s) for '" << oper.operation_name);
        throw ql::exception("Out of range operand(s) for '"+oper.operation_name+"' !",false);
    }

    // check sanity of other operands
    for (auto &op : oper.operands) {
        if (op->type() == operand_type_t::CREG) {
            if (op->id >= creg_count) {
                EOUT("Out of range operand(s) for '" << oper.operation_name);
                throw ql::exception("Out of range operand(s) for '"+oper.operation_name+"' !",false);
            }
        }
    }

    c.push_back(new ql::classical(destination, oper));
    cycles_valid = false;
}

void quantum_kernel::classical(const std::string &operation) {
    c.push_back(new ql::classical(operation));
    cycles_valid = false;
}

void quantum_kernel::controlled_x(size_t tq, size_t cq) {
    // from: https://arxiv.org/pdf/1206.0758v3.pdf
    // A meet-in-the-middle algorithm for fast synthesis
    // of depth-optimal quantum circuits
    cnot(cq, tq);
}

void quantum_kernel::controlled_y(size_t tq, size_t cq) {
    // from: https://arxiv.org/pdf/1206.0758v3.pdf
    // A meet-in-the-middle algorithm for fast synthesis
    // of depth-optimal quantum circuits
    sdag(tq);
    cnot(cq, tq);
    s(tq);
}

void quantum_kernel::controlled_z(size_t tq, size_t cq) {
    // from: https://arxiv.org/pdf/1206.0758v3.pdf
    // A meet-in-the-middle algorithm for fast synthesis
    // of depth-optimal quantum circuits
    hadamard(tq);
    cnot(cq, tq);
    hadamard(tq);
}

void quantum_kernel::controlled_h(size_t tq, size_t cq) {
    // from: https://arxiv.org/pdf/1206.0758v3.pdf
    // A meet-in-the-middle algorithm for fast synthesis
    // of depth-optimal quantum circuits
    s(tq);
    hadamard(tq);
    t(tq);
    cnot(cq, tq);
    tdag(tq);
    hadamard(tq);
    sdag(tq);
}

void quantum_kernel::controlled_i(size_t tq, size_t cq) {
    // well, basically you dont need to do anything for it :‑)
}

void quantum_kernel::controlled_s(size_t tq, size_t cq) {
    // cphase(cq, tq);

    // from: https://arxiv.org/pdf/1206.0758v3.pdf
    // A meet-in-the-middle algorithm for fast synthesis
    // of depth-optimal quantum circuits

    cnot(tq, cq);
    tdag(cq);
    cnot(tq, cq);
    t(cq);
    t(tq);
}

void quantum_kernel::controlled_sdag(size_t tq, size_t cq) {
    // based on: https://arxiv.org/pdf/1206.0758v3.pdf
    // A meet-in-the-middle algorithm for fast synthesis
    // of depth-optimal quantum circuits

    tdag(cq);
    tdag(tq);
    cnot(tq, cq);
    t(cq);
    cnot(tq, cq);
}

void quantum_kernel::controlled_t(size_t tq, size_t cq, size_t aq) {
    WOUT("Controlled-T implementation requires an ancilla");
    WOUT("At the moment, Qubit 0 is used as ancilla");
    WOUT("This will change when Qubit allocater is implemented");
    // from: https://arxiv.org/pdf/1206.0758v3.pdf
    // A meet-in-the-middle algorithm for fast synthesis
    // of depth-optimal quantum circuits
    cnot(cq, tq);
    hadamard(aq);
    sdag(cq);
    cnot(tq, aq);
    cnot(aq, cq);
    t(cq);
    tdag(aq);
    cnot(tq, cq);
    cnot(tq, aq);
    t(cq);
    tdag(aq);
    cnot(aq, cq);
    h(cq);
    t(cq);
    h(cq);
    cnot(aq, cq);
    tdag(cq);
    t(aq);
    cnot(tq, aq);
    cnot(tq, cq);
    t(aq);
    tdag(cq);
    cnot(aq, cq);
    s(cq);
    cnot(tq, aq);
    cnot(cq, tq);
    h(aq);
}

void quantum_kernel::controlled_tdag(size_t tq, size_t cq, size_t aq) {
    WOUT("Controlled-Tdag implementation requires an ancilla");
    WOUT("At the moment, Qubit 0 is used as ancilla");
    WOUT("This will change when Qubit allocater is implemented");
    // from: https://arxiv.org/pdf/1206.0758v3.pdf
    // A meet-in-the-middle algorithm for fast synthesis
    // of depth-optimal quantum circuits
    h(aq);
    cnot(cq, tq);
    sdag(cq);
    cnot(tq, aq);
    cnot(aq, cq);
    t(cq);
    cnot(tq, cq);
    tdag(aq);
    cnot(tq, aq);
    t(cq);
    tdag(aq);
    cnot(aq, cq);
    h(cq);
    tdag(cq);
    h(cq);
    cnot(aq, cq);
    tdag(cq);
    t(aq);
    cnot(tq, aq);
    cnot(tq, cq);
    tdag(cq);
    t(aq);
    cnot(aq, cq);
    s(cq);
    cnot(tq, aq);
    cnot(cq, tq);
    hadamard(aq);
}

void quantum_kernel::controlled_ix(size_t tq, size_t cq) {
    // from: https://arxiv.org/pdf/1210.0974.pdf
    // Quantum circuits of T-depth one
    cnot(cq, tq);
    s(cq);
}

// toffoli decomposition
// from: https://arxiv.org/pdf/1210.0974.pdf
// Quantum circuits of T-depth one
void quantum_kernel::controlled_cnot_AM(size_t tq, size_t cq1, size_t cq2) {
    h(tq);
    t(cq1);
    t(cq2);
    t(tq);
    cnot(cq2, cq1);
    cnot(tq, cq2);
    cnot(cq1, tq);
    tdag(cq2);
    cnot(cq1, cq2);
    tdag(cq1);
    tdag(cq2);
    tdag(tq);
    cnot(tq, cq2);
    cnot(cq1, tq);
    cnot(cq2, cq1);
    h(tq);
}

// toffoli decomposition
// Neilsen and Chuang
void quantum_kernel::controlled_cnot_NC(size_t tq, size_t cq1, size_t cq2) {
    h(tq);
    cnot(cq2,tq);
    tdag(tq);
    cnot(cq1,tq);
    t(tq);
    cnot(cq2,tq);
    tdag(tq);
    cnot(cq1,tq);
    tdag(cq2);
    t(tq);
    cnot(cq1,cq2);
    h(tq);
    tdag(cq2);
    cnot(cq1,cq2);
    t(cq1);
    s(cq2);
}

void quantum_kernel::controlled_swap(size_t tq1, size_t tq2, size_t cq) {
    // from: https://arxiv.org/pdf/1210.0974.pdf
    // Quantum circuits of T-depth one
    cnot(tq2, tq1);
    cnot(cq, tq1);
    h(tq2);
    t(cq);
    tdag(tq1);
    t(tq2);
    cnot(tq2, tq1);
    cnot(cq, tq2);
    t(tq1);
    cnot(cq, tq1);
    tdag(tq2);
    tdag(tq1);
    cnot(cq, tq2);
    cnot(tq2, tq1);
    t(tq1);
    h(tq2);
    cnot(tq2, tq1);
}

void quantum_kernel::controlled_rx(size_t tq, size_t cq, double theta) {
    rx(tq, theta/2);
    cz(cq, tq);
    rx(tq, -theta/2);
    cz(cq, tq);
}

void quantum_kernel::controlled_ry(size_t tq, size_t cq, double theta) {
    ry(tq, theta/2);
    cnot(cq, tq);
    ry(tq, -theta/2);
    cnot(cq, tq);
}

void quantum_kernel::controlled_rz(size_t tq, size_t cq, double theta) {
    rz(tq, theta/2);
    cnot(cq, tq);
    rz(tq, -theta/2);
    cnot(cq, tq);
}

void quantum_kernel::controlled_single(
    const ql::quantum_kernel *k,
    size_t control_qubit,
    size_t ancilla_qubit
) {
    const ql::circuit &ckt = k->get_circuit();
    for (auto &g : ckt) {
        std::string gname = g->name;
        ql::gate_type_t gtype = g->type();
        std::vector<size_t> goperands = g->operands;
        DOUT("Generating controlled gate for " << gname);
        DOUT("Type : " << gtype);
        if (gtype == __pauli_x_gate__ || gtype == __rx180_gate__) {
            size_t tq = goperands[0];
            size_t cq = control_qubit;
            controlled_x(tq, cq);
        } else if (gtype == __pauli_y_gate__ || gtype == __ry180_gate__) {
            size_t tq = goperands[0];
            size_t cq = control_qubit;
            controlled_y(tq, cq);
        } else if (gtype == __pauli_z_gate__) {
            size_t tq = goperands[0];
            size_t cq = control_qubit;
            controlled_z(tq, cq);
        } else if (gtype == __hadamard_gate__) {
            size_t tq = goperands[0];
            size_t cq = control_qubit;
            controlled_h(tq, cq);
        } else if (gtype == __identity_gate__) {
            size_t tq = goperands[0];
            size_t cq = control_qubit;
            controlled_i(tq, cq);
        } else if (gtype == __t_gate__) {
            size_t tq = goperands[0];
            size_t cq = control_qubit;
            size_t aq = ancilla_qubit;
            controlled_t(tq, cq, aq);
        } else if (gtype == __tdag_gate__) {
            size_t tq = goperands[0];
            size_t cq = control_qubit;
            size_t aq = ancilla_qubit;
            controlled_tdag(tq, cq, aq);
        } else if (gtype == __phase_gate__) {
            size_t tq = goperands[0];
            size_t cq = control_qubit;
            controlled_s(tq, cq);
        } else if (__phasedag_gate__ == gtype) {
            size_t tq = goperands[0];
            size_t cq = control_qubit;
            controlled_sdag(tq, cq);
        } else if (__cnot_gate__ == gtype) {
            size_t cq1 = goperands[0];
            size_t cq2 = control_qubit;
            size_t tq = goperands[1];

            auto opt = ql::options::get("decompose_toffoli");
            if (opt == "AM") {
                controlled_cnot_AM(tq, cq1, cq2);
            } else if (opt == "NC") {
                controlled_cnot_NC(tq, cq1, cq2);
            } else {
                toffoli(cq1, cq2, tq);
            }
        } else if (gtype == __swap_gate__) {
            size_t tq1 = goperands[0];
            size_t tq2 = goperands[1];
            size_t cq = control_qubit;
            controlled_swap(tq1, tq2, cq);
        } else if (gtype == __rx_gate__) {
            size_t tq = goperands[0];
            size_t cq = control_qubit;
            controlled_rx(tq, cq, g->angle);
        } else if (gtype == __ry_gate__) {
            size_t tq = goperands[0];
            size_t cq = control_qubit;
            controlled_ry(tq, cq, g->angle);
        } else if (gtype == __rz_gate__) {
            size_t tq = goperands[0];
            size_t cq = control_qubit;
            controlled_rz(tq, cq, g->angle);
        } else if (gtype == __rx90_gate__) {
            size_t tq = goperands[0];
            size_t cq = control_qubit;
            controlled_rx(tq, cq, K_PI/2);
        } else if (gtype == __mrx90_gate__) {
            size_t tq = goperands[0];
            size_t cq = control_qubit;
            controlled_rx(tq, cq, -1*K_PI/2);
        } else if (gtype == __rx180_gate__) {
            size_t tq = goperands[0];
            size_t cq = control_qubit;
            controlled_rx(tq, cq, K_PI);
            // controlled_x(tq, cq);
        } else if (gtype == __ry90_gate__) {
            size_t tq = goperands[0];
            size_t cq = control_qubit;
            controlled_ry(tq, cq, K_PI/4);
        } else if (gtype == __mry90_gate__) {
            size_t tq = goperands[0];
            size_t cq = control_qubit;
            controlled_ry(tq, cq, -1*K_PI/4);
        } else if (gtype == __ry180_gate__) {
            size_t tq = goperands[0];
            size_t cq = control_qubit;
            controlled_ry(tq, cq, K_PI);
            // controlled_y(tq, cq);
        } else {
            EOUT("Controlled version of gate '" << gname << "' not defined !");
            throw ql::exception("[x] error : ql::kernel::controlled : Controlled version of gate '"+gname+"' not defined ! ",false);
        }
    }
}

void quantum_kernel::controlled(
    const ql::quantum_kernel *k,
    const std::vector<size_t> &control_qubits,
    const std::vector<size_t> &ancilla_qubits
) {
    DOUT("Generating controlled kernel ... ");
    int ncq = control_qubits.size();
    int naq = ancilla_qubits.size();

    if (ncq == 0) {
        EOUT("At least one control_qubits should be specified !");
        throw ql::exception("[x] error : ql::kernel::controlled : At least one control_qubits should be specified !",false);
    } else if (ncq == 1) {
        //                      control               ancilla
        controlled_single(k, control_qubits[0], ancilla_qubits[0]);
    } else if (ncq > 1) {
        // Network implementing C^n(U) operation
        // - based on Fig. 4.10, p.p 185, Nielson & Chuang
        // - requires as many ancilla/work qubits as control qubits
        if (naq == ncq) {
            toffoli(control_qubits[0], control_qubits[1], ancilla_qubits[0]);

            for (int n = 0; n <= naq - 3; n++) {
                toffoli(control_qubits[n+2], ancilla_qubits[n], ancilla_qubits[n+1]);
            }

            //                      control               ancilla
            controlled_single(k, ancilla_qubits[naq-2], ancilla_qubits[naq-1]);

            for (int n = naq - 3; n >= 0; n--) {
                toffoli(control_qubits[n+2], ancilla_qubits[n], ancilla_qubits[n+1]);
            }

            toffoli(control_qubits[0], control_qubits[1], ancilla_qubits[0]);
        } else {
            EOUT("No. of control qubits should be equal to No. of ancilla qubits!");
            throw ql::exception("[x] error : ql::kernel::controlled : No. of control qubits should be equal to No. of ancilla qubits!",false);
        }
    }

    DOUT("Generating controlled kernel [Done]");
}

void quantum_kernel::conjugate(const ql::quantum_kernel *k) {
    COUT("Generating conjugate kernel");
    const ql::circuit &ckt = k->get_circuit();
    for (auto rgit = ckt.rbegin(); rgit != ckt.rend(); ++rgit) {
        auto g = *rgit;
        std::string gname = g->name;
        ql::gate_type_t gtype = g->type();
        DOUT("Generating conjugate gate for " << gname);
        DOUT("Type : " << gtype);
        if (gtype == __pauli_x_gate__ || gtype == __rx180_gate__) {
            gate("x", g->operands, {}, g->duration, g->angle);
        } else if (gtype == __pauli_y_gate__ || gtype == __ry180_gate__) {
            gate("y", g->operands, {}, g->duration, g->angle);
        } else if (gtype == __pauli_z_gate__) {
            gate("z", g->operands, {}, g->duration, g->angle);
        } else if (gtype == __hadamard_gate__) {
            gate("hadamard", g->operands, {}, g->duration, g->angle);
        } else if (gtype == __identity_gate__) {
            gate("identity", g->operands, {}, g->duration, g->angle);
        } else if (gtype == __t_gate__) {
            gate("tdag", g->operands, {}, g->duration, g->angle);
        } else if (gtype == __tdag_gate__) {
            gate("t", g->operands, {}, g->duration, g->angle);
        } else if (gtype == __phase_gate__) {
            gate("sdag", g->operands, {}, g->duration, g->angle);
        } else if (gtype == __phasedag_gate__) {
            gate("s", g->operands, {}, g->duration, g->angle);
        } else if (gtype == __cnot_gate__) {
            gate("cnot", g->operands, {}, g->duration, g->angle);
        } else if (gtype == __swap_gate__) {
            gate("swap", g->operands, {}, g->duration, g->angle);
        } else if (gtype == __rx_gate__) {
            gate("rx", g->operands, {}, g->duration, -(g->angle) );
        } else if (gtype == __ry_gate__) {
            gate("ry", g->operands, {}, g->duration, -(g->angle) );
        } else if (gtype == __rz_gate__) {
            gate("rz", g->operands, {}, g->duration, -(g->angle) );
        } else if (gtype == __rx90_gate__) {
            gate("mrx90", g->operands, {}, g->duration, g->angle);
        } else if (gtype == __mrx90_gate__) {
            gate("rx90", g->operands, {}, g->duration, g->angle);
        } else if (gtype == __rx180_gate__) {
            gate("x", g->operands, {}, g->duration, g->angle);
        } else if (gtype == __ry90_gate__) {
            gate("mry90", g->operands, {}, g->duration, g->angle);
        } else if (gtype == __mry90_gate__) {
            gate("ry90", g->operands, {}, g->duration, g->angle);
        } else if (gtype == __ry180_gate__) {
            gate("y", g->operands, {}, g->duration, g->angle);
        } else if (gtype == __cphase_gate__) {
            gate("cphase", g->operands, {}, g->duration, g->angle);
        } else if (gtype == __toffoli_gate__) {
            gate("toffoli", g->operands, {}, g->duration, g->angle);
        } else {
            EOUT("Conjugate version of gate '" << gname << "' not defined !");
            throw ql::exception("[x] error : ql::kernel::conjugate : Conjugate version of gate '" + gname + "' not defined ! ", false);
        }
    }
    COUT("Generating conjugate kernel [Done]");
}

} // namespace ql
