/** \file
 * Main include for the IR structures. Simply includes the header generated by
 * tree-gen and then adds a few shorthand typedefs.
 */

#pragma once

#include <regex>
#include "ql/utils/tree.h"
#include "ql/ir/ir.gen.h"

namespace ql {
namespace ir {

/**
 * Reference to the complete IR.
 */
using Ref = utils::One<Root>;

/**
 * Reference to the platform subtree.
 */
using PlatformRef = utils::One<Platform>;

/**
 * Reference to an instruction decomposition.
 */
using DecompositionRef = utils::One<InstructionDecomposition>;

/**
 * Reference to the program subtree.
 */
using ProgramRef = utils::Maybe<Program>;

/**
 * Reference to a block.
 */
using BlockRef = utils::One<Block>;

/**
 * Reference to a sub-block.
 */
using SubBlockRef = utils::One<SubBlock>;

/**
 * Reference to either a block or a sub-block.
 */
using BlockBaseRef = utils::One<BlockBase>;

/**
 * Reference to a statement.
 */
using StatementRef = utils::One<Statement>;

/**
 * Reference to an instruction.
 */
using InstructionRef = utils::One<Instruction>;

/**
 * Reference to a custom instruction.
 */
using CustomInstructionRef = utils::One<CustomInstruction>;

/**
 * Link to a (custom) instruction type.
 */
using InstructionTypeLink = utils::Link<InstructionType>;

/**
 * Link to a function type.
 */
using FunctionTypeLink = utils::Link<FunctionType>;

/**
 * Reference to an expression.
 */
using ExpressionRef = utils::One<Expression>;

/**
 * Reference to a data type.
 */
using DataTypeLink = utils::Link<DataType>;

/**
 * Reference to an object.
 */
using ObjectLink = utils::Link<Object>;

/**
 * Regular expression for valid identifiers.
 */
static const std::regex IDENTIFIER_RE{"[a-zA-Z_][a-zA-Z0-9_]*"};

} // namespace ir
} // namespace ql
