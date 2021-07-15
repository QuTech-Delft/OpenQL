/** \file
 * Defines the ExpressionMapper base class.
 */

#pragma once

#include "ql/ir/ir.h"

namespace ql {
namespace com {
namespace map {

/**
 * Class for implementing map operations on expressions or references. While not
 * an abstract class, this must be subclassed to be useful; either or both of
 * on_expression() and on_reference() must be overridden with the desired map
 * operation.
 */
class ExpressionMapper {
protected:

    /**
     * Called when an expression of any kind is encountered in the tree. The
     * subtree formed by the expression will already have been processed (i.e.
     * traversal is depth-first.) The method may assign the Maybe edge to
     * change the complete expression (including its node type), or may change
     * the contents of the expression. If the method returns true, the subtree
     * formed by the new expression will be processed as well. The default
     * implementation calls on_reference() if the expression is a reference.
     */
    virtual utils::Bool on_expression(utils::Maybe<ir::Expression> &expr);

    /**
     * Like on_expression(), but called for edges that must always be a
     * reference of some kind. The default implementation is no-op and just
     * returns false.
     */
    virtual utils::Bool on_reference(utils::Maybe<ir::Reference> &ref);

private:

    /**
     * Handles visiting an expression subtree before or after on_expression()
     * is called by the callee.
     */
    void recurse_into_expression(const ir::ExpressionRef &expression);

public:

    /**
     * Visits an expression. This processes the subtree formed by the expression
     * depth-first, then calls on_expression(), and if that returns true
     * processes the new subtree depth-first.
     */
    void process_expression(utils::Maybe<ir::Expression> &expression);

    /**
     * Visits a statement. on_expression()/on_reference() will be called for
     * all expression/reference edges found in the statement, depth-first.
     */
    void process_statement(const ir::StatementRef &statement);

    /**
     * Visits a block. on_expression()/on_reference() will be called for
     * all expression/reference edges found in the block, depth-first.
     */
    void process_block(const ir::BlockBaseRef &block);

};

} // namespace map
} // namespace com
} // namespace ql
