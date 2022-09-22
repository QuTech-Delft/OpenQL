/** \file
 * Defines the structures and functions used to construct the control-flow
 * graph for a program.
 */

#pragma once

#include "ql/com/cfg/types.h"

namespace ql {
namespace com {
namespace cfg {

/**
 * Builds a control-flow graph for the given program.
 *
 * It's not possible to construct a CFG for a program that still contains
 * structured control-flow. It must in that case be converted to basic-block
 * form first.
 *
 * The nodes of the graph are represented by the blocks in the program and
 * two sentinel blocks, known as the source and the sink. The edges are formed
 * by dependencies from one block to another.
 */
void build(const ir::ProgramRef &program);

} // namespace cfg
} // namespace com
} // namespace ql
