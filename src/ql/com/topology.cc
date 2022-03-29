/** \file
 * Definition and access functions to the grid of qubits that supports the real
 * qubits.
 */

#include "ql/com/topology.h"

#include "ql/utils/logger.h"

// uncomment next line to enable multi-line dumping
// #define MULTI_LINE_LOG_DEBUG

namespace ql {
namespace com {

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
 * Dumps the documentation for the topology JSON structure.
 */
void Topology::dump_docs(std::ostream &os, const utils::Str &line_prefix) {
    utils::dump_str(os, line_prefix, R"(
    The topology JSON object must have the following structure.

    ```
    {
        "form": <optional string, either "xy" or "irregular">,
        "x_size": <optional integer for form="xy">,
        "y_size": <optional integer for form="xy">,
        "qubits": <mandatory array of objects for form="xy", unused for "irregular">,
        "number_of_cores": <optional positive integer, default 1>,
        "comm_qubits_per_core": <optional positive integer, num_qubits / number_of_cores>,
        "connectivity": <optional string, either "specified" or "full">,
        "edges": <mandatory array of objects for connectivity="specified", unused for "full">
        ...
    }
    ```

    The `"form"` key specifies whether the qubits can be arranged in a 2D grid
    of integer coordinates (`"xy"`) or not (`"irregular"`). If irregular, mapper
    heuristics that rely on sorting possible paths by angle are unavailable.
    If `"xy"`, `"x_size"` and `"y_size"` specify the coordinate ranges (from
    zero to the limit minus one), and `"qubits"` specifies the coordinates.
    `"qubits"` must then be an array of objects of the following form:

    ```
    {
        "id": <qubit index, mandatory>,
        "x": <X coordinate, mandatory>,
        "y": <Y coordinate, mandatory>,
        ...
    }
    ```

    Each qubit must be specified exactly once. Any additional keys in the
    object are silently ignored, as other parts of OpenQL may use the
    structure as well.

    If the `"form"` key is missing, its value is derived from whether a
    `"qubits"` list is given. If `"x_size"` or `"y_size"` are missing, the
    values are inferred from the largest coordinate found in `"qubits"`.
)" R"(
    The `"number_of_cores"` key is used to specify multi-core architectures.
    It must be a positive integer. Each core is assumed to have the same
    number of qubits, so the total number of qubits must be divisible by this
    number. The first N qubits belong to core 0, the next N belong to core 1,
    etc, where N equals the total number of qubits divided by the number of
    cores.

    Cores can communicate only via communication qubits. The amount of these
    qubits per cores may be set using the `"comm_qubits_per_core"` key. Its
    value must range between 1 and the number of qubits per core, and
    defaults to the latter. The first N qubits for each core are considered
    to be communication qubits, whereas the remainder are local qubits.

    The `"connectivity"` key specifies whether there are qubit connectivity
    constraints (`"specified"`) or all qubits (within a core) are connected
    (`"full"`). In the former case, the `"edges"` key must map to an array of
    objects of the following form:

    ```
    {
        "id": <optional unique identifying integer>,
        "src": <source qubit index, mandatory>,
        "dst": <target qubit index, mandatory>,
        ...
    }
    ```

    Edges are directional; to allow qubits to interact "in both ways," both
    directions must be specified. If any identifiers are specified, all edges
    should get one, and they should all be unique; otherwise, indices are
    generated using src*nq+dst. Any additional keys in the object are
    silently ignored, as other parts of OpenQL may use the structure as well
    (although they should preferably just extend this class).

    When `"connectivity"` is set to `"full"` in a multi-core environment,
    inter-core edges are only generated when both the source and destination
    qubit is a communication qubit.

    If the `"connectivity"` key is missing, its value is derived from whether
    an "edges" list is given.

    Any additional keys in the topology root object are silently ignored, as
    other parts of OpenQL may use the structure as well.
    )");
}

/**
 * Generates the neighbor list for the given qubit for full connectivity.
 */
void Topology::generate_neighbors_list(utils::UInt qs, Neighbors &qubits) const {
    QL_ASSERT(connectivity == GridConnectivity::FULL);

    // Generate neighbors for qubit qs for full connectivity per core.
    for (utils::UInt qd = 0; qd < num_qubits; qd++) {
        if (qs == qd) {
            continue;
        }
        if (is_inter_core_hop(qs, qd) && (!is_comm_qubit(qs) || !is_comm_qubit(qd))) {
            continue;
        }
        qubits.push_back(qd);
    }
}

/**
 * Constructs the grid for the given number of qubits from the given JSON
 * object. Refer to dump_docs() for details.
 */
Topology::Topology(utils::UInt num_qubits, const utils::Json &topology) {

    // Shorthand.
    using JsonType = utils::Json::value_t;

    // Save number of qubits and original JSON.
    this->num_qubits = num_qubits;
    this->json = topology;

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
                if (xy_size.x > 0 && x >= xy_size.x) {
                    throw utils::Exception("topology.qubits.*.x is out of range");
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
                if (xy_size.y > 0 && y >= xy_size.y) {
                    throw utils::Exception("topology.qubits.*.y is out of range");
                }

                // Save the position.
                xy_coord.set(id) = {x, y};

            }
        }

        // If x_size and y_size were not configured, compute them.
        if (xy_size.x == 0) {
            for (const auto &coord : xy_coord) {
                xy_size.x = utils::max(xy_size.x, coord.second.x + 1);
            }
        }
        if (xy_size.y == 0) {
            for (const auto &coord : xy_coord) {
                xy_size.y = utils::max(xy_size.y, coord.second.y + 1);
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
    max_edge = 0;
    if (connectivity == GridConnectivity::SPECIFIED) {

        // Parse connectivity from JSON.
        it = topology.find("edges");
        if (it == topology.end()) {
            throw utils::Exception("topology.edges is missing while topology.connectivity explicitly requires it");
        } else if (it->type() != JsonType::array) {
            throw utils::Exception("topology.edges key must be an array of objects if specified");
        } else {
            utils::Bool first_edge = true;
            utils::Bool edges_have_ids = false;
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

                // Check uniqueness and add to neighbors lookup.
                for (auto neighbor : neighbors.get(src)) {
                    if (neighbor == dst) {
                        throw utils::Exception(
                            "redefinition of edge with src=" + utils::to_string(src) +
                            " and dst=" + utils::to_string(dst)
                        );
                    }
                }
                neighbors.set(src).push_back(dst);

                // Read ID.
                auto it3 = edge.find("id");
                if (first_edge) {
                    edges_have_ids = it3 != edge.end();
                    first_edge = false;
                }
                if (edges_have_ids != (it3 != edge.end())) {
                    throw utils::Exception("topology.edges.*.id must be specified for all or none of the edges");
                } else if (it3 != edge.end()) {
                    if (it3->type() != JsonType::number_unsigned) {
                        throw utils::Exception("topology.edges.*.id must be an unsigned integer if specified");
                    }
                    Edge id = it3->get<utils::Int>();
                    if (edge_to_qubits.find(id) != edge_to_qubits.end()) {
                        throw utils::Exception("topology.edges.*.id is not unique (" + utils::to_string(id) + ")");
                    }
                    edge_to_qubits.set(id) = {src, dst};
                    qubits_to_edge.set({src, dst}) = id;
                    max_edge = utils::max(max_edge, id + 1);
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

    } else if (connectivity == GridConnectivity::FULL) {

        // If we have full connectivity and the qubits have coordinates, we
        // want neighbor lists for each qubit sorted based on angle. So, in this
        // case, we should generate the neighbors list. If not, we can easily
        // figure out neighbors on-the-fly.
        if (has_coordinates()) {

            // Pre-generate full connectivity.
            for (utils::UInt qs = 0; qs < num_qubits; qs++) {
                generate_neighbors_list(qs, neighbors.set(qs));
            }

        }

    }
    if (max_edge == 0) {

        // Full connectivity (within a core) or no edge indices specified by
        // the user. In this case, qubit edge indices are generated on-the-fly
        // using src*nq + dst, so max_edge is simply num_qubits**2.
        // edge_to_qubits and qubits_to_edge can stay empty.
        max_edge = num_qubits * num_qubits;

    }

    // When qubits have coordinates, sort neighbor lists clockwise starting from
    // 12:00, to know boundary of search space.
    if (has_coordinates()) {
        for (auto &it : neighbors) {
            it.second.sort(
                [this, it](const utils::UInt &i, const utils::UInt &j) {
                    return get_angle(xy_coord.at(it.first), xy_coord.at(i)) <
                           get_angle(xy_coord.at(it.first), xy_coord.at(j));
                }
            );
        }
    }

#ifdef MULTI_LINE_LOG_DEBUG
    QL_IF_LOG_DEBUG {
        QL_DOUT("Dump the grid structure to stdout ...");
        dump();
    }
#else
    QL_DOUT("Dump the grid structure to stdout (disabled)");
#endif
}

/**
 * Returns the number of qubits for this topology.
 */
utils::UInt Topology::get_num_qubits() const {
    return num_qubits;
}

/**
 * Returns the JSON that was used to construct this topology. This is used
 * for serialization/deserialization of the IR.
 */
const utils::Json &Topology::get_json() const {
    return json;
}

/**
 * Returns the size of the qubit grid, if coordinates have been specified.
 * If not, this returns (0, 0).
 */
XYCoordinate Topology::get_grid_size() const {
    return xy_size;
}

/**
 * Returns the coordinate of the given qubit, if coordinates have been
 * specified. If not, or if the qubit index is out of range, this returns
 * (0, 0).
 */
XYCoordinate Topology::get_qubit_coordinate(Qubit q) const {
    return xy_coord.get(q, {0, 0});
}

/**
 * Returns the edge index for the given qubit pair, or returns -1 when there
 * is no defined edge index for the given qubit pair.
 */
Topology::Edge Topology::get_edge_index(QubitPair qs) const {
    if (qubits_to_edge.empty()) {
        if (get_distance(qs.first, qs.second) != 1) {
            return -1;
        } else {
            return qs.first * num_qubits + qs.second;
        }
    } else {
        auto it = qubits_to_edge.find(qs);
        if (it == qubits_to_edge.end()) {
            return -1;
        } else {
            return it->second;
        }
    }
}

/**
 * Returns the qubit pair corresponding with the given edge, or returns 0,0
 * when there is no edge with the given index.
 */
Topology::QubitPair Topology::get_edge_qubits(Edge edge) const {
    if (edge < 0) {
        return {0, 0};
    } else if (edge_to_qubits.empty()) {
        if ((utils::UInt)edge >= num_qubits * num_qubits) {
            return {0, 0};
        } else {
            return {edge / num_qubits, edge % num_qubits};
        }
    } else {
        auto it = edge_to_qubits.find((utils::UInt)edge);
        if (it == edge_to_qubits.end()) {
            return {0, 0};
        } else {
            return it->second;
        }
    }
}

/**
 * Returns the highest used edge index plus one. Note that not all edge
 * indices between 0 and max-1 actually need to be in use, so this is not
 * necessarily the total number of edges.
 */
Topology::Edge Topology::get_max_edge() const {
    return max_edge;
}

/**
 * Returns the number of cores.
 */
utils::UInt Topology::get_num_cores() const {
    return num_cores;
}

/**
 * Returns the indices of the neighboring qubits for the given qubit.
 */
Topology::Neighbors Topology::get_neighbors(Qubit qubit) const {
    if (connectivity != GridConnectivity::FULL || has_coordinates()) {
        return neighbors.get(qubit);
    } else {
        Neighbors retval;
        generate_neighbors_list(qubit, retval);
        return retval;
    }
}

/**
 * Get the conectivity
 */
GridConnectivity Topology::get_connectivity(){
    return connectivity;
}


/**
 * Returns whether the given qubit is a communication qubit of a core.
 */
utils::Bool Topology::is_comm_qubit(Qubit qubit) const {
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
utils::UInt Topology::get_core_index(Qubit qubit) const {
    if (num_cores == 1) return 0;
    QL_ASSERT(connectivity == GridConnectivity::FULL);
    utils::UInt nqpc = num_qubits / num_cores;
    return qubit / nqpc;
}

/**
 * Returns whether communication between the given two qubits involves
 * inter-core communication.
 */
utils::Bool Topology::is_inter_core_hop(Qubit source, Qubit target) const {
    return get_core_index(source) != get_core_index(target);
}

/**
 * Returns the distance between the two given qubits in number of hops.
 * Returns 0 iff source == target.
 */
utils::UInt Topology::get_distance(Qubit source, Qubit target) const {
    if (connectivity == GridConnectivity::FULL) {
        if (source == target) {
            return 0;
        }
        utils::UInt d = 1;
        if (get_core_index(source) == get_core_index(target)) {
            return d;
        }
        if (!is_comm_qubit(source)) {
            d++;
        }
        if (!is_comm_qubit(target)) {
            d++;
        }
        return d;
    }

    return distance[source][target];
}

/**
 * Returns the distance between the given two qubits in terms of cores.
 */
utils::UInt Topology::get_core_distance(Qubit source, Qubit target) const {
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
utils::UInt Topology::get_min_hops(Qubit source, Qubit target) const {
    utils::UInt d = get_distance(source, target);
    utils::UInt cd = get_core_distance(source, target);
    QL_ASSERT(cd <= d);
    if (connectivity == GridConnectivity::FULL || cd != d){
        return d;
    } else {
        return d+2;
    }
}

/**
 * Returns whether qubits have coordinates associated with them.
 */
utils::Bool Topology::has_coordinates() const {
    return form != GridForm::IRREGULAR;
}

/**
 * Rotate neighbors list such that largest angle difference between adjacent
 * elements is behind back. This is needed when a given subset of variations
 * from a node is wanted (mappathselect==borders). This can only be computed
 * when there is an underlying x/y grid (so not for form==gf_irregular).
 *
 * TODO/FIXME:
 *  see https://github.com/QuTech-Delft/OpenQL/pull/405#issuecomment-831247204
 */
void Topology::sort_neighbors_by_angle(Qubit src, Neighbors &nbl) const {
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
void Topology::dump(std::ostream &os, const utils::Str &line_prefix) const {
    os << line_prefix << "grid form = " << form << "\n";
    for (utils::UInt i = 0; i < num_qubits; i++) {
        os << line_prefix << "qubit[" << i << "]=" << xy_coord.dbg(i);
        os << " has neighbors";
        for (auto &n : get_neighbors(i)) {
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

} // namespace com
} // namespace ql
