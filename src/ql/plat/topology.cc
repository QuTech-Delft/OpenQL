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

using namespace utils;

// Grid initializer
// initialize mapper internal grid maps from configuration
// this remains constant over multiple kernels on the same platform
Grid::Grid(utils::UInt num_qubits, const utils::Json &topology) {
    QL_DOUT("Grid::Init");
    //p = p;
    nq = num_qubits;
    QL_DOUT("... number of real qbits=" << nq);

    // init grid form attributes
    Str formstr;
    if (topology.count("form") <= 0) {
        if (topology.count("qubits")) {
            formstr = "xy";
        } else {
            formstr = "irregular";
        }
    } else {
        formstr = topology["form"].get<Str>();
    }
    if (formstr == "xy") { form = gf_xy; }
    if (formstr == "irregular") { form = gf_irregular; }

    if (form == gf_irregular) {
        // irregular can do without topology.x_size, topology.y_size, and topology.qubits
        nx = 0;
        ny = 0;
    } else {
        // gf_xy have an x/y space; coordinates are explicitly specified
        nx = topology["x_size"];
        ny = topology["y_size"];
    }
    QL_DOUT("... formstr=" << formstr << "; form=" << form << "; nx=" << nx << "; ny=" << ny);

    // init multi-core attributes
    if (topology.count("number_of_cores") <= 0) {
        ncores = 1;
        QL_DOUT("Number of cores (topology[\"number_of_cores\"]) not defined");
    } else {
        ncores = topology["number_of_cores"];
        if (ncores <= 0) {
            QL_FATAL("Number of cores (topology[\"number_of_cores\"]) is not a positive value: " << ncores);
        }
    }
    QL_DOUT("Numer of cores= " << ncores);

    // when not specified in single-core: == nq (i.e. all qubits)
    // when not specified in multi-core: == nq/ncores (i.e. all qubits of a core)
    if (topology.count("comm_qubits_per_core") <= 0) {
        ncommqpc = nq/ncores;   // i.e. all are comm qubits
        QL_DOUT("Number of comm_qubits per core (topology[\"comm_qubits_per_core\"]) not defined; assuming all are comm qubits.");
    } else {
        ncommqpc = topology["comm_qubits_per_core"];
        if (ncommqpc <= 0) {
            QL_FATAL("Number of communication qubits per core (topology[\"comm_qubits_per_core\"]) is not a positive value: " << ncommqpc);
        }
        if (ncommqpc > nq/ncores) {
            QL_FATAL("Number of communication qubits per core (topology[\"comm_qubits_per_core\"]) is larger than number of qubits per core: " << ncommqpc);
        }
    }
    QL_DOUT("Numer of communication qubits per core= " << ncommqpc);

    // init x, and y maps
    if (form != gf_irregular) {
        if (topology.count("qubits") == 0) {
            QL_FATAL("Regular configuration doesn't specify qubits and their coordinates");
        } else {
            if (nq != topology["qubits"].size()) {
                QL_FATAL("Mismatch between platform qubit number and qubit coordinate list");
            }
            for (auto &aqbit : topology["qubits"]) {
                UInt qi = aqbit["id"];
                Int qx = aqbit["x"];
                Int qy = aqbit["y"];

                // sanity checks
                if (!(0<=qi && qi<nq)) {
                    QL_FATAL(" qbit in platform topology with id=" << qi << " is configured with id that is not in the range 0..nq-1 with nq=" << nq);
                }
                if (x.count(qi) > 0) {
                    QL_FATAL(" qbit in platform topology with id=" << qi << ": duplicate definition of x coordinate");
                }
                if (y.count(qi) > 0) {
                    QL_FATAL(" qbit in platform topology with id=" << qi << ": duplicate definition of y coordinate");
                }
                if (!(0<=qx && qx<nx)) {
                    QL_FATAL(" qbit in platform topology with id=" << qi << " is configured with x that is not in the range 0..x_size-1 with x_size=" << nx);
                }
                if (!(0<=qy && qy<ny)) {
                    QL_FATAL(" qbit in platform topology with id=" << qi << " is configured with y that is not in the range 0..y_size-1 with y_size=" << ny);
                }

                x.set(qi) = qx;
                y.set(qi) = qy;
            }
        }
    }

    // nbs[qi], read from topology.edges when connectivity is specified, otherwise, when full, computed
    if (!topology.count("connectivity")) {
        if (topology.count("edges")) {
            QL_DOUT("Configuration doesn't specify topology.connectivity: assuming connectivity is specified by edges section");
            conn = gc_specified;
        } else {
            QL_DOUT("Configuration doesn't specify topology.connectivity, nor does it specify edges; assuming full connectivity");
            conn = gc_full;
        }
    } else {
        Str connstr;
        connstr = topology["connectivity"].get<Str>();
        if (connstr == "specified") {
            conn = gc_specified;
        } else if (connstr == "full") {
            conn = gc_full;
        } else {
            QL_FATAL("connectivity " << connstr << " not supported");
        }
        QL_DOUT("topology.connectivity=" << connstr );
    }
    if (conn == gc_specified) {
        if (topology.count("edges") == 0) {
            QL_FATAL(" There aren't edges configured in the platform's topology");
        }
        for (auto &anedge : topology["edges"]) {
            QL_DOUT("connectivity is specified by edges section, reading ...");
            UInt qs = anedge["src"];
            UInt qd = anedge["dst"];

            // sanity checks
            if (!(0<=qs && qs<nq)) {
                QL_FATAL(" edge in platform topology has src=" << qs << " that is not in the range 0..nq-1 with nq=" << nq);
            }
            if (!(0<=qd && qd<nq)) {
                QL_FATAL(" edge in platform topology has dst=" << qd << " that is not in the range 0..nq-1 with nq=" << nq);
            }
            for (auto &n : nbs.get(qs)) {
                if (n == qd) {
                    QL_FATAL(" redefinition of edge with src=" << qs << " and dst=" << qd);
                }
            }

            nbs.set(qs).push_back(qd);
            QL_DOUT("connectivity has been stored in nbs map");
        }
    }
    if (conn == gc_full) {
        QL_DOUT("connectivity is full");
        for (UInt qs = 0; qs < nq; qs++) {
            for (UInt qd = 0; qd < nq; qd++) {
                if (qs != qd) {
                    if (IsInterCoreHop(qs,qd) && (!IsCommQubit(qs) || !IsCommQubit(qd)) ) {
                        continue;
                    }
                    QL_DOUT("connecting qubit[" << qs << "] to qubit[" << qd << "]");
                    nbs.set(qs).push_back(qd);
                }
            }
        }
    }

    // when form embedded in grid, sort clock-wise starting from 12:00, to know boundary of search space
    if (form != gf_irregular) {
        // sort neighbor list by angles
        for (UInt qi = 0; qi < nq; qi++) {
            // sort nbs[qi] to have increasing clockwise angles around qi, starting with angle 0 at 12:00
            auto nbsq = nbs.find(qi);
            if (nbsq != nbs.end()) {
                nbsq->second.sort(
                    [this, qi](const UInt &i, const UInt &j) {
                        return Angle(x.at(qi), y.at(qi), x.at(i), y.at(i)) <
                               Angle(x.at(qi), y.at(qi), x.at(j), y.at(j));
                    }
                );
            }
        }
    }

    // Floyd-Warshall dist[i][j] = shortest distances between all nq qubits i and j
    // when not connected, distance remains maximum value

    // initialize all distances to maximum value, to neighbors to 1, to itself to 0
    dist.resize(nq); for (UInt i=0; i<nq; i++) dist[i].resize(nq, MAX);
    for (UInt i = 0; i < nq; i++) {
        dist[i][i] = 0;
        for (UInt j : nbs.get(i)) {
            dist[i][j] = 1;
        }
    }

    // find shorter distances by gradually including more qubits (k) in path
    for (UInt k = 0; k < nq; k++) {
        for (UInt i = 0; i < nq; i++) {
            for (UInt j = 0; j < nq; j++) {
                if (dist[i][j] > dist[i][k] + dist[k][j]) {
                    dist[i][j] = dist[i][k] + dist[k][j];
                }
            }
        }
    }
#ifdef debug
    for (UInt i = 0; i < nq; i++) {
        for (UInt j = 0; j < nq; j++) {
            if (form == gf_cross) {
                QL_ASSERT (dist[i][j] == (max(abs(x[i] - x[j]),
                                                      abs(y[i] - y[j]))));
            } else if (form == gf_plus) {
                QL_ASSERT (dist[i][j] ==
                              (abs(x[i] - x[j]) + abs(y[i] - y[j])));
            }

        }
    }
#endif

    DPRINTGrid();
}

// returns the neighbors for the given qubit
const utils::List<utils::UInt> &Grid::get_neighbors(utils::UInt qubit) const {
    return nbs.get(qubit);
}

// whether qubit is a communication qubit of a core, i.e. can communicate with another core
Bool Grid::IsCommQubit(UInt qi) const {
    if (ncores == 1) return true;
    QL_ASSERT(conn == gc_full);
    UInt qci = qi%ncores;   // index of qubit local to core
    return qci < ncommqpc;  // 0..ncommqpc-1 are comm qubits, ncommqpc..nq/ncores-1 are not comm qubits
}

// core index from qubit index
// when multi-core assumes full and uniform core connectivity
UInt Grid::CoreOf(UInt qi) const {
    if (ncores == 1) return 1;
    QL_ASSERT(conn == gc_full);
    UInt nqpc = nq/ncores;
    return qi/nqpc;
}

// inter-core hop from qs to qt?
Bool Grid::IsInterCoreHop(UInt qs, UInt qt) const {
    return CoreOf(qs) != CoreOf(qt);
}

// distance between two qubits
// formulae for convex (hole free) topologies with underlying grid and with bidirectional edges:
//      gf_cross:   max( abs( x[to_realqi] - x[from_realqi] ), abs( y[to_realqi] - y[from_realqi] ))
//      gf_plus:    abs( x[to_realqi] - x[from_realqi] ) + abs( y[to_realqi] - y[from_realqi] )
// when the neighbor relation is defined (topology.edges in config file), Floyd-Warshall is used, which currently is always
UInt Grid::Distance(UInt from_realqi, UInt to_realqi) const {
    return dist[from_realqi][to_realqi];
}

// coredistance between two qubits
// the number of inter-core hops that are minimally required on any path between the two qubits
//
// Here we assume for multi-core full and uniform core connectivity.
// When not full, have to compute it and store it in a CoreDistance matrix.
// Two cores are neighbours when they have communication qubits that are only one hop apart.
UInt Grid::CoreDistance(UInt from_realqi, UInt to_realqi) const {
    if (CoreOf(from_realqi) == CoreOf(to_realqi)) return 0;
    QL_ASSERT(conn == gc_full);
    return 1;
}

// minimum number of hops between two qubits is always >= distance(from, to)
// and inside one core (or without multi-core) the minimum number of hops == distance
//
// however, in multi-core with inter-core hops, an inter-core hop cannot execute a 2qgate
// so when the minimum number of hops are all inter-core hops (so distance(from,to) == coredistance(from,to))
// and no 2qgate has been placed yet, then at least one additional intra-core hop is needed for the 2qgate,
// the number of hops required being at least distance+1;
// but when there is only one comm qubit per core and that qubit is one of the operands of the 2qgate,
// then one additional intra-core hop is not sufficient,
// one additional one is needed to go/return to a non-comm qubit on the core;
// note that in the latter case, the comm qubit is twice in the path; this must not be regarded as a failure!
//
// we assume below that a valid path exists with distance+2 hops
UInt Grid::MinHops(UInt from_realqi, UInt to_realqi) const {
    UInt d = Distance(from_realqi, to_realqi);
    UInt cd = CoreDistance(from_realqi, to_realqi);
    QL_ASSERT(cd <= d);
    if (cd == d) {
        return d+2;
    } else {
        return d;
    }
}

// return clockwise angle around (cx,cy) of (x,y) wrt vertical y axis with angle 0 at 12:00, 0<=angle<2*pi
Real Grid::Angle(Int cx, Int cy, Int x, Int y) const {
    const Real pi = 4 * std::atan(1);
    Real a = std::atan2((x - cx), (y - cy));
    if (a < 0) a += 2*pi;
    return a;
}

// rotate neighbors list such that largest angle difference between adjacent elements is behind back;
// this is needed when a given subset of variations from a node is wanted (mappathselect==borders);
// and this can only be computed when there is an underlying x/y grid (so not for form==gf_irregular)
void Grid::Normalize(UInt src, neighbors_t &nbl) const {
    if (form != gf_xy) {
        // there are no implicit/explicit x/y coordinates defined per qubit, so no sense of nearness
        Str mappathselectopt = com::options::get("mappathselect");
        QL_ASSERT(mappathselectopt != "borders");
        return;
    }

    // std::cout << "Normalizing list from src=" << src << ": ";
    // for (auto dn : nbl) { std::cout << dn << " "; } std::cout << std::endl;

    const Real pi = 4 * std::atan(1);
    if (nbl.size() == 1) {
        // QL_DOUT("... size was 1; unchanged");
        return;
    }

    // find maxinx index in neighbor list before which largest angle difference occurs
    Int maxdiff = 0;                            // current maximum angle difference in loop search below
    auto maxinx = nbl.begin(); // before which max diff occurs

    // for all indices in and its next one inx compute angle difference and find largest of these
    for (auto in = nbl.begin(); in != nbl.end(); in++) {
        Real a_in = Angle(x.at(src), y.at(src), x.at(*in), y.at(*in));

        auto inx = std::next(in); if (inx == nbl.end()) inx = nbl.begin();
        Real a_inx = Angle(x.at(src), y.at(src), x.at(*inx), y.at(*inx));

        Int diff = a_inx - a_in; if (diff < 0) diff += 2*pi;
        if (diff > maxdiff) {
            maxdiff = diff;
            maxinx = inx;
        }
    }

    // and now rotate neighbor list so that largest angle difference is behind last one
    neighbors_t newnbl;
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


void Grid::DPRINTGrid() const {
    if (logger::log_level >= logger::LogLevel::LOG_DEBUG) {
        PrintGrid();
    }
}

void Grid::PrintGrid() const {
    if (form != gf_irregular) {
        for (UInt i = 0; i < nq; i++) {
            std::cout << "qubit[" << i << "]=(" << x.at(i) << "," << y.at(i) << ")";
            std::cout << " has neighbors ";
            for (auto &n : nbs.get(i)) {
                std::cout << "qubit[" << n << "]=(" << x.at(n) << "," << y.at(n) << ") ";
            }
            std::cout << std::endl;
        }
    } else {
        for (UInt i = 0; i < nq; i++) {
            std::cout << "qubit[" << i << "]";
            std::cout << " has neighbors ";
            for (auto &n : nbs.get(i)) {
                std::cout << "qubit[" << n << "] ";
            }
            std::cout << std::endl;
        }
    }
    for (UInt i = 0; i < nq; i++) {
        std::cout << "qubit[" << i << "] distance(" << i << ",j)=";
        for (UInt j = 0; j < nq; j++) {
            std::cout << Distance(i, j) << " ";
        }
        std::cout << std::endl;
    }
    for (UInt i = 0; i < nq; i++) {
        std::cout << "qubit[" << i << "] minhops(" << i << ",j)=";
        for (UInt j = 0; j < nq; j++) {
            std::cout << MinHops(i, j) << " ";
        }
        std::cout << std::endl;
    }
}

} // namespace plat
} // namespace ql
