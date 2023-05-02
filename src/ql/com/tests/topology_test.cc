#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "ql/com/topology.h"

namespace ql {
namespace com {

class TopologyTest {};

TEST_CASE_FIXTURE(TopologyTest, "Single core grid 2x3") {
    std::uint64_t qubit_count = 6;

/*

        0 --- 1 --- 2
        |     |     |
        5 --- 4 --- 3

*/

    auto victim = ql::com::Topology(qubit_count, ql::utils::Json( R"({
"form": "xy",
"x_size": 2,
"y_size": 3,
"qubits": [
    { "id": 0, "x": 0, "y": 0},
    { "id": 1, "x": 0, "y": 1},
    { "id": 2, "x": 0, "y": 2},
    { "id": 3, "x": 1, "y": 2},
    { "id": 4, "x": 1, "y": 1},
    { "id": 5, "x": 1, "y": 0}
],
"edges": [
    { "id": 0, "src": 0, "dst": 1},
    { "id": 1, "src": 1, "dst": 0},
    { "id": 2, "src": 1, "dst": 2},
    { "id": 3, "src": 2, "dst": 1},
    { "id": 4, "src": 2, "dst": 3},
    { "id": 5, "src": 3, "dst": 2},
    { "id": 6, "src": 3, "dst": 4},
    { "id": 7, "src": 4, "dst": 3},
    { "id": 8, "src": 4, "dst": 5},
    { "id": 9, "src": 5, "dst": 4},
    { "id": 10, "src": 5, "dst": 0},
    { "id": 11, "src": 0, "dst": 5},
    { "id": 12, "src": 1, "dst": 4},
    { "id": 13, "src": 4, "dst": 1}
]
})"_json ));

    CHECK_EQ(victim.get_num_cores(), 1);
    CHECK_EQ(victim.get_num_qubits(), qubit_count);
    CHECK_EQ(victim.get_num_qubits_per_core(), qubit_count);
    CHECK_EQ(victim.get_core_index(15), 0);
    CHECK_EQ(victim.get_grid_size().x, 2);
    CHECK_EQ(victim.get_grid_size().y, 3);
    CHECK_EQ(victim.get_qubit_coordinate(2).x, 0);
    CHECK_EQ(victim.get_qubit_coordinate(2).y, 2);
    CHECK_EQ(victim.get_neighbors(2), Topology::Neighbors{ 3, 1 });
    CHECK_EQ(victim.get_neighbors(4), Topology::Neighbors{ 3, 5, 1 });
}

TEST_CASE_FIXTURE(TopologyTest, "Single core custom connectivity") {
    std::uint64_t qubit_count = 5;

/*

              3
              |
        0 --- 1 --- 2
              |
              4

*/

    auto victim = ql::com::Topology(qubit_count, ql::utils::Json( R"({
"form": "irregular",
"edges": [
    { "id": 0, "src": 1, "dst": 0},
    { "id": 1, "src": 0, "dst": 1},
    { "id": 2, "src": 1, "dst": 3},
    { "id": 3, "src": 3, "dst": 1},
    { "id": 4, "src": 1, "dst": 2},
    { "id": 5, "src": 2, "dst": 1},
    { "id": 6, "src": 1, "dst": 4},
    { "id": 7, "src": 4, "dst": 1}
]
})"_json ));

    CHECK_EQ(victim.get_num_cores(), 1);
    CHECK_EQ(victim.get_num_qubits(), qubit_count);
    CHECK_EQ(victim.get_num_qubits_per_core(), qubit_count);
    CHECK_EQ(victim.get_grid_size().x, 0);
    CHECK_EQ(victim.get_grid_size().y, 0);
    CHECK_EQ(victim.get_neighbors(1), Topology::Neighbors{ 0, 3, 2, 4 });
    CHECK_EQ(victim.get_neighbors(4), Topology::Neighbors{ 1 });
    CHECK_EQ(victim.get_distance(4, 3), 2);
    CHECK_EQ(victim.get_distance(0, 1), 1);
    CHECK_EQ(victim.get_distance(0, 0), 0);
    CHECK(!victim.has_coordinates());
    CHECK_EQ(victim.get_edge_index({0, 0}), -1);
    CHECK_EQ(victim.get_edge_index({0, 1}), 1);
    CHECK_EQ(victim.get_edge_index({4, 1}), 7);
    CHECK_EQ(victim.get_edge_index({0, 2}), -1);
}

TEST_CASE_FIXTURE(TopologyTest, "Large multicore, all qubits are communication qubits") {
    std::uint64_t qubit_count = 1024;
    auto victim = ql::com::Topology(qubit_count, ql::utils::Json( R"({
"number_of_cores": 64,
"connectivity": "full",
"form": "irregular",
"comm_qubits_per_core": 16
})"_json ));

    CHECK_EQ(victim.get_num_qubits(), qubit_count);
    CHECK_EQ(victim.get_num_cores(), 64);
    CHECK_EQ(victim.get_num_qubits_per_core(), 16);
    CHECK_EQ(victim.get_core_index(15), 0);
    CHECK_EQ(victim.get_core_index(23), 1);

    for (std::uint64_t i = 0; i < qubit_count; ++i) {
        CHECK(victim.is_comm_qubit(i));
    }

    CHECK(!victim.is_inter_core_hop(0, 13));
    CHECK(victim.is_inter_core_hop(0, 24));
    CHECK(victim.is_inter_core_hop(123, 456));
    CHECK(!victim.is_inter_core_hop(123, 124));
}

TEST_CASE_FIXTURE(TopologyTest, "Multicore, 2 communication qubits for each 4-qubits core") {
    std::uint64_t qubit_count = 8;
    auto victim = ql::com::Topology(qubit_count, ql::utils::Json( R"({
"number_of_cores": 2,
"connectivity": "full",
"form": "irregular",
"comm_qubits_per_core": 2
})"_json ));

    CHECK_EQ(victim.get_num_cores(), 2);
    CHECK_EQ(victim.get_core_index(0), 0);
    CHECK(victim.is_comm_qubit(0));
    CHECK(victim.is_comm_qubit(1));
    CHECK(!victim.is_comm_qubit(2));
    CHECK(!victim.is_comm_qubit(3));
    CHECK_EQ(victim.get_core_index(4), 1);
    CHECK(victim.is_comm_qubit(4));
}

}
}