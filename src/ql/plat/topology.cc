/** \file
 * Topology: definition and access functions to the grid of qubits that supports
 * the real qubits.
 *
 * See header file for more information.
 */

#include "ql/plat/topology.h"

#include "ql/utils/logger.h"
#include "ql/com/options.h"

namespace ql {
namespace plat {

/**
 * Returns the clockwise angle of b around a with respect to the positive Y
 * axis, with angle 0 at 12:00, and 0 <= angle < 2*pi.
 */
static utils::Real get_angle(XYCoordinate a, XYCoordinate b) {
    utils::Real ang = std::atan2((b.x - a.x), (b.y - a.y));
    if (ang < 0) ang += 2 * utils::PI;
    return ang;
}

/**
 * String representation for GridForm.
 */
std::ostream &operator<<(std::ostream &os, GridForm gf) {
    switch (gf) {
        case GridForm::XY:        os << "xy";        break;
        case GridForm::IRREGULAR: os << "irregular"; break;
    }
    return os;
}

/**
 * String representation for XYCoordinate.
 */
std::ostream &operator<<(std::ostream &os, XYCoordinate xy) {
    os << "(" << xy.x << ", " << xy.y << ")";
    return os;
}

/**
 * String representation for GridConnectivity.
 */
std::ostream &operator<<(std::ostream &os, GridConnectivity gc) {
    switch (gc) {
        case GridConnectivity::SPECIFIED: os << "specified"; break;
        case GridConnectivity::FULL:      os << "full";      break;
    }
    return os;
}

/**
 * Constructs the grid for the given number of qubits from the given JSON
 * object.
 *
 * See header file for JSON format documentation.
 */
Grid::Grid(utils::UInt num_qubits, const utils::Json &topology) {
    QL_DOUT("Grid::Init");
    //p = p;
    this->num_qubits = num_qubits;
    QL_DOUT("... number of real qbits=" << num_qubits);

    // init grid form attributes
    utils::Str formstr;
    if (topology.count("form") <= 0) {
        if (topology.count("qubits")) {
            formstr = "xy";
        } else {
            formstr = "irregular";
        }
    } else {
        formstr = topology["form"].get<utils::Str>();
    }
    if (formstr == "xy") { form = GridForm::XY; }
    if (formstr == "irregular") { form = GridForm::IRREGULAR; }

    if (form == GridForm::IRREGULAR) {
        // irregular can do without topology.x_size, topology.y_size, and topology.qubits
        xy_size = {0, 0};
    } else {
        // gf_xy have an x/y space; coordinates are explicitly specified
        xy_size.x = topology["x_size"];
        xy_size.y = topology["y_size"];
    }
    QL_DOUT("... formstr=" << formstr << "; form=" << form << "; xy_size=" << xy_size);

    // init multi-core attributes
    if (topology.count("number_of_cores") <= 0) {
        num_cores = 1;
        QL_DOUT("Number of cores (topology[\"number_of_cores\"]) not defined");
    } else {
        num_cores = topology["number_of_cores"];
        if (num_cores <= 0) {
            QL_FATAL("Number of cores (topology[\"number_of_cores\"]) is not a positive value: " << num_cores);
        }
    }
    QL_DOUT("Numer of cores= " << num_cores);

    // when not specified in single-core: == nq (i.e. all qubits)
    // when not specified in multi-core: == nq/ncores (i.e. all qubits of a core)
    if (topology.count("comm_qubits_per_core") <= 0) {
        num_comm_qubits = num_qubits / num_cores;   // i.e. all are comm qubits
        QL_DOUT("Number of comm_qubits per core (topology[\"comm_qubits_per_core\"]) not defined; assuming all are comm qubits.");
    } else {
        num_comm_qubits = topology["comm_qubits_per_core"];
        if (num_comm_qubits <= 0) {
            QL_FATAL("Number of communication qubits per core (topology[\"comm_qubits_per_core\"]) is not a positive value: " << num_comm_qubits);
        }
        if (num_comm_qubits > num_qubits / num_cores) {
            QL_FATAL("Number of communication qubits per core (topology[\"comm_qubits_per_core\"]) is larger than number of qubits per core: " << num_comm_qubits);
        }
    }
    QL_DOUT("Numer of communication qubits per core= " << num_comm_qubits);

    // init x, and y maps
    if (form != GridForm::IRREGULAR) {
        if (topology.count("qubits") == 0) {
            QL_FATAL("Regular configuration doesn't specify qubits and their coordinates");
        } else {
            if (num_qubits != topology["qubits"].size()) {
                QL_FATAL("Mismatch between platform qubit number and qubit coordinate list");
            }
            for (auto &aqbit : topology["qubits"]) {
                utils::UInt qi = aqbit["id"];
                utils::Int qx = aqbit["x"];
                utils::Int qy = aqbit["y"];

                // sanity checks
                if (!(0<=qi && qi < num_qubits)) {
                    QL_FATAL(" qbit in platform topology with id=" << qi << " is configured with id that is not in the range 0..nq-1 with nq=" << num_qubits);
                }
                if (xy_coord.count(qi) > 0) {
                    QL_FATAL(" qbit in platform topology with id=" << qi << ": duplicate definition of coordinate");
                }
                if (qx < 0 || qx >= xy_size.x) {
                    QL_FATAL(" qbit in platform topology with id=" << qi << " is configured with x that is not in the range 0..x_size-1 with x_size=" << xy_size.x);
                }
                if (qy < 0 || qy >= xy_size.y) {
                    QL_FATAL(" qbit in platform topology with id=" << qi << " is configured with y that is not in the range 0..y_size-1 with y_size=" << xy_size.y);
                }

                xy_coord.set(qi) = {qx, qy};
            }
        }
    }

    // nbs[qi], read from topology.edges when connectivity is specified, otherwise, when full, computed
    if (!topology.count("connectivity")) {
        if (topology.count("edges")) {
            QL_DOUT("Configuration doesn't specify topology.connectivity: assuming connectivity is specified by edges section");
            connectivity = GridConnectivity::SPECIFIED;
        } else {
            QL_DOUT("Configuration doesn't specify topology.connectivity, nor does it specify edges; assuming full connectivity");
            connectivity = GridConnectivity::FULL;
        }
    } else {
        utils::Str connstr;
        connstr = topology["connectivity"].get<utils::Str>();
        if (connstr == "specified") {
            connectivity = GridConnectivity::SPECIFIED;
        } else if (connstr == "full") {
            connectivity = GridConnectivity::FULL;
        } else {
            QL_FATAL("connectivity " << connstr << " not supported");
        }
        QL_DOUT("topology.connectivity=" << connstr );
    }
    if (connectivity == GridConnectivity::SPECIFIED) {
        if (topology.count("edges") == 0) {
            QL_FATAL("There aren't edges configured in the platform's topology");
        }
        for (auto &anedge : topology["edges"]) {
            QL_DOUT("connectivity is specified by edges section, reading ...");
            utils::UInt qs = anedge["src"];
            utils::UInt qd = anedge["dst"];

            // sanity checks
            if (!(0<=qs && qs < num_qubits)) {
                QL_FATAL(" edge in platform topology has src=" << qs << " that is not in the range 0..nq-1 with nq=" << num_qubits);
            }
            if (!(0<=qd && qd < num_qubits)) {
                QL_FATAL(" edge in platform topology has dst=" << qd << " that is not in the range 0..nq-1 with nq=" << num_qubits);
            }
            for (auto &n : neighbors.get(qs)) {
                if (n == qd) {
                    QL_FATAL(" redefinition of edge with src=" << qs << " and dst=" << qd);
                }
            }

            neighbors.set(qs).push_back(qd);
            QL_DOUT("connectivity has been stored in nbs map");
        }
    }
    if (connectivity == GridConnectivity::FULL) {
        QL_DOUT("connectivity is full");
        for (utils::UInt qs = 0; qs < num_qubits; qs++) {
            for (utils::UInt qd = 0; qd < num_qubits; qd++) {
                if (qs != qd) {
                    if (is_inter_core_hop(qs, qd) && (!is_comm_qubit(qs) || !is_comm_qubit(qd)) ) {
                        continue;
                    }
                    QL_DOUT("connecting qubit[" << qs << "] to qubit[" << qd << "]");
                    neighbors.set(qs).push_back(qd);
                }
            }
        }
    }

    // when form embedded in grid, sort clock-wise starting from 12:00, to know boundary of search space
    if (form != GridForm::IRREGULAR) {
        // sort neighbor list by angles
        for (utils::UInt qi = 0; qi < num_qubits; qi++) {
            // sort nbs[qi] to have increasing clockwise angles around qi, starting with angle 0 at 12:00
            auto nbsq = neighbors.find(qi);
            if (nbsq != neighbors.end()) {
                nbsq->second.sort(
                    [this, qi](const utils::UInt &i, const utils::UInt &j) {
                        return get_angle(xy_coord.at(qi), xy_coord.at(i)) <
                               get_angle(xy_coord.at(qi), xy_coord.at(j));
                    }
                );
            }
        }
    }

    // Floyd-Warshall dist[i][j] = shortest distances between all nq qubits i and j
    // when not connected, distance remains maximum value

    // initialize all distances to maximum value, to neighbors to 1, to itself to 0
    distance.resize(num_qubits);
    for (utils::UInt i = 0; i < num_qubits; i++) {
        distance[i].resize(num_qubits, utils::MAX); // NOTE: /2 to prevent overflow in addition
        distance[i][i] = 0;
        for (utils::UInt j : neighbors.get(i)) {
            distance[i][j] = 1;
        }
    }

    // find shorter distances by gradually including more qubits (k) in path
    for (utils::UInt k = 0; k < num_qubits; k++) {
        for (utils::UInt i = 0; i < num_qubits; i++) {
            for (utils::UInt j = 0; j < num_qubits; j++) {
                if (distance[i][j] > distance[i][k] + distance[k][j]) {
                    distance[i][j] = distance[i][k] + distance[k][j];
                }
            }
        }
    }

    QL_IF_LOG_DEBUG {
        dump();
    }
}

/**
 * Returns the indices of the neighboring qubits for the given qubit.
 */
const Grid::Neighbors &Grid::get_neighbors(Qubit qubit) const {
    return neighbors.get(qubit);
}

/**
 * Returns whether the given qubit is a communication qubit of a core.
 */
utils::Bool Grid::is_comm_qubit(Qubit qubit) const {
    if (num_cores == 1) return true;
    QL_ASSERT(connectivity == GridConnectivity::FULL);
    utils::UInt qci = qubit % num_cores;   // index of qubit local to core
    return qci < num_comm_qubits;  // 0..ncommqpc-1 are comm qubits, ncommqpc..nq/ncores-1 are not comm qubits
}

/**
 * Returns the core index for the given qubit in a multi-core environment.
 */
utils::UInt Grid::get_core_index(Qubit qubit) const {
    if (num_cores == 1) return 1;
    QL_ASSERT(connectivity == GridConnectivity::FULL);
    utils::UInt nqpc = num_qubits / num_cores;
    return qubit / nqpc;
}

/**
 * Returns whether communication between the given two qubits involves
 * inter-core communication.
 */
utils::Bool Grid::is_inter_core_hop(Qubit source, Qubit target) const {
    return get_core_index(source) != get_core_index(target);
}

/**
 * Returns the distance between the two given qubits in number of hops.
 * Returns 0 iff source == target.
 */
utils::UInt Grid::get_distance(Qubit source, Qubit target) const {
    return distance[source][target];
}

/**
 * Returns the distance between the given two qubits in terms of cores.
 */
utils::UInt Grid::get_core_distance(Qubit source, Qubit target) const {
    if (get_core_index(source) == get_core_index(target)) return 0;
    QL_ASSERT(connectivity == GridConnectivity::FULL);
    return 1;
}

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
utils::UInt Grid::get_min_hops(Qubit source, Qubit target) const {
    utils::UInt d = get_distance(source, target);
    utils::UInt cd = get_core_distance(source, target);
    QL_ASSERT(cd <= d);
    if (cd == d) {
        return d+2;
    } else {
        return d;
    }
}

/**
 * Rotate neighbors list such that largest angle difference between adjacent
 * elements is behind back. This is needed when a given subset of variations
 * from a node is wanted (mappathselect==borders). This can only be computed
 * when there is an underlying x/y grid (so not for form==gf_irregular).
 *
 * TODO JvS: does this even belong in grid now that it's not part of the
 *  mapper anymore? It feels like a very specific thing.
 */
void Grid::sort_neighbors_by_angle(Qubit src, Neighbors &nbl) const {
    if (form != GridForm::XY) {
        // there are no implicit/explicit x/y coordinates defined per qubit, so no sense of nearness
        utils::Str mappathselectopt = com::options::get("mappathselect");
        QL_ASSERT(mappathselectopt != "borders");
        return;
    }

    // std::cout << "Normalizing list from src=" << src << ": ";
    // for (auto dn : nbl) { std::cout << dn << " "; } std::cout << std::endl;

    const utils::Real pi = 4 * std::atan(1);
    if (nbl.size() == 1) {
        // QL_DOUT("... size was 1; unchanged");
        return;
    }

    // find maxinx index in neighbor list before which largest angle difference occurs
    utils::Int maxdiff = 0;                            // current maximum angle difference in loop search below
    auto maxinx = nbl.begin(); // before which max diff occurs

    // for all indices in and its next one inx compute angle difference and find largest of these
    for (auto in = nbl.begin(); in != nbl.end(); in++) {
        utils::Real a_in = get_angle(xy_coord.at(src), xy_coord.at(*in));

        auto inx = std::next(in); if (inx == nbl.end()) inx = nbl.begin();
        utils::Real a_inx = get_angle(xy_coord.at(src), xy_coord.at(*inx));

        utils::Int diff = a_inx - a_in; if (diff < 0) diff += 2*pi;
        if (diff > maxdiff) {
            maxdiff = diff;
            maxinx = inx;
        }
    }

    // and now rotate neighbor list so that largest angle difference is behind last one
    Neighbors newnbl;
    for (auto in = maxinx; in != nbl.end(); in++) {
        newnbl.push_back(*in);
    }
    for (auto in = nbl.begin(); in != maxinx; in++) {
        newnbl.push_back(*in);
    }
    nbl = newnbl;

    // std::cout << "... rotated; result: ";
    // for (auto dn : nbl) { std::cout << dn << " "; } std::cout << std::endl;
}

/**
 * Dumps the grid configuration to the given stream.
 */
void Grid::dump(std::ostream &os, const utils::Str &line_prefix) const {
    os << line_prefix << "grid form = " << form << "\n";
    for (utils::UInt i = 0; i < num_qubits; i++) {
        os << line_prefix << "qubit[" << i << "]=" << xy_coord.dbg(i);
        os << " has neighbors";
        for (auto &n : neighbors.get(i)) {
            os << " qubit[" << n << "]=" << xy_coord.dbg(i);
        }
        os << "\n";
    }
    for (utils::UInt i = 0; i < num_qubits; i++) {
        os << line_prefix << "qubit[" << i << "] distance(" << i << ",j)=";
        for (utils::UInt j = 0; j < num_qubits; j++) {
            os << get_distance(i, j) << " ";
        }
        os << "\n";
    }
    for (utils::UInt i = 0; i < num_qubits; i++) {
        os << line_prefix << "qubit[" << i << "] minhops(" << i << ",j)=";
        for (utils::UInt j = 0; j < num_qubits; j++) {
            os << get_min_hops(i, j) << " ";
        }
        os << "\n";
    }
}

} // namespace plat
} // namespace ql
