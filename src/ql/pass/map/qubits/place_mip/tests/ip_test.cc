#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "ql/pass/map/qubits/place_mip/detail/impl.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace place_mip {
namespace detail {

class IpTest {
protected:
    IpTest() {
        ql::utils::logger::set_log_level("LOG_INFO");
    }

    void computeAndCheckResultType(Result expected) {
        REQUIRE_GE(qubitsCount, 0);

        Impl impl{qubitsCount, twoQGatesCount, [this](utils::UInt q1, utils::UInt q2) {
            REQUIRE_LE(q1, utils::MAX);
            REQUIRE_LE(q2, utils::MAX);
            return q1 == q2 ? 0 : distances[q1][q2];
        }, {}};

        auto actual = impl.run(mapping);

        CHECK_EQ(expected, actual);
    }

    void setupClique() {
        init(5);

        for (utils::UInt i = 0; i < distances.size(); ++i) {
            for (utils::UInt j = 0; j < distances[i].size(); ++j) {
                if (i != j) {
                    distances[i][j] = 1;
                }
            }
        }
    }

    void setupStar() {
        init(5);

        /*

                    2
                    |
                    |
            1-------0-------3
                    |
                    |
                    4

        */

        for (utils::UInt i = 1; i < qubitsCount; ++i) {
            distances[0][i] = 1;
            distances[i][0] = 1;
        }

        for (utils::UInt i = 1; i < qubitsCount; ++i) {
            for (utils::UInt j = 1; j < qubitsCount; ++j) {
                if (i == j) {
                    continue;
                }

                distances[i][j] = 2;
                distances[j][i] = 2;
            }
        }
    }
    
    void setupLine(utils::UInt qubitsCount = 3) {
        init(qubitsCount);

        /*

            0-------1-------2----- ....  -----(n-1)------n

        */

        for (utils::UInt i = 0; i < qubitsCount; ++i) {
            for (utils::UInt j = i + 1; j < qubitsCount; ++j) {
                distances[i][j] = j - i;   
                distances[j][i] = j - i;   
            }
       }
    }

    void setupGrid() {
        init(6);

        /*

            0------1------2
            |      |      |
            |      |      |
            3------4------5

        */

        auto setDistance = [this](utils::UInt q1, utils::UInt q2, utils::UInt d) { distances[q1][q2] = d; distances[q2][q1] = d; };
        setDistance(0, 1, 1);
        setDistance(1, 2, 1);
        setDistance(2, 5, 1);
        setDistance(5, 4, 1);
        setDistance(4, 3, 1);
        setDistance(3, 0, 1);
        setDistance(1, 4, 1);

        setDistance(0, 2, 2);
        setDistance(0, 4, 2);
        setDistance(1, 3, 2);
        setDistance(1, 5, 2);
        setDistance(2, 4, 2);
        setDistance(3, 5, 2);

        setDistance(0, 5, 3);
        setDistance(2, 3, 3);
    }
    
    void add2QGate(utils::UInt q1, utils::UInt q2, utils::UInt n = 1) {
        REQUIRE_GE(qubitsCount, 0);
        REQUIRE_LE(q1, qubitsCount);
        REQUIRE_LE(q2, qubitsCount);

        twoQGatesCount[std::make_pair(q1, q2)] += n;
    }

    void checkPermutation(std::vector<utils::UInt> expected) {
        REQUIRE_EQ(expected.size(), qubitsCount);
        REQUIRE_EQ(mapping.size(), qubitsCount);

        for (utils::UInt i = 0; i < qubitsCount; ++i) {
            INFO("Failure happens because virtual qubit ", i, " is expected to map to"
            " real qubit ", expected[i], ", but the result shows that it actually maps to real qubit ", mapping[i]);
            CHECK_EQ(expected[i], mapping[i]);
        }
    }

    void checkAllMappedGatesAreNearestNeighbors() {
        for (const auto& kv: twoQGatesCount) {
            INFO("Gate between operands ", kv.first.first, " and ", kv.first.second, " and occurrence count ", kv.second, " is not between nearest neighbors after mapping.");
            REQUIRE_NE(kv.first.first, kv.first.second);
            CHECK_EQ(distances[mapping[kv.first.first]][mapping[kv.first.second]], 1);
        }
    }

    void checkAtLeastOneMappedGateIsNonNN() {
        utils::UInt nonNNGateCount = 0;
        for (const auto& kv: twoQGatesCount) {
            if (distances[mapping[kv.first.first]][mapping[kv.first.second]] > 1) {
                ++nonNNGateCount;
            };
        }

        CHECK_GE(nonNNGateCount, 1);
    }

    utils::UInt getQubitsCount() {
        return qubitsCount;
    }

private:
    void init(utils::UInt aQubitsCount) {
        REQUIRE_EQ(qubitsCount, 0);
        qubitsCount = aQubitsCount;
        mapping = utils::Vec<utils::UInt>(qubitsCount, UNDEFINED_QUBIT);

        distances.resize(qubitsCount);
        for (auto &ds: distances) {
            ds.resize(qubitsCount, utils::MAX);
        }
    }

    utils::UInt qubitsCount = 0;
    std::vector<std::vector<utils::UInt>> distances;
    Impl::TwoQGatesCount twoQGatesCount{};
    utils::Vec<utils::UInt> mapping;
};

TEST_CASE_FIXTURE(IpTest, "Star with no 2Q gate") {
    setupStar();

    computeAndCheckResultType(Result::ANY);
}

TEST_CASE_FIXTURE(IpTest, "Clique with no 2Q gate") {
    setupClique();

    computeAndCheckResultType(Result::ANY);
}

TEST_CASE_FIXTURE(IpTest, "Line with 2Q gate") {
    setupLine();

    add2QGate(0, 2);

    computeAndCheckResultType(Result::NEW_MAP);
    
    checkPermutation({1, 0, 2});
    checkAllMappedGatesAreNearestNeighbors();
}

TEST_CASE_FIXTURE(IpTest, "Clique with 2Q gate") {
    setupClique();

    add2QGate(1, 3);

    computeAndCheckResultType(Result::CURRENT);
}

TEST_CASE_FIXTURE(IpTest, "Star with 2Q gate") {
    setupStar();

    SUBCASE("One 2q gate") {
        add2QGate(1, 3);

        computeAndCheckResultType(Result::NEW_MAP);

        // This is not the most straightforward mapping (swapping 2 and 3 is useless) but it works.
        checkPermutation({1, 0, 3, 2, 4});
        checkAllMappedGatesAreNearestNeighbors();
    }

    SUBCASE("Force change of center") {
        add2QGate(1, 3);
        add2QGate(1, 2);

        computeAndCheckResultType(Result::NEW_MAP);

        // Swapping 2 and 3 is useless...
        checkPermutation({1, 0, 3, 2, 4});
        checkAllMappedGatesAreNearestNeighbors();
    }
    
    SUBCASE("All possible interactions between properly mapped qubits") {
        add2QGate(0, 1);
        add2QGate(0, 2);
        add2QGate(0, 3);
        add2QGate(0, 4);

        computeAndCheckResultType(Result::CURRENT);
    }
    
    SUBCASE("All possible interactions with new center") {
        add2QGate(1, 0);
        add2QGate(1, 2);
        add2QGate(1, 3);
        add2QGate(1, 4);

        computeAndCheckResultType(Result::NEW_MAP);

        // Virtual qubit 1 maps to center.
        checkPermutation({1, 0, 4, 2, 3});
        checkAllMappedGatesAreNearestNeighbors();
    }
    
    SUBCASE("No perfect solution") {
        add2QGate(1, 2, 5);
        add2QGate(3, 4, 10);

        computeAndCheckResultType(Result::NEW_MAP);

        // Virtual qubit 4 maps to center.
        checkPermutation({4, 3, 2, 1, 0});
        checkAtLeastOneMappedGateIsNonNN();
    }

    SUBCASE("No perfect solution, same gates, counts swapped => better center is chosen") {
        add2QGate(1, 2, 10);
        add2QGate(3, 4, 5);

        computeAndCheckResultType(Result::NEW_MAP);

        // Virtual qubit 1 maps to center.
        checkPermutation({3, 0, 1, 4, 2});
        checkAtLeastOneMappedGateIsNonNN();
    }
}

TEST_CASE_FIXTURE(IpTest, "Grid") {
    setupGrid();

    SUBCASE("Preserve non-used virtual qubit indices") {
        add2QGate(0, 2);

        computeAndCheckResultType(Result::NEW_MAP);

        checkPermutation({1, 2, 0, 3, 4, 5});
        checkAllMappedGatesAreNearestNeighbors();
    }

    SUBCASE("Make extremes closer") {
        add2QGate(0, 5);
        add2QGate(3, 2);

        computeAndCheckResultType(Result::NEW_MAP);

        checkPermutation({5, 2, 0, 1, 3, 4});
        checkAllMappedGatesAreNearestNeighbors();
    }

    SUBCASE("Find complex permutation") {
        /*

        This test case adds all possible nearest neighboring gates in the following topology: 

            3------5------0
            |      |      |
            |      |      |
            2------1------4

        */

        add2QGate(3, 5);
        add2QGate(5, 0);
        add2QGate(0, 4);
        add2QGate(4, 1);
        add2QGate(5, 1);
        add2QGate(1, 2);
        add2QGate(2, 3);

        computeAndCheckResultType(Result::NEW_MAP);

        checkPermutation({2, 4, 3, 0, 5, 1});

        checkAllMappedGatesAreNearestNeighbors();
    }
}

// The following test case can take some time to complete,
// especially when compiler optimizations are disabled (DEBUG build).
TEST_CASE_FIXTURE(IpTest, "Very long line, find permutation of N qubits") {
#ifdef NDEBUG
    utils::UInt lineSize = 10;
#else
    utils::UInt lineSize = 6;
#endif

    setupLine(lineSize);
    REQUIRE_EQ(getQubitsCount() % 2, 0);

    // n = qubitsCount
    // 0 ------ (n - 1) ------ 1 ------ (n - 2) ----- 2 ------ (n - 3) ------ ... ------- (n / 2 - 2) ------ (n / 2 + 1) ------ (n / 2 - 1) ------- (n / 2)

    for (utils::UInt i = 0; i <= getQubitsCount() / 2 - 2; ++i) {
        add2QGate(i, getQubitsCount() - 1 - i, 2 + i % 5);
        add2QGate(getQubitsCount() - 1 - i, i + 1, 3 + i % 5);
    }
    add2QGate(getQubitsCount() / 2 - 1, getQubitsCount() / 2, 4);

    // There are two possible perfect permutations that make these 2q gates
    // executable on a line (since the line has symmetry).
    // Which one is output by the algorithm depends on HiGHS implementation details,
    // so should be considered to be random.
    std::vector<utils::UInt> expectedPermutation1(getQubitsCount(), 0);
    for (utils::UInt i = 0; i < getQubitsCount() / 2; ++i) {
        expectedPermutation1[i] = 2 * i;
        expectedPermutation1[getQubitsCount() - 1 - i] = 2 * i + 1;
    }

    std::vector<utils::UInt> expectedPermutation2(getQubitsCount(), 0);
    for (utils::UInt i = 0; i < getQubitsCount() / 2; ++i) {
        expectedPermutation2[i] = getQubitsCount() - 1 - 2 * i;
        expectedPermutation2[getQubitsCount() - 1 - i] = getQubitsCount() - 2 * (i + 1);
    }
    (void)expectedPermutation2;

    SUBCASE("Perfect mapping") {
        computeAndCheckResultType(Result::NEW_MAP);

        checkPermutation(expectedPermutation1);
        checkAllMappedGatesAreNearestNeighbors();
    }

    SUBCASE("Imperfect mapping") {
        add2QGate(0, 1); // This gate is not NN in the optimal case.

        computeAndCheckResultType(Result::NEW_MAP);

        checkPermutation(expectedPermutation1);
        checkAtLeastOneMappedGateIsNonNN();
    }
}

TEST_CASE("Horizon") {
    Impl::TwoQGatesCount twoQGatesCount{};

    twoQGatesCount[std::make_pair(0, 1)] = 10;
    twoQGatesCount[std::make_pair(3, 4)] = 5;
    twoQGatesCount[std::make_pair(3, 5)] = 3;
    twoQGatesCount[std::make_pair(5, 3)] = 3;
    twoQGatesCount[std::make_pair(1, 2)] = 9;
    twoQGatesCount[std::make_pair(1, 0)] = 2;

    auto originalTwoQGatesCount = twoQGatesCount;

    SUBCASE("Horizon smaller than # of different 2q gates, pick most important 2q gate types") {
        applyHorizon(2, twoQGatesCount);

        CHECK_EQ(twoQGatesCount.size(), 2);
        CHECK_EQ(twoQGatesCount.at(std::make_pair(0, 1)), 10);
        CHECK_EQ(twoQGatesCount.at(std::make_pair(1, 2)), 9);
    }

    SUBCASE("Horizon greater than # of different 2q gates") {
        applyHorizon(20, twoQGatesCount);

        CHECK_EQ(twoQGatesCount.size(), 6);
        CHECK_EQ(originalTwoQGatesCount, twoQGatesCount);
    }

    SUBCASE("Horizon is 0") {
        applyHorizon(0, twoQGatesCount);

        CHECK_EQ(twoQGatesCount.size(), 6);
        CHECK_EQ(originalTwoQGatesCount, twoQGatesCount);
    }
}

}
}
}
}
}
}