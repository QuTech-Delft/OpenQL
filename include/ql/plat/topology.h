/** \file
 * Definition and access functions to the grid of qubits that supports the real
 * qubits.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/pair.h"
#include "ql/utils/list.h"
#include "ql/utils/vec.h"
#include "ql/utils/map.h"
#include "ql/utils/json.h"

namespace ql {
namespace plat {

/**
 * Qubit grid form/shape.
 */
enum class GridForm {

    /**
     * Qubits have integer X/Y coordinates associated with them.
     */
    XY,

    /**
     * Qubits do not have any kind of coordinates associated with them.
     */
    IRREGULAR

};

/**
 * String representation for GridForm.
 */
std::ostream &operator<<(std::ostream &os, GridForm gf);

/**
 * A coordinate as used by GridForm::XY.
 */
struct XYCoordinate {
    utils::Int x;
    utils::Int y;
};

/**
 * String representation for XYCoordinate.
 */
std::ostream &operator<<(std::ostream &os, XYCoordinate xy);

/**
 * Qubit connectivity mode.
 */
enum class GridConnectivity {

    /**
     * Qubit connectivity is specified in the platform configuration file via
     * the "edges" section.
     */
    SPECIFIED,

    /**
     * Qubit connectivity is not specified in the platform configuration file.
     * Full connectivity is assumed.
     */
    FULL

};

/**
 * String representation for GridConnectivity.
 */
std::ostream &operator<<(std::ostream &os, GridConnectivity gc);

/**
 * Qubit grid abstraction layer.
 */
class Grid {
public:

    /**
     * Shorthand for a qubit index.
     */
    using Qubit = utils::UInt;

    /**
     * Shorthand for a pair of qubits.
     */
    using QubitPair = utils::Pair<Qubit, Qubit>;

    /**
     * Shorthand for an edge index.
     */
    using Edge = utils::Int;

    /**
     * A list of neighboring qubits.
     */
    using Neighbors = utils::List<Qubit>;

private:

    /**
     * Shorthand for a map from a qubit number to something else.
     */
    template <class T>
    using QubitMap = utils::Map<Qubit, T>;

    /**
     * The total number of qubits in the platform.
     */
    utils::UInt num_qubits;

    /**
     * The number of quantum cores. If greater than 1, each core is assumed to
     * have the same number of qubits, being num_qubits/num_cores.
     */
    utils::UInt num_cores;

    /**
     * Number of communication qubits per core. The first num_comm_qubits qubits
     * associated with each core is considered to be a communication qubit.
     */
    utils::UInt num_comm_qubits;

    /**
     * The grid form/shape.
     */
    GridForm form;

    /**
     * If this is an XY grid, this is the size of the grid; all X coordinates
     * must be between 0 and xy_size.x-1, and all Y coordinates must be between
     * 0 and xy_size.y-1.
     */
    XYCoordinate xy_size;

    /**
     * If this is an XY grid, this contains the coordinates for each qubit.
     */
    QubitMap<XYCoordinate> xy_coord;

    /**
     * Connectivity of the grid.
     */
    GridConnectivity connectivity;

    /**
     * The list of neighboring qubits for each qubit.
     */
    QubitMap<Neighbors> neighbors;

    /**
     * Edge to qubit pair map. Only used for specified connectivity.
     */
    utils::Map<Edge, QubitPair> edge_to_qubits;

    /**
     * Qubit pair to edge index. Only used for specified connectivity.
     */
    utils::Map<QubitPair, Edge> qubits_to_edge;

    /**
     * The distance (number of edges) between a pair of qubits.
     */
    utils::Vec<utils::Vec<utils::UInt>> distance;

public:

    /**
     * Constructs the grid for the given number of qubits from the given JSON
     * object.
     *
     * The topology JSON object must have the following structure.
     *
     * ```json
     * {
     *     "form": <optional string, either "xy" or "irregular">,
     *     "x_size": <optional integer for form="xy">,
     *     "y_size": <optional integer for form="xy">,
     *     "qubits": <mandatory array of objects for form="xy">,
     *     "number_of_cores": <optional positive integer, default 1>,
     *     "comm_qubits_per_core": <optional positive integer, num_qubits / number_of_cores>,
     *     "connectivity": <optional string, either "specified" or "full">,
     *     "edges": <mandatory array of objects for connectivity="full">
     *     ...
     * }
     * ```
     *
     * The "form" key specifies whether the qubits can be arranged in a 2D grid
     * of integer coordinates ("xy") or not ("irregular"). If irregular, mapper
     * heuristics that rely on sorting possible paths by angle are unavailable.
     * If xy, "x_size" and "y_size" specify the coordinate ranges (from zero to
     * the limit minus one), and "qubits" specifies the coordinates. "qubits"
     * must then be an array of objects of the following form:
     *
     * ```json
     * {
     *     "id": <qubit index, mandatory>,
     *     "x": <X coordinate, mandatory>,
     *     "y": <Y coordinate, mandatory>,
     *     ...
     * }
     * ```
     *
     * Each qubit must be specified exactly once. Any additional keys in the
     * object are silently ignored, as other parts of OpenQL may use the
     * structure as well.
     *
     * If the "form" key is missing, its value is derived from whether a
     * "qubits" list is given. If "x_size" or "y_size" are missing, the values
     * are inferred from the largest coordinate found in "qubits".
     *
     * The "number_of_cores" key is used to specify multi-core architectures.
     * It must be a positive integer. Each core is assumed to have the same
     * number of qubits, so the total number of qubits must be divisible by this
     * number.
     *
     * Cores can communicate only via communication qubits. The amount of these
     * qubits per cores may be set using the "comm_qubits_per_core" key. Its
     * value must range between 1 and the number of qubits per core, and
     * defaults to the latter. The first N qubits for each core are considered
     * to be communication qubits, whereas the remainder are local qubits.
     *
     * The "connectivity" key specifies whether there are qubit connectivity
     * constraints ("specified") or all qubits (within a core) are connected
     * ("full"). In the former case, the "edges" key must map to an array of
     * objects of the following form:
     *
     * ```json
     * {
     *     "id": <optional unique identifying integer>,
     *     "src": <source qubit index, mandatory>,
     *     "dst": <target qubit index, mandatory>,
     *     ...
     * }
     * ```
     *
     * Edges are directional; to allow qubits to interact "in both ways," both
     * directions must be specified. If any identifiers are specified, all edges
     * should get one, and they should all be unique; otherwise, indices are
     * generated using src*nq+dst. Any additional keys in the object are
     * silently ignored, as other parts of OpenQL may use the structure as well
     * (although they should preferably just extend this class).
     *
     * When "connectivity" is set to "full" in a multi-core environment,
     * inter-core edges are only generated when both the source and destination
     * qubit is a communication qubit.
     *
     * If the "connectivity" key is missing, its value is derived from whether
     * an "edges" list is given.
     *
     * Any additional keys in the topology root object are silently ignored, as
     * other parts of OpenQL may use the structure as well.
     */
    Grid(utils::UInt num_qubits, const utils::Json &topology);

    /**
     * Returns the edge index for the given qubit pair, or returns -1 when there
     * is no defined edge index for the given qubit pair.
     */
    Edge get_edge_index(QubitPair qs) const;

    /**
     * Returns the qubit pair corresponding with the given edge, or returns 0,0
     * when there is no edge with the given index.
     */
    QubitPair get_edge_qubits(Edge edge) const;

    /**
     * Returns the indices of the neighboring qubits for the given qubit.
     */
    const Neighbors &get_neighbors(Qubit qubit) const;

    /**
     * Returns the number of cores.
     */
    utils::UInt get_num_cores() const;

    /**
     * Returns whether the given qubit is a communication qubit of a core.
     */
    utils::Bool is_comm_qubit(Qubit qubit) const;

    /**
     * Returns the core index for the given qubit in a multi-core environment.
     */
    utils::UInt get_core_index(Qubit qubit) const;

    /**
     * Returns whether communication between the given two qubits involves
     * inter-core communication.
     */
    utils::Bool is_inter_core_hop(Qubit source, Qubit target) const;

    /**
     * Returns the distance between the two given qubits in number of hops.
     * Returns 0 iff source == target.
     */
    utils::UInt get_distance(Qubit source, Qubit target) const;

    /**
     * Returns the distance between the given two qubits in terms of cores.
     */
    utils::UInt get_core_distance(Qubit source, Qubit target) const;

    /**
     * Minimum number of hops between two qubits is always >= distance(from, to)
     * and inside one core (or without multi-core) the minimum number of
     * hops == distance.
     *
     * However, in multi-core with inter-core hops, an inter-core hop cannot
     * execute a 2qgate so when the minimum number of hops are all inter-core
     * hops (so distance(from,to) == coredistance(from,to)) and no 2qgate has
     * been placed yet, then at least one additional inter-core hop is needed
     * for the 2qgate, the number of hops required being at least distance+1.
     *
     * We assume below that a valid path exists with distance+1 hops; this fails
     * when not all qubits in a core support connections to all other cores.
     * See the check in initialization of neighbors.
     */
    utils::UInt get_min_hops(Qubit source, Qubit target) const;

    /**
     * Returns whether qubits have coordinates associated with them.
     */
    utils::Bool has_coordinates() const;

    /**
     * Rotate neighbors list such that largest angle difference between adjacent
     * elements is behind back. This is needed when a given subset of variations
     * from a node is wanted (mappathselect==borders). This can only be computed
     * when there is an underlying x/y grid (so not for form==gf_irregular).
     *
     * TODO JvS: does this even belong in grid now that it's not part of the
     *  mapper anymore? It feels like a very specific thing.
     */
    void sort_neighbors_by_angle(Qubit src, Neighbors &nbl) const;

    /**
     * Dumps the grid configuration to the given stream.
     */
    void dump(std::ostream &os=std::cout, const utils::Str &line_prefix="") const;

};

} // namespace plat
} // namespace ql
