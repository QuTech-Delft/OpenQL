/** \file
 * Find circuit variations from commutable sets of gates and select shortest.
 *
 * Commutation of gates such as Control-Unitaries (CZ, CNOT, etc.) is exploited
 * to find all variations of a given circuit by varying on the order of those
 * commutations. The resource-constrained scheduler is used to compute the
 * circuit latency of each circuit variation's schedule. At the end, the current
 * kernel's circuit is replaced by the variation with the minimum circuit
 * latency, and the scheduler_commute option is set to false to prevent a later
 * scheduler to undo this reorder. Since the dependence graph of the scheduler
 * is used to represent the commuting sets of gates, the option schedule_commute
 * must have been set to yes for the above to function.
 *
 * For exploring semantically equivalent versions of a circuit that differ only
 * by commutation such as small error correction circuits, each of the
 * variations can be printed to a separate file, if desired.
 *
 * Control-Unitaries (e.g. CZ and CNOT) commute when their first operands are
 * the same qubit. Furthermore, CNOTs in addition commute when their second
 * operands are the same qubit. The OpenQL depgraph construction recognizes
 * these and represents these in the dependency graph:
 *
 * - The Control-Unitary's first operands are seen as Reads. On each such Read a
 *   dependency is created from the last Write (RAW) or last D (RAD) (i.e. last
 *   non-Read) to the Control-Unitary, and on each first Write or D (i.e. first
 *   non-Read) after a set of Reads, dependencies are created from those
 *   Control-Unitaries to that first Write (WAR) or that first D (DAR).
 * - The CNOT's second operands are seen as Ds (the D stands for controlleD).
 *   On each such D a dependency is created from the last Write (DAW) or last
 *   Read (DAR) (i.e. last non-D) to the CNOT, and on each first Write or Read
 *   (i.e. first non-D) after a set of Ds, dependencies are created from those
 *   CNOTs to that first Write (WAD) or that first Read (RAD).
 *
 * The commutable sets of Control-Unitaries (resp. CNOTs) can be found in the
 * dependency graph by finding those first non-Read (/first non-D) nodes that
 * have such incoming WAR/DAR (/WAD/RAD) dependencies and considering the nodes
 * that those incoming dependencies come from; those nodes form the commutable
 * sets. Recognition of commutation during dependency graph construction is
 * enabled by pre-setting the option scheduler_commute to yes.
 */

#pragma once

#include "utils/str.h"
#include "program.h"
#include "platform.h"

namespace ql {

// commute_variation pass
void commute_variation(
    quantum_program *programp,              // updates the circuits of the program
    const quantum_platform &platform,
    const utils::Str &passname
);

} // namespace ql
