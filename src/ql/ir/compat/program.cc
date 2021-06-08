/** \file
 * Quantum program abstraction implementation.
 */

#include <ql/pmgr/manager.h>
#include "ql/ir/compat/program.h"

#include "ql/utils/filesystem.h"
#include "ql/com/options.h"

static unsigned long phi_node_count = 0;    // FIXME: number across quantum_program instances

namespace ql {
namespace ir {
namespace compat {

using namespace utils;

/**
 * Constructs a new program.
 */
Program::Program(
    const Str &name,
    const PlatformRef &platform,
    UInt qubit_count,
    UInt creg_count,
    UInt breg_count
) :
    name(name),
    unique_name(name),
    platform(platform),
    qubit_count(qubit_count),
    creg_count(creg_count),
    breg_count(breg_count)
{
    if (qubit_count > platform->qubit_count) {
        throw Exception(
            "cannot create program (" + name + ") "
            + "that uses more qubits (" + to_string(qubit_count) + ") "
            + "than the platform has (" + to_string(platform->qubit_count) + ")"
        );
    }
    if (creg_count > platform->creg_count) {
        if (platform->compat_implicit_creg_count) {
            platform->creg_count = creg_count;
        } else {
            throw Exception(
                "cannot create program (" + name + ") "
                + "that uses more cregs (" + to_string(creg_count) + ") "
                + "than the platform has (" + to_string(platform->creg_count) + ")"
            );
        }
    }
    if (breg_count > platform->breg_count) {
        if (platform->compat_implicit_breg_count) {
            platform->breg_count = breg_count;
        } else {
            throw Exception(
                "cannot create program (" + name + ") "
                + "that uses more bregs (" + to_string(breg_count) + ") "
                + "than the platform has (" + to_string(platform->breg_count) + ")"
            );
        }
    }

    // Generate unique filename if requested via the unique_output option.
    if (com::options::global["unique_output"].as_bool()) {

        // Filename for the name uniquification number.
        Str version_file = QL_SS2S(com::options::get("output_dir") << "/" << name << ".unique");

        // Retrieve old version number, if one exists.
        UInt vers = 0;
        if (is_file(version_file)) {
            InFile(version_file) >> vers;
        }

        // Increment to get new one.
        vers++;

        // Store version for a later run.
        OutFile(version_file) << vers;

        if (vers > 1) {
            unique_name = name + to_string(vers);
            QL_DOUT("Unique program name is " << unique_name << ", based on version " << vers);
        }
    }

}

/**
 * Adds the given kernel to the end of the program, after checking that it's
 * safe to add.
 */
void Program::add(const KernelRef &kernel) {

    // Check name uniqueness.
    // FIXME: use a set or a map!
    for (const auto &kernel2 : kernels) {
        if (kernel2->name == kernel->name) {
            throw Exception("duplicate kernel name: " + kernel->name);
        }
    }

    // Check platform.
    if (kernel->platform.get_ptr() != platform.get_ptr()) {
        throw Exception(
            "cannot add kernel (" + kernel->name + ") "
            "built using a different platform"
        );
    }

    // Check register counts.
    if (kernel->qubit_count > qubit_count) {
        throw Exception(
            "cannot add kernel (" + kernel->name + ") " +
            "that uses more qubits (" + to_string(kernel->qubit_count) + ") " +
            "than the program declares ( "+ to_string(qubit_count) + ")"
        );
    }
    if (kernel->creg_count > creg_count) {
        throw Exception(
            "cannot add kernel (" + kernel->name + ") " +
            "that uses more cregs (" + to_string(kernel->creg_count) + ") " +
            "than the program declares ( "+ to_string(creg_count) + ")"
        );
    }
    if (kernel->breg_count > breg_count) {
        throw Exception(
            "cannot add kernel (" + kernel->name + ") " +
            "that uses more bregs (" + to_string(kernel->breg_count) + ") " +
            "than the program declares ( "+ to_string(breg_count) + ")"
        );
    }

    // If sane, add kernel to list of kernels.
    kernels.add(kernel);

}

/**
 * Adds the kernels in the given (sub)program to the end of this program,
 * checking for each kernel whether it's safe to add.
 */
void Program::add_program(const ProgramRef &p) {
    for (auto &k : p->kernels) {
        add(k);
    }
}

/**
 * Adds a conditional kernel, conditioned by a classical operation via
 * classical flow control.
 */
void Program::add_if(const KernelRef &k, const ClassicalOperation &cond) {
    // phi node
    auto kphi1 = KernelRef::make(k->name+"_if", platform, qubit_count, creg_count, breg_count);
    kphi1->set_kernel_type(KernelType::IF_START);
    kphi1->set_condition(cond);
    kernels.add(kphi1);

    add(k);

    // phi node
    auto kphi2 = KernelRef::make(k->name+"_if_end", platform, qubit_count, creg_count, breg_count);
    kphi2->set_kernel_type(KernelType::IF_END);
    kphi2->set_condition(cond);
    kernels.add(kphi2);
}

void Program::add_if(const ProgramRef &p, const ClassicalOperation &cond) {
    // phi node
    auto kphi1 = KernelRef::make(p->name+"_if", platform, qubit_count, creg_count, breg_count);
    kphi1->set_kernel_type(KernelType::IF_START);
    kphi1->set_condition(cond);
    kernels.add(kphi1);

    add_program(p);

    // phi node
    auto kphi2 = KernelRef::make(p->name+"_if_end", platform, qubit_count, creg_count, breg_count);
    kphi2->set_kernel_type(KernelType::IF_END);
    kphi2->set_condition(cond);
    kernels.add(kphi2);
}

/**
 * Adds two conditional kernels, conditioned by a classical operation and
 * its complement respectively via classical flow control.
 */
void Program::add_if_else(
    const KernelRef &k_if,
    const KernelRef &k_else,
    const ClassicalOperation &cond
) {
    auto kphi1 = KernelRef::make(k_if->name+"_if"+ to_string(phi_node_count), platform, qubit_count, creg_count, breg_count);
    kphi1->set_kernel_type(KernelType::IF_START);
    kphi1->set_condition(cond);
    kernels.add(kphi1);

    add(k_if);

    // phi node
    auto kphi2 = KernelRef::make(k_if->name+"_if"+ to_string(phi_node_count) +"_end", platform, qubit_count, creg_count, breg_count);
    kphi2->set_kernel_type(KernelType::IF_END);
    kphi2->set_condition(cond);
    kernels.add(kphi2);


    // phi node
    auto kphi3 = KernelRef::make(k_else->name+"_else" + to_string(phi_node_count), platform, qubit_count, creg_count, breg_count);
    kphi3->set_kernel_type(KernelType::ELSE_START);
    kphi3->set_condition(cond);
    kernels.add(kphi3);

    add(k_else);

    // phi node
    auto kphi4 = KernelRef::make(k_else->name+"_else" + to_string(phi_node_count)+"_end", platform, qubit_count, creg_count, breg_count);
    kphi4->set_kernel_type(KernelType::ELSE_END);
    kphi4->set_condition(cond);
    kernels.add(kphi4);

    phi_node_count++;
}

void Program::add_if_else(
    const ProgramRef &p_if,
    const ProgramRef &p_else,
    const ClassicalOperation &cond
) {
    auto kphi1 = KernelRef::make(p_if->name+"_if"+ to_string(phi_node_count), platform, qubit_count, creg_count, breg_count);
    kphi1->set_kernel_type(KernelType::IF_START);
    kphi1->set_condition(cond);
    kernels.add(kphi1);

    add_program(p_if);

    // phi node
    auto kphi2 = KernelRef::make(p_if->name+"_if"+ to_string(phi_node_count) +"_end", platform, qubit_count, creg_count, breg_count);
    kphi2->set_kernel_type(KernelType::IF_END);
    kphi2->set_condition(cond);
    kernels.add(kphi2);


    // phi node
    auto kphi3 = KernelRef::make(p_else->name+"_else" + to_string(phi_node_count), platform, qubit_count, creg_count, breg_count);
    kphi3->set_kernel_type(KernelType::ELSE_START);
    kphi3->set_condition(cond);
    kernels.add(kphi3);

    add_program(p_else);

    // phi node
    auto kphi4 = KernelRef::make(p_else->name+"_else" + to_string(phi_node_count)+"_end", platform, qubit_count, creg_count, breg_count);
    kphi4->set_kernel_type(KernelType::ELSE_END);
    kphi4->set_condition(cond);
    kernels.add(kphi4);

    phi_node_count++;
}

/**
 * Adds a do-while loop with the given kernel as the body.
 */
void Program::add_do_while(const KernelRef &k, const ClassicalOperation &cond) {
    // phi node
    auto kphi1 = KernelRef::make(k->name+"_do_while"+ to_string(phi_node_count) +"_start", platform, qubit_count, creg_count, breg_count);
    kphi1->set_kernel_type(KernelType::DO_WHILE_START);
    kphi1->set_condition(cond);
    kernels.add(kphi1);

    add(k);

    // phi node
    auto kphi2 = KernelRef::make(k->name+"_do_while" + to_string(phi_node_count), platform, qubit_count, creg_count, breg_count);
    kphi2->set_kernel_type(KernelType::DO_WHILE_END);
    kphi2->set_condition(cond);
    kernels.add(kphi2);
    phi_node_count++;
}

/**
 * Adds a do-while loop with the given program as the body.
 */
void Program::add_do_while(const ProgramRef &p, const ClassicalOperation &cond) {
    // phi node
    auto kphi1 = KernelRef::make(p->name+"_do_while"+ to_string(phi_node_count) +"_start", platform, qubit_count, creg_count, breg_count);
    kphi1->set_kernel_type(KernelType::DO_WHILE_START);
    kphi1->set_condition(cond);
    kernels.add(kphi1);

    add_program(p);

    // phi node
    auto kphi2 = KernelRef::make(p->name+"_do_while" + to_string(phi_node_count), platform, qubit_count, creg_count, breg_count);
    kphi2->set_kernel_type(KernelType::DO_WHILE_END);
    kphi2->set_condition(cond);
    kernels.add(kphi2);
    phi_node_count++;
}

/**
 * Adds a static for loop with the given kernel as the body.
 */
void Program::add_for(const KernelRef &k, UInt iterations) {
    // phi node
    auto kphi1 = KernelRef::make(k->name+"_for"+ to_string(phi_node_count) +"_start", platform, qubit_count, creg_count, breg_count);
    kphi1->set_kernel_type(KernelType::FOR_START);
    kphi1->iteration_count = iterations;
    kernels.add(kphi1);

    add(k);
    kernels.back()->iteration_count = iterations;

    // phi node
    auto kphi2 = KernelRef::make(k->name+"_for" + to_string(phi_node_count) +"_end", platform, qubit_count, creg_count, breg_count);
    kphi2->set_kernel_type(KernelType::FOR_END);
    kernels.add(kphi2);
    phi_node_count++;
}

/**
 * Adds a static for loop with the given program as the body.
 */
void Program::add_for(const ProgramRef &p, UInt iterations) {

    // optimize away if zero iterations
    if (iterations <= 0) {
        return;
    }

    // phi node
    auto kphi1 = KernelRef::make(p->name+"_for"+ to_string(phi_node_count) +"_start", platform, qubit_count, creg_count, breg_count);
    kphi1->set_kernel_type(KernelType::FOR_START);
    kphi1->iteration_count = iterations;
    kernels.add(kphi1);

    // phi node
    auto kphi2 = KernelRef::make(p->name, platform, qubit_count, creg_count, breg_count);
    kphi2->set_kernel_type(KernelType::STATIC);
    kernels.add(kphi2);

    add_program(p);

    // phi node
    auto kphi3 = KernelRef::make(p->name+"_for" + to_string(phi_node_count) +"_end", platform, qubit_count, creg_count, breg_count);
    kphi3->set_kernel_type(KernelType::FOR_END);
    kernels.add(kphi3);
    phi_node_count++;
}

} // namespace compat
} // namespace ir
} // namespace ql
