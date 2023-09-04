/** \file
 * Defines types for representing the data dependency graph.
 */

#pragma once

#include "ql/utils/map.h"
#include "ql/ir/ir.h"

namespace ql {
namespace com {
namespace ddg {

/**
 * A reference to an object (including index) or a null reference, for the
 * purpose of representing a data dependency. The null reference is used for
 * barriers without operands (i.e. barriers that must have a data dependency
 * with all other objects) and goto instructions: these instructions "write"
 * to the "null object", while all other instructions read from it. This just
 * wraps ir::Reference, in such a way that it can be used as the key for ordered
 * maps and sets, and such that equality is value-based.
 */
class Reference {
public:

    /**
     * Link to the target object.
     */
    utils::Link<ir::Object> target = {};

    /**
     * The data type that the object is accessed as. In almost all cases, this
     * must be equal to target->data_type. The only exception currently allowed
     * is accessing a qubit type as a bit. This yields the implicit classical
     * bit associated with the qubit in targets which use this paradigm.
     */
    utils::Link<ir::DataType> data_type = {};

    /**
     * The indices by which the object is indexed, for as far as they are
     * statically known.
     */
    utils::Vec<utils::UInt> indices = {};

    /**
     * Makes a null static reference, semantically referring to any state in the
     * system.
     */
    Reference() = default;

    /**
     * Converts an IR reference to a static reference object.
     */
    Reference(const utils::One<ir::Reference> &ref);

    /**
     * Converts a static reference back to a normal reference.
     */
    utils::One<ir::Reference> make_reference(const ir::Ref &ir) const;

    /**
     * String conversion for Reference.
     */
    friend std::ostream &operator<<(std::ostream &os, const Reference &reference);

    /**
     * Value-based less-than operator to allow this to be used as a key to
     * a map.
     */
    utils::Bool operator<(const Reference &rhs) const;

    /**
     * Value-based equality operator.
     */
    utils::Bool operator==(const Reference &rhs) const;

    /**
     * Returns whether this is a null reference, i.e. it refers to unknown or
     * global state of the system.
     */
    utils::Bool is_global_state() const;

    /**
     * Returns whether the given reference refers to a statically provable
     * distinct object.
     */
    utils::Bool is_provably_distinct_from(const Reference &reference) const;

    /**
     * Returns whether the given reference refers to a superset of the
     * objects/elements that this reference refers to.
     */
    utils::Bool is_shadowed_by(const Reference &reference) const;

    /**
     * Combines two references into the most specific reference that encompasses
     * both a and b.
     */
    Reference union_with(const Reference &reference) const;

    /**
     * Combines two references into the most specific reference that encompasses
     * the intersection between a and b.
     */
    Reference intersect_with(const Reference &reference) const;

};

/**
 * Object access mode.
 */
class AccessMode {
private:

    /**
     * Enumeration of the modes currently defined. This is intentionally
     * private: all semantics are to be derived from the public methods of the
     * AccessMode class, allowing access modes (and their commutativity
     * relations) to be made configurable in the future.
     */
    enum class Enum {

        /**
         * Used for classical write or non-commuting qubit access. The corresponding
         * operand must be a reference.
         */
        WRITE,

        /**
         * Used for classical read-only access. Other instructions accessing the
         * same operand with mode READ may commute.
         */
        READ,

        /**
         * Used for qubit usage that commutes along the X axis; i.e., other
         * instructions involving the corresponding qubit in mode COMMUTE_X may
         * commute.
         */
        COMMUTE_X,

        /**
         * Used for qubit usage that commutes along the Y axis; i.e., other
         * instructions involving the corresponding qubit in mode COMMUTE_Y may
         * commute.
         */
        COMMUTE_Y,

        /**
         * Used for qubit usage that commutes along the Z axis; i.e., other
         * instructions involving the corresponding qubit in mode COMMUTE_Z may
         * commute.
         */
        COMMUTE_Z

    };

    /**
     * The access type.
     */
    Enum value;

public:

    /**
     * Returns the classical write access mode, that doesn't commute with
     * anything else.
     */
    AccessMode();

    /**
     * Constructs an access mode from a (currently hardcoded) operand mode.
     */
    explicit AccessMode(ir::prim::OperandMode operand_mode);

    /**
     * Returns the classical write access mode, that doesn't commute with
     * anything else.
     */
    static AccessMode write();

    /**
     * Returns the classical read access mode, that commutes with itself but
     * not with write.
     */
    static AccessMode read();

    /**
     * String conversion for AccessMode. Returns its word form.
     */
    friend std::ostream &operator<<(std::ostream &os, const AccessMode &access_mode);

    /**
     * Value-based equality operator.
     */
    utils::Bool operator==(const AccessMode &access_mode) const;

    /**
     * Represents the given access mode as a single character, used to represent
     * the dependency relation between two non-commuting modes (RAW, WAW, WAR,
     * etc.).
     */
    utils::Char as_letter() const;

    /**
     * Returns whether the given two access modes commute. Must be symmetric.
     */
    utils::Bool commutes_with(const AccessMode &access_mode) const;

    /**
     * Combines two modes into one, for example used when a single object is
     * accessed in multiple ways but has to be represented with a single access
     * mode. The requirement on combine_modes(a, b) -> c is that any mode d that
     * does not commute with a OR does not commute with mode b also does not commute
     * with mode c, but the more modes the result commutes with, the less
     * pessimistic the DDG will be.
     */
    AccessMode combine_with(const AccessMode &access_mode) const;

};

/**
 * An object access, a.k.a. event.
 */
struct Event {

    /**
     * Reference to the object being accessed.
     */
    Reference reference = {};

    /**
     * The mode by which it is being accessed.
     */
    AccessMode mode = AccessMode::read();

    /**
     * Creates an Event object from a pair as stored in the Events map.
     */
    Event(const utils::Pair<const Reference, AccessMode> &pair);

    /**
     * String conversion for Event.
     */
    friend std::ostream &operator<<(std::ostream &os, const Event &event);

    /**
     * Returns whether the given event commutes with this event. This is true if
     * the references belonging to the events are statically known to refer to
     * different objects, or if the access modes commute.
     */
    utils::Bool commutes_with(const Event &event) const;

    /**
     * Returns whether the given event completely shadows this event. That is,
     * the access modes don't commute, and the specified reference refers to a
     * superset of the objects referred to by this reference.
     */
    utils::Bool is_shadowed_by(const Event &event) const;

};

/**
 * A number of distinct events.
 */
using Events = utils::Map<Reference, AccessMode>;

/**
 * The type of dependency between two DDG nodes for a given object reference
 * (RAW, WAR, WAW etc). The contained modes should not commute.
 */
struct DependencyType {

    /**
     * The way the object is accessed in the first instruction.
     */
    AccessMode first_mode;

    /**
     * The way the object is accessed in the second instruction.
     */
    AccessMode second_mode;

    /**
     * String conversion for DependencyType.
     */
    friend std::ostream &operator<<(std::ostream &os, const DependencyType &dependency_type);

};

/**
 * Cause for a dependency to exist.
 */
struct Cause {

    /**
     * Reference to the object that caused the dependency.
     */
    Reference reference;

    /**
     * The type of dependency.
     */
    DependencyType dependency_type;

    /**
     * String conversion for Cause.
     */
    friend std::ostream &operator<<(std::ostream &os, const Cause &cause);

};

/**
 * Represents an edge in the data dependency graph.
 */
struct Edge {

    /**
     * Reference to the instruction (and DDG node via the Node annotation) that
     * the edge originates from.
     */
    ir::StatementRef predecessor = {};

    /**
     * Reference to the instruction (and DDG node via the Node annotation) that
     * the edge targets.
     */
    ir::StatementRef successor = {};

    /**
     * The minimum number of cycles that must be between the predecessor and
     * successor in the final schedule. If the DDG is reversed, these values
     * will be zero or negative, otherwise they will be zero or positive.
     */
    utils::Int weight = 0;

    /**
     * The reason(s) for this edge to exist.
     */
    utils::List<Cause> causes = {};

};

/**
 * Reference to a DDG edge.
 */
using EdgeRef = utils::Ptr<Edge>;

/**
 * Const reference to a DDG edge.
 */
using EdgeCRef = utils::Ptr<const Edge>;

/**
 * Shorthand for a list of endpoints for a node.
 */
using Endpoints = std::list<std::pair<ir::StatementRef, EdgeRef>>;

/**
 * A node in the DDG.
 */
struct Node {

    /**
     * The endpoints of the incoming edges for this node.
     */
    Endpoints predecessors;

    /**
     * The endpoints of the outgoing edges for this node.
     */
    Endpoints successors;

    /**
     * The index of the statement this node belongs to within the block it
     * belongs to. This may be used as an ultimate tie-breaker for scheduling
     * heuristics to guarantee stability of instruction order when the heuristic
     * determines two instructions to be equal. Instructions should then be
     * scheduled by increasing value of order, regardless of the scheduling
     * direction (when the DDG is reversed for ALAP scheduling. order is
     * negated along with the edge weights).
     */
    utils::Int order;

};

/**
 * A reference to a DDG node. This is attached to statements via an annotation.
 */
using NodeRef = utils::Ptr<Node>;

/**
 * A const reference to a DDG node.
 */
using NodeCRef = utils::Ptr<const Node>;

/**
 * Annotation structure placed on a block when the DDG is constructed,
 * containing things that need to be tracked for the DDG as a whole.
 */
struct Graph {

    /**
     * The source statement, serving as a sentinel that precedes all other
     * statements.
     */
    utils::One<ir::SentinelStatement> source;

    /**
     * The sink statement, serving as a sentinel that follows all other
     * statements.
     */
    utils::One<ir::SentinelStatement> sink;

    /**
     * The direction of the data dependency graph. This must be either 1 or -1.
     * When 1, the edges are pointed in the logical, causal direction. When -1,
     * the direction, edge weights, and source/sink are reversed. This is useful
     * because the direction of a scheduling algorithm operating on the DDG is
     * effectively reversed by this as well, turning ASAP into ALAP.
     */
    utils::Int direction;

};

struct Remaining {
    utils::UInt remaining = 0;

    Remaining(utils::UInt r) : remaining(r) {};
};

} // namespace ddg
} // namespace com
} // namespace ql
