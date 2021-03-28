/** \file
 * Circuit (i.e. gate container) implementation.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "ql/utils/tree.h"
#include "gate.h"

namespace ql {
namespace ir {

using Circuit = Gates;

void print(const Circuit &c);

/**
 * generate qasm for a given circuit
 */
utils::Str qasm(const Circuit &c);

utils::Vec<Circuit> split_circuit(Circuit &x);

/**
 * detect measurements and qubit preparations
 */
utils::Bool contains_measurements(const Circuit &x);

} // namespace ir
} // namespace ql
