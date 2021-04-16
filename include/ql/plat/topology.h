/** \file
 * Topology: definition and access functions to the grid of qubits that supports
 * the real qubits.
 *
 * TODO JvS: clean up docs
 * TODO JvS: naming conventions
 *
 * Maintain several maps to ease navigating in the grid; these are constant after initialization.
 *
 * Grid
 *
 * Config file definitions:
 *  nq:                         hardware_settings.qubit_number
 *  topology.number_of_cores:   number_of_cores
 *  topology.form;              gf_xy/gf_irregular: how relation between neighbors is specified
 *  topology.connectivity:      gc_specified/gc_full: how connectivity between qubits is specified
 *  topology.comm_qubits_per_core: number of qubits per core that can communicate directly with qubits in other cores
 *  topology.x_size/y_size:     x/y space, defines underlying grid (only gf_xy)
 *  topology.qubits:            mapping of qubit to x/y coordinates
 *                              (defines x[i]/y[i] for each qubit i) (only gf_xy)
 *  topology.edges:             mapping of edge (physical connection between 2 qubits)
 *                              to its src and dst qubits (defines nbs)
 *
 * Grid public members (apart from nq):
 *  form:               how relation between neighbors is specified
 *  Distance(qi,qj):    distance in physical connection hops from real qubit qi to real qubit qj;
 *                      - computing it relies on nbs (and Floyd-Warshall) (gf_xy and gf_irregular)
 *  nbs[qi]:            list of neighbor real qubits of real qubit qi
 *                      - nbs can be derived from topology.edges (gf_xy and gf_irregular)
 *  sort_neighbors_by_angle(qi, neighborlist):    rotate neighborlist such that largest angle diff around qi is behind last element
 *                      relies on nbs, and x[i]/y[i] (gf_xy only)
 *
 * For an irregular grid form, only nq and edges (so nbs) need to be specified; distance is computed from nbs:
 * - there is no underlying rectangular grid, so there are no defined x and y coordinates of qubits;
 *   this means that sort_neighbors_by_angle as needed by mappathselect==borders cannot work
 * - edges (so nbs) can be specified explicitly (connectivity==gc_specified) or implicitly (it is gc_full):
 *   when connectivity==gc_specified, the edges must be specified in topology.edges in terms of connected qubits;
 *   when connectivity==gc_full, there are edges between all qubits but between cores only between comm_qubits
 *
 * Below, we support regular (xy) grids which need not be fully assigned; this requires edges (so nbs) to be defined,
 *   from which distance is computed; also we have x/y coordinates per qubit specified in the configuration file
 *   An underlying grid with x/y coordinates comes in use for:
 *   - crossbars
 *   - cclight qwg assignment (done in another manner now)
 *   - when mappathselectopt==borders
 *
 * Not implemented:
 * forms gf_cross and gf_plus: given x_size and y_size, the relations are implicitly defined by the internal
 *      diagonal (gf_cross) or horizontal/vertical (gf_plus) connections between grid points (qubits);
 *      with gf_cross only half the grid is occupied by a qubit;
 *      the grid point (0,0) doesn't have a qubit, (1,0) and (0,1) do;
 *      topology.qubits and topology.edges need not be present in the configuration file;
 *      Distance in both forms would be defined by a formula, not a function.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
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
     * The distance (number of edges) between a pair of qubits.
     */
    utils::Vec<utils::Vec<utils::UInt>> distance;

public:

    /**
     * Constructs the grid for the given number of qubits from the given JSON
     * object.
     *
     * TODO: document JSON structure
     */
    Grid(utils::UInt num_qubits, const utils::Json &topology);

    /**
     * Returns the indices of the neighboring qubits for the given qubit.
     */
    const Neighbors &get_neighbors(Qubit qubit) const;

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
