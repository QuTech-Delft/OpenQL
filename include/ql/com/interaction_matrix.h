/** \file
 * Qubit interaction matrix generator.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "ql/ir/ir.h"

namespace ql {
namespace com {

// TODO JvS: this is really just an "advanced" metric, and should be written
//  as such sometime.

/**
 * Utility for counting the number of two-qubit gates, grouped by their qubit
 * operands.
 */
class InteractionMatrix {
private:

    /**
     * Shorthand for the matrix type.
     */
    using Matrix = utils::Vec<utils::Vec<utils::UInt>>;

    /**
     * Size of the matrix, i.e. the number of qubits.
     */
    const utils::UInt size;

    /**
     * Square matrix of unsigned integers, representing the number of two-qubit
     * gates spanning the indexed qubits. Operand order is not respected; the
     * matrix is symmetric.
     */
    Matrix matrix;

public:

    /**
     * Computes the interaction matrix for the given kernel.
     */
    InteractionMatrix(const ir::KernelRef &kernel);

    /**
     * Returns the embedded matrix.
     */
    const Matrix &get_matrix() const;

    /**
     * Returns the matrix as a string.
     */
    utils::Str get_string() const;

};

} // namespace com
} // namespace ql
