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
 *  Normalize(qi, neighborlist):    rotate neighborlist such that largest angle diff around qi is behind last element
 *                      relies on nbs, and x[i]/y[i] (gf_xy only)
 *
 * For an irregular grid form, only nq and edges (so nbs) need to be specified; distance is computed from nbs:
 * - there is no underlying rectangular grid, so there are no defined x and y coordinates of qubits;
 *   this means that Normalize as needed by mappathselect==borders cannot work
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

enum GridConnectivity {
    gc_specified,   // "specified": edges are specified in "edges" section
    gc_full         // "full": qubits are fully connected by edges, between cores only between comm_qubits
};

enum GridForm {
    gf_xy,          // nodes have explicit neighbor definitions, qubits have explicit x/y coordinates
    gf_irregular    // nodes have explicit neighbor definitions, qubits don't have x/y coordinates
};

class Grid {
private:
    utils::UInt nq;                       // number of qubits in the platform
    utils::UInt ncores;                   // number of cores in the platform
    // Grid configuration, all constant after initialization
    GridForm form;                        // form of grid
    GridConnectivity conn;                // connectivity of grid
    utils::UInt ncommqpc;                 // number of comm_qubits per core, ==nq/ncores when all can communicate
    utils::Int nx;                        // length of x dimension (x coordinates count 0..nx-1)
    utils::Int ny;                        // length of y dimension (y coordinates count 0..ny-1)

    typedef utils::List<utils::UInt> neighbors_t;  // neighbors is a list of qubits
    utils::Map<utils::UInt,neighbors_t> nbs;       // nbs[i] is list of neighbor qubits of qubit i
    utils::Map<utils::UInt,utils::Int> x;          // x[i] is x coordinate of qubit i
    utils::Map<utils::UInt,utils::Int> y;          // y[i] is y coordinate of qubit i
    utils::Vec<utils::Vec<utils::UInt>> dist;      // dist[i][j] is computed distance between qubits i and j;

public:

    // Grid initializer
    // initialize mapper internal grid maps from configuration
    // this remains constant over multiple kernels on the same platform
    Grid(utils::UInt num_qubits, const utils::Json &topology);

    // returns the neighbors for the given qubit
    const utils::List<utils::UInt> &get_neighbors(utils::UInt qubit) const;

    // whether qubit is a communication qubit of a core
    utils::Bool IsCommQubit(utils::UInt qi) const;

    // core index from qubit index
    // when multi-core assumes full and uniform core connectivity
    utils::UInt CoreOf(utils::UInt qi) const;

    // inter-core hop from qs to qt?
    utils::Bool IsInterCoreHop(utils::UInt qs, utils::UInt qt) const;

    // distance between two qubits
    // formulae for convex (hole free) topologies with underlying grid and with bidirectional edges:
    //      gf_cross:   max( abs( x[to_realqi] - x[from_realqi] ), abs( y[to_realqi] - y[from_realqi] ))
    //      gf_plus:    abs( x[to_realqi] - x[from_realqi] ) + abs( y[to_realqi] - y[from_realqi] )
    // when the neighbor relation is defined (topology.edges in config file), Floyd-Warshall is used, which currently is always
    utils::UInt Distance(utils::UInt from_realqi, utils::UInt to_realqi) const;

    // coredistance between two qubits
    // when multi-core assumes full and uniform core connectivity
    utils::UInt CoreDistance(utils::UInt from_realqi, utils::UInt to_realqi) const;

    // minimum number of hops between two qubits is always >= distance(from, to)
    // and inside one core (or without multi-core) the minimum number of hops == distance
    //
    // however, in multi-core with inter-core hops, an inter-core hop cannot execute a 2qgate
    // so when the minimum number of hops are all inter-core hops (so distance(from,to) == coredistance(from,to))
    // and no 2qgate has been placed yet, then at least one additional inter-core hop is needed for the 2qgate,
    // the number of hops required being at least distance+1;
    //
    // we assume below that a valid path exists with distance+1 hops;
    // this fails when not all qubits in a core support connections to all other cores;
    // see the check in InitNbs
    utils::UInt MinHops(utils::UInt from_realqi, utils::UInt to_realqi) const;

    // return clockwise angle around (cx,cy) of (x,y) wrt vertical y axis with angle 0 at 12:00, 0<=angle<2*pi
    utils::Real Angle(utils::Int cx, utils::Int cy, utils::Int x, utils::Int y) const;

    // rotate neighbors list such that largest angle difference between adjacent elements is behind back;
    // this is needed when a given subset of variations from a node is wanted (mappathselect==borders);
    // and this can only be computed when there is an underlying x/y grid (so not for form==gf_irregular)
    void Normalize(utils::UInt src, neighbors_t &nbl) const;

    void DPRINTGrid() const;
    void PrintGrid() const;

};

} // namespace plat
} // namespace ql
