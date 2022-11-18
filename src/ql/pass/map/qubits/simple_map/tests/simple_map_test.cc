#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "ql/pass/map/qubits/simple_map/detail/impl.h"
#include "ql/pass/map/qubits/simple_map/detail/path_solver.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace simple_map {
namespace detail {

class PathSolverTest {
protected:
    PathSolverTest() : victim(5) {}

    void add(PathSolver::QubitOccupation occ, Path p) {
        ++nPaths;

        victim.addRoutingPath(occ, p);
    }

    void compute() {
        res = victim.compute();

        CHECK_EQ(res.size(), nPaths);
    }

    void checkFirst(std::size_t index, Path expected) {
        REQUIRE_LE(index, res.size());

        checkVector(res[index].first, expected);
    }

    void checkSecond(std::size_t index, Path expected) {
        REQUIRE_LE(index, res.size());

        checkVector(res[index].second, expected);
    }

private:
    static void checkVector(Path actual, Path expected) {
        REQUIRE_EQ(actual.size(), expected.size());

        for (std::size_t i = 0; i < actual.size(); ++i) {
            CAPTURE(i);
            CHECK_EQ(actual[i], expected[i]);
        }
    }

    PathSolver victim;
    std::vector<PathSolver::SplitPath> res;
    std::size_t nPaths = 0;
};

TEST_CASE_FIXTURE(PathSolverTest, "No occupation") {
    add({}, { 1, 5, 2, 3 });
    SUBCASE("Single path") {
        compute();

        checkFirst(0, {1, 5});
        checkSecond(0, {3, 2});
    }

    SUBCASE("2 paths, distinct qubits") {
        add({}, { 7, 4, 6 });

        compute();

        checkFirst(0, {1, 5});
        checkSecond(0, {3, 2});
        checkFirst(1, {7});
        checkSecond(1, {6, 4});
    }

    SUBCASE("2 paths, common qubit") {
        add({}, { 7, 4, 6, 3 });

        compute();

        checkFirst(0, {1, 5});
        checkSecond(0, {3, 2});
        checkFirst(1, {1, 5});
        checkSecond(1, {3, 2});
    }
}


class ShortestPathsTest {
protected:
    ShortestPathsTest() {
        ql::utils::logger::set_log_level("LOG_INFO");
    }

    void setNumQubits(UInt n) {
        QL_ASSERT(n > 0);
        numQubits = n;
        neighbors.resize(numQubits);
    }

    void bidirectionalEdge(UInt q1, UInt q2) {
        QL_ASSERT(numQubits > 0);

        neighbors[q1].push_back(q2);
        neighbors[q2].push_back(q1);
    }

    void compute() {
        result = Impl::computeShortestPaths(numQubits, [this](UInt q1) {
            REQUIRE_LE(q1, numQubits);
            return neighbors[q1];
        });

        debugPrintShortestPaths();
    }

    UInt nShortestPaths(UInt q1, UInt q2) {
        REQUIRE(!result.empty());

        return result[std::make_pair(q1, q2)].size();
    }

    bool isShortestPath(utils::List<UInt> path) {
        REQUIRE_GE(path.size(), 2);
        REQUIRE(!result.empty());

        const auto& actual = result[std::make_pair(path.front(), path.back())];
        REQUIRE(!actual.empty());
        return std::find(actual.begin(), actual.end(), path) != actual.end();
    }

    void debugPrintShortestPaths() {
        REQUIRE(!result.empty());

        for (const auto& kv: result) {
            std::cout << kv.first << "  ->  " << std::endl;
            for (const auto& p: kv.second) {
                std::cout << "        ->  " << p << std::endl;
            }
        }
    }

private:
    UInt numQubits = 0;
    std::vector<utils::List<UInt>> neighbors;
    Impl::ShortestPaths result;
};

TEST_CASE_FIXTURE(ShortestPathsTest, "Small line") {
    setNumQubits(3);

    bidirectionalEdge(0, 1);
    bidirectionalEdge(1, 2);

    compute();

    CHECK_EQ(nShortestPaths(0, 0), 0);
    CHECK_EQ(nShortestPaths(1, 1), 0);
    CHECK_EQ(nShortestPaths(2, 2), 0);
    CHECK_EQ(nShortestPaths(1, 0), 1);
    CHECK_EQ(nShortestPaths(1, 0), 1);
    CHECK_EQ(nShortestPaths(1, 2), 1);
    CHECK_EQ(nShortestPaths(2, 1), 1);
    CHECK_EQ(nShortestPaths(0, 2), 1);

    CHECK(isShortestPath({0, 1}));
    CHECK(isShortestPath({1, 0}));
    CHECK(isShortestPath({0, 1, 2}));
    CHECK(isShortestPath({2, 1, 0}));
}

TEST_CASE_FIXTURE(ShortestPathsTest, "Square") {
    setNumQubits(4);

    /*
        0 ---- 1
        |      |
        |      |
        3 ---- 2
    */

    bidirectionalEdge(0, 1);
    bidirectionalEdge(1, 2);
    bidirectionalEdge(2, 3);
    bidirectionalEdge(3, 0);

    compute();

    for (auto i = 0; i < 4; ++i) {
        CAPTURE(i);
        CHECK_EQ(nShortestPaths(i, i), 0);
    }

    for (auto i = 0; i < 4; ++i) {
        CAPTURE(i);
        CHECK_EQ(nShortestPaths(i, (i + 1) % 4), 1);
    }
    
    for (auto i = 0; i < 4; ++i) {
        CAPTURE(i);
        CHECK_EQ(nShortestPaths(i, (i + 2) % 4), 2);
    }

    
    CHECK(isShortestPath({0, 1}));
    CHECK(isShortestPath({0, 1, 2}));
    CHECK(isShortestPath({3, 2}));
    CHECK(isShortestPath({3, 2, 1}));
    CHECK(isShortestPath({3, 0, 1}));
    CHECK(isShortestPath({2, 3, 0}));
    CHECK(isShortestPath({2, 1, 0}));
}

class SimpleMapTest {
protected:
    SimpleMapTest() {
        ql::utils::logger::set_log_level("LOG_INFO");
    }

private:
};

TEST_CASE_FIXTURE(SimpleMapTest, "First test") {
}


}
}
}
}
}
}