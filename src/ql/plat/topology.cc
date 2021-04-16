/** \file
 * Definition and access functions to the grid of qubits that supports the real
 * qubits.
 */

#include "ql/plat/topology.h"

#include "ql/utils/logger.h"

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

    // Shorthand.
    using JsonType = utils::Json::value_t;

    // Save number of qubits.
    this->num_qubits = num_qubits;

    // Handle grid form key.
    auto it = topology.find("form");
    if (it == topology.end()) {
        if (topology.find("qubits") != topology.end()) {
            form = GridForm::XY;
        } else {
            form = GridForm::IRREGULAR;
        }
    } else if (it->type() != JsonType::string) {
        throw utils::Exception("topology.form key must be a string if specified");
    } else if (it->get<utils::Str>() == "xy") {
        form = GridForm::XY;
    } else if (it->get<utils::Str>() == "irregular") {
        form = GridForm::IRREGULAR;
    } else {
        throw utils::Exception("topology.form key must be either \"xy\" or \"irregular\" if specified");
    }

    // Handle XY grid keys.
    xy_size = {0, 0};
    if (form != GridForm::IRREGULAR) {

        // Handle X size.
        it = topology.find("x_size");
        if (it == topology.end()) {
            xy_size.x = 0;
        } else if (it->type() != JsonType::number_unsigned) {
            throw utils::Exception("topology.x_size key must be an unsigned integer if specified");
        } else {
            xy_size.x = it->get<utils::UInt>();
        }

        // Handle Y size.
        it = topology.find("y_size");
        if (it == topology.end()) {
            xy_size.y = 0;
        } else if (it->type() != JsonType::number_unsigned) {
            throw utils::Exception("topology.y_size key must be an unsigned integer if specified");
        } else {
            xy_size.y = it->get<utils::UInt>();
        }

        // Handle qubits.
        it = topology.find("qubits");
        if (it == topology.end()) {
            throw utils::Exception("topology.qubits is missing while topology.form explicitly requires XY mode");
        } else if (it->type() != JsonType::array) {
            throw utils::Exception("topology.qubits key must be an array of objects if specified");
        } else {
            for (const auto &qubit : *it) {
                if (qubit.type() != JsonType::object) {
                    throw utils::Exception("topology.qubits entries must be objects");
                }

                // Read ID.
                utils::UInt id;
                auto it2 = qubit.find("id");
                if (it2 == qubit.end()) {
                    throw utils::Exception("topology.qubits.*.id must be specified");
                } else if (it2->type() != JsonType::number_unsigned) {
                    throw utils::Exception("topology.qubits.*.id must be an unsigned integer");
                } else {
                    id = it2->get<utils::UInt>();
                }
                if (id >= num_qubits) {
                    throw utils::Exception("topology.qubits.*.id is out of range");
                } else if (xy_coord.count(id) > 0) {
                    throw utils::Exception("topology.qubits has multiple entries for qubit " + utils::to_string(id));
                }

                // Read X coordinate.
                utils::Int x;
                it2 = qubit.find("x");
                if (it2 == qubit.end()) {
                    throw utils::Exception("topology.qubits.*.x must be specified");
                } else if (it2->type() != JsonType::number_unsigned) {
                    throw utils::Exception("topology.qubits.*.x must be an unsigned integer");
                } else {
                    x = it2->get<utils::Int>();
                }
                if (x < 0) {
                    throw utils::Exception("topology.qubits.*.x cannot be negative");
                }
                if (xy_size.x > 0) {
                    if (x >= xy_size.x) {
                        throw utils::Exception("topology.qubits.*.x is out of range");
                    } else {
                        xy_size.x = utils::max(xy_size.x, x);
                    }
                }

                // Read Y coordinate.
                utils::Int y;
                it2 = qubit.find("y");
                if (it2 == qubit.end()) {
                    throw utils::Exception("topology.qubits.*.y must be specified");
                } else if (it2->type() != JsonType::number_unsigned) {
                    throw utils::Exception("topology.qubits.*.y must be an unsigned integer");
                } else {
                    y = it2->get<utils::Int>();
                }
                if (y < 0) {
                    throw utils::Exception("topology.qubits.*.y cannot be negative");
                }
                if (xy_size.y > 0) {
                    if (y >= xy_size.y) {
                        throw utils::Exception("topology.qubits.*.y is out of range");
                    } else {
                        xy_size.y = utils::max(xy_size.y, y);
                    }
                }

                // Save the position.
                xy_coord.set(id) = {x, y};

            }
        }
    }

    // Handle number of cores.
    it = topology.find("number_of_cores");
    if (it == topology.end()) {
        num_cores = 1;
    } else if (it->type() != JsonType::number_unsigned) {
        throw utils::Exception("topology.number_of_cores key must be an unsigned integer if specified");
    } else {
        num_cores = it->get<utils::UInt>();
    }
    if (num_cores < 1) {
        throw utils::Exception("topology.number_of_cores must be a positive integer");
    } else if (num_qubits % num_cores) {
        throw utils::Exception("number of qubits is not divisible by topology.number_of_cores");
    }

    // Handle number of communication qubits per core.
    it = topology.find("comm_qubits_per_core");
    if (it == topology.end()) {
        num_comm_qubits = num_qubits / num_cores;
    } else if (it->type() != JsonType::number_unsigned) {
        throw utils::Exception("topology.comm_qubits_per_core key must be an unsigned integer if specified");
    } else {
        num_comm_qubits = it->get<utils::UInt>();
    }
    if (num_comm_qubits < 1) {
        throw utils::Exception("topology.comm_qubits_per_core must be a positive integer");
    } else if (num_comm_qubits > num_qubits / num_cores) {
        throw utils::Exception("topology.comm_qubits_per_core is larger than total number of qubits per core");
    }

    // Handle connectivity key.
    it = topology.find("connectivity");
    if (it == topology.end()) {
        if (topology.find("edges") != topology.end()) {
            connectivity = GridConnectivity::SPECIFIED;
        } else {
            connectivity = GridConnectivity::FULL;
        }
    } else if (it->type() != JsonType::string) {
        throw utils::Exception("topology.connectivity key must be a string if specified");
    } else if (it->get<utils::Str>() == "specified") {
        connectivity = GridConnectivity::SPECIFIED;
    } else if (it->get<utils::Str>() == "full") {
        connectivity = GridConnectivity::FULL;
    } else {
        throw utils::Exception("topology.connectivity key must be either \"specified\" or \"full\" if specified");
    }

    // Handle edges.
    if (connectivity == GridConnectivity::SPECIFIED) {

        // Parse connectivity from JSON.
        it = topology.find("edges");
        if (it == topology.end()) {
            throw utils::Exception("topology.edges is missing while topology.connectivity explicitly requires it");
        } else if (it->type() != JsonType::array) {
            throw utils::Exception("topology.edges key must be an array of objects if specified");
        } else {
            for (const auto &edge : *it) {
                if (edge.type() != JsonType::object) {
                    throw utils::Exception("topology.edges entries must be objects");
                }

                // Read source ID.
                utils::UInt src;
                auto it2 = edge.find("src");
                if (it2 == edge.end()) {
                    throw utils::Exception("topology.edges.*.src must be specified");
                } else if (it2->type() != JsonType::number_unsigned) {
                    throw utils::Exception("topology.edges.*.src must be an unsigned integer");
                } else {
                    src = it2->get<utils::UInt>();
                }
                if (src >= num_qubits) {
                    throw utils::Exception("topology.edges.*.src is out of range");
                }

                // Read destination ID.
                utils::UInt dst;
                it2 = edge.find("dst");
                if (it2 == edge.end()) {
                    throw utils::Exception("topology.edges.*.dst must be specified");
                } else if (it2->type() != JsonType::number_unsigned) {
                    throw utils::Exception("topology.edges.*.dst must be an unsigned integer");
                } else {
                    dst = it2->get<utils::UInt>();
                }
                if (dst >= num_qubits) {
                    throw utils::Exception("topology.edges.*.dst is out of range");
                }

                // Check uniqueness and add.
                for (auto neighbor : neighbors.get(src)) {
                    if (neighbor == dst) {
                        throw utils::Exception(
                            "redefinition of edge with src=" + utils::to_string(src) +
                            " and dst=" + utils::to_string(dst)
                        );
                    }
                }
                neighbors.set(src).push_back(dst);

            }
        }

    } else if (connectivity == GridConnectivity::FULL) {

        // Generate full connectivity.
        for (utils::UInt qs = 0; qs < num_qubits; qs++) {
            for (utils::UInt qd = 0; qd < num_qubits; qd++) {
                if (qs == qd) {
                    continue;
                }
                if (is_inter_core_hop(qs, qd) && (!is_comm_qubit(qs) || !is_comm_qubit(qd))) {
                    continue;
                }
                neighbors.set(qs).push_back(qd);
            }
        }

    }

    // When qubits have coordinates, sort neighbor lists clockwise starting from
    // 12:00, to know boundary of search space.
    if (has_coordinates()) {
        for (utils::UInt qi = 0; qi < num_qubits; qi++) {
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

    // Compute distances between all qubits using Floyd-Warshall. When not
    // connected, distance remains set to utils::MAX.
    distance.resize(num_qubits);
    for (utils::UInt i = 0; i < num_qubits; i++) {

        // Initialize all distances to maximum value...
        distance[i].resize(num_qubits, utils::MAX);

        // ... except the self-edge, which is 0 distance...
        distance[i][i] = 0;

        // ... and the neighbors, which get distance 1.
        for (utils::UInt j : neighbors.get(i)) {
            distance[i][j] = 1;
        }

    }

    // Find shorter distances by gradually including more qubits (k) in path
    for (utils::UInt k = 0; k < num_qubits; k++) {
        for (utils::UInt i = 0; i < num_qubits; i++) {
            for (utils::UInt j = 0; j < num_qubits; j++) {

                // Prevent overflow in the sum below by explicitly checking for
                // MAX.
                if (distance[i][k] == utils::MAX) {
                    continue;
                }
                if (distance[k][j] == utils::MAX) {
                    continue;
                }

                if (distance[i][j] > distance[i][k] + distance[k][j]) {
                    distance[i][j] = distance[i][k] + distance[k][j];
                }
            }
        }
    }

    // Dump the grid structure to stdout if the loglevel is sufficiently
    // verbose.
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

    // Compute index of qubit local to core.
    utils::UInt qci = qubit % num_cores;

    // 0..ncommqpc-1 are comm qubits, ncommqpc..nq/ncores-1 are not comm qubits.
    return qci < num_comm_qubits;
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
 * Returns whether qubits have coordinates associated with them.
 */
utils::Bool Grid::has_coordinates() const {
    return form != GridForm::IRREGULAR;
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
