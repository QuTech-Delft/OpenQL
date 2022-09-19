/** \file
 * Unitary matrix (decomposition) implementation.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "ql/ir/compat/compat.h"

namespace ql {
namespace com {
namespace dec {

/**
 * Unitary matrix decomposition class.
 *
 * This is actually just a wrapper for the real thing, which lives entirely
 * inside unitary.cc. This allows the unitary decomposition logic to be disabled
 * during compilation by substituting a mocked implementation, reducing
 * compilation times immensely, because Eigen is absolutely massive.
 */
class Unitary {
private:

    /**
     * Whether this unitary gate has been decomposed yet.
     */
    utils::Bool decomposed;

    /**
     * The list of decomposed instructions.
     *
     * TODO: document and refactor, using Reals to communicate enum information
     *  is not okay...
     */
    utils::Vec<utils::Real> instruction_list;

public:

    /**
     * The name of the unitary gate.
     */
    const utils::Str name;

    /**
     * The unitary matrix in row-major form.
     */
    const utils::Vec<utils::Complex> array;

    /**
     * Creates a unitary gate with the given name and row-major unitary matrix.
     */
    Unitary(const utils::Str &name, const utils::Vec<utils::Complex> &array);

    /**
     * Returns the number of elements in the incoming matrix.
     */
    utils::UInt size() const;

    /**
     * Explicitly runs the matrix decomposition algorithm. Used to be required,
     * nowadays is called implicitly by get_decomposition() if not done explicitly.
     */
    void decompose();

    /**
     * Returns whether unitary decomposition support was enabled in this build
     * of OpenQL.
     */
    static utils::Bool is_decompose_support_enabled();

    /**
     * Returns the decomposed circuit.
     */
    ir::compat::GateRefs get_decomposition(const utils::Vec<utils::UInt> &qubits);

    /**
     * Does state preparation, results in a circuit for which A|0> = |psi> for given state psi, stored as the array in Unitary
     */
    ir::compat::GateRefs prepare_state(const utils::Vec<utils::UInt> &qubits);

};

} // namespace dec
} // namespace com
} // namespace ql
