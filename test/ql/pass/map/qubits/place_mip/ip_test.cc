#include "ql/pass/map/qubits/place_mip/detail/impl.h"

#include <gtest/gtest.h>


namespace ql::pass::map::qubits::place_mip::detail {

class IpTest : public ::testing::Test {
protected:
    IpTest() {
        ql::utils::logger::set_log_level("LOG_INFO");
    }

    void computeAndCheckResultType(Result expected) {
        ASSERT_GE(qubitsCount, 0);

        Impl impl{qubitsCount, twoQGatesCount, [this](utils::UInt q1, utils::UInt q2) -> utils::UInt {
            return (q1 == q2) ? 0 : distances[q1][q2];
        }, {}};

        auto actual = impl.run(mapping);

        EXPECT_EQ(expected, actual);
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

    void setupLine(utils::UInt qubits_count = 3) {
        init(qubits_count);

        /*

            0-------1-------2----- ....  -----(n-1)------n

        */

        for (utils::UInt i = 0; i < qubits_count; ++i) {
            for (utils::UInt j = i + 1; j < qubits_count; ++j) {
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
        ASSERT_GE(qubitsCount, 0);
        ASSERT_LE(q1, qubitsCount);
        ASSERT_LE(q2, qubitsCount);

        twoQGatesCount[std::make_pair(q1, q2)] += n;
    }

    void checkPermutation(std::vector<utils::UInt> expected) {
        ASSERT_EQ(expected.size(), qubitsCount);
        ASSERT_EQ(mapping.size(), qubitsCount);

        for (utils::UInt i = 0; i < qubitsCount; ++i) {
            std::ostringstream info;
            info << "Failure happens because virtual qubit " << i << " is expected to map to real qubit " << expected[i]
                << ", but the result shows that it actually maps to real qubit " << mapping[i];
            EXPECT_EQ(expected[i], mapping[i]) << info.str();
        }
    }

    void checkAllMappedGatesAreNearestNeighbors() {
        for (const auto& kv: twoQGatesCount) {
            std::ostringstream info;
            info << "Gate between operands " << kv.first.first << " and " << kv.first.second
                << " and occurrence count " << kv.second << " is not between nearest neighbors after mapping.";
            ASSERT_NE(kv.first.first, kv.first.second) << info.str();
            EXPECT_EQ(distances[mapping[kv.first.first]][mapping[kv.first.second]], 1) << info.str();
        }
    }

    void checkAtLeastOneMappedGateIsNonNN() {
        utils::UInt nonNNGateCount = 0;
        for (const auto& kv: twoQGatesCount) {
            if (distances[mapping[kv.first.first]][mapping[kv.first.second]] > 1) {
                ++nonNNGateCount;
            }
        }

        EXPECT_GE(nonNNGateCount, 1);
    }

    void checkIndividualMapping(size_t mapping_index, size_t qubit_index) {
        EXPECT_EQ(mapping[mapping_index], qubit_index);
    }

    utils::UInt getQubitsCount() const {
        return qubitsCount;
    }

private:
    void init(utils::UInt aQubitsCount) {
        ASSERT_EQ(qubitsCount, 0);
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

class IpStarTest : public IpTest {
protected:
    void SetUp() override {
        setupStar();
    }
};

class IpCliqueTest : public IpTest {
protected:
    void SetUp() override {
        setupClique();
    }
};

class IpLineTest : public IpTest {
protected:
    void SetUp() override {
        setupLine();
    }
};

class IpGridTest : public IpTest {
protected:
    void SetUp() override {
        setupGrid();
    }
};

TEST_F(IpStarTest, star_with_no_2q_gate) {
    computeAndCheckResultType(Result::ANY);
}

TEST_F(IpCliqueTest, clique_with_no_2q_gate) {
    computeAndCheckResultType(Result::ANY);
}

TEST_F(IpLineTest, line_with_2q_gate) {
    add2QGate(0, 2);

    computeAndCheckResultType(Result::NEW_MAP);

    checkPermutation({1, 0, 2});
    checkAllMappedGatesAreNearestNeighbors();
}

TEST_F(IpCliqueTest, clique_with_2q_gate) {
    add2QGate(1, 3);

    computeAndCheckResultType(Result::CURRENT);
}

TEST_F(IpStarTest, star_with_2q_gate__one_2q_gate) {
    add2QGate(1, 3);

    computeAndCheckResultType(Result::NEW_MAP);

    // This is not the most straightforward mapping (swapping 2 and 3 is useless) but it works.
    checkPermutation({1, 0, 3, 2, 4});
    checkAllMappedGatesAreNearestNeighbors();
}

TEST_F(IpStarTest, star_with_2q_gate__force_change_of_center) {
    add2QGate(1, 3);
    add2QGate(1, 2);

    computeAndCheckResultType(Result::NEW_MAP);

    // Swapping 2 and 3 is useless...
    checkPermutation({1, 0, 3, 2, 4});
    checkAllMappedGatesAreNearestNeighbors();
}

TEST_F(IpStarTest, star_with_2q_gate__all_possible_interactions_between_properly_mapped_qubits) {
    add2QGate(0, 1);
    add2QGate(0, 2);
    add2QGate(0, 3);
    add2QGate(0, 4);

    computeAndCheckResultType(Result::CURRENT);
}

TEST_F(IpStarTest, star_with_2q_gate__all_possible_interactions_with_new_center) {
    add2QGate(1, 0);
    add2QGate(1, 2);
    add2QGate(1, 3);
    add2QGate(1, 4);

    computeAndCheckResultType(Result::NEW_MAP);

    // Virtual qubit 1 maps to center.
    checkPermutation({1, 0, 4, 2, 3});
    checkAllMappedGatesAreNearestNeighbors();
}

TEST_F(IpStarTest, star_with_2q_gate__no_perfect_solution) {
    add2QGate(1, 2, 5);
    add2QGate(3, 4, 10);

    computeAndCheckResultType(Result::NEW_MAP);

    // Virtual qubit 4 maps to center.
    checkIndividualMapping(4, 0);
    // We don't check a given permutation here because there are at least two valid ones
    // E.g. {4, 3, 2, 1, 0} and {4, 2, 3, 1, 0}
    checkAtLeastOneMappedGateIsNonNN();
}

TEST_F(IpStarTest, star_with_2q_gate__no_perfect_solution__same_gates__counts_swapped__better_center_is_chosen) {
    add2QGate(1, 2, 10);
    add2QGate(3, 4, 5);

    computeAndCheckResultType(Result::NEW_MAP);

    // Virtual qubit 1 maps to center.
    checkPermutation({3, 0, 1, 4, 2});
    checkAtLeastOneMappedGateIsNonNN();
}

TEST_F(IpGridTest, grid__preserve_non_used_virtual_qubit_indices) {
    add2QGate(0, 2);

    computeAndCheckResultType(Result::NEW_MAP);

    checkPermutation({1, 2, 0, 3, 4, 5});
    checkAllMappedGatesAreNearestNeighbors();
}

TEST_F(IpGridTest, grid__make_extremes_closer) {
    add2QGate(0, 5);
    add2QGate(3, 2);

    computeAndCheckResultType(Result::NEW_MAP);

    checkPermutation({5, 2, 0, 1, 3, 4});
    checkAllMappedGatesAreNearestNeighbors();
}

TEST_F(IpGridTest, grid__find_complex_permutation) {
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

    // We don't check a given permutation here because there are at least two valid ones
    // E.g. {2, 4, 3, 0, 5, 1} and {0, 4, 5, 2, 3, 1}
    checkAllMappedGatesAreNearestNeighbors();
}

class IpVeryLongLineFindPermutationOfNQubitsTest : public IpTest {
protected:
    void SetUp() override {
#ifdef NDEBUG
        utils::UInt lineSize = 10;
#else
        utils::UInt lineSize = 6;
#endif

        setupLine(lineSize);
        ASSERT_EQ(getQubitsCount() % 2, 0);

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
        expectedPermutation1 = std::vector<utils::UInt>(getQubitsCount(), 0);
        for (utils::UInt i = 0; i < getQubitsCount() / 2; ++i) {
            expectedPermutation1[i] = 2 * i;
            expectedPermutation1[getQubitsCount() - 1 - i] = 2 * i + 1;
        }

        std::vector<utils::UInt> expectedPermutation2(getQubitsCount(), 0);
        for (utils::UInt i = 0; i < getQubitsCount() / 2; ++i) {
            expectedPermutation2[i] = getQubitsCount() - 1 - 2 * i;
            expectedPermutation2[getQubitsCount() - 1 - i] = getQubitsCount() - 2 * (i + 1);
        }
        (void) expectedPermutation2;
    }

protected:
    std::vector<utils::UInt> expectedPermutation1;
};

// The following test case can take some time to complete,
// especially when compiler optimizations are disabled (DEBUG build).
TEST_F(IpVeryLongLineFindPermutationOfNQubitsTest, perfect_mapping) {
    computeAndCheckResultType(Result::NEW_MAP);

    checkPermutation(expectedPermutation1);
    checkAllMappedGatesAreNearestNeighbors();
}

TEST_F(IpVeryLongLineFindPermutationOfNQubitsTest, imperfect_mapping) {
    add2QGate(0, 1); // This gate is not NN in the optimal case.

    computeAndCheckResultType(Result::NEW_MAP);

    checkPermutation(expectedPermutation1);
    checkAtLeastOneMappedGateIsNonNN();
}

class IpHorizonTest : public ::testing::Test {
protected:
    void SetUp() override {
        twoQGatesCount[std::make_pair(0, 1)] = 10;
        twoQGatesCount[std::make_pair(3, 4)] = 5;
        twoQGatesCount[std::make_pair(3, 5)] = 3;
        twoQGatesCount[std::make_pair(5, 3)] = 3;
        twoQGatesCount[std::make_pair(1, 2)] = 9;
        twoQGatesCount[std::make_pair(1, 0)] = 2;

        originalTwoQGatesCount = twoQGatesCount;
    }

protected:
    Impl::TwoQGatesCount twoQGatesCount{};
    Impl::TwoQGatesCount originalTwoQGatesCount{};
};

TEST_F(IpHorizonTest, horizon_smaller_than_number_of_different_2q_gates__pick_most_important_2q_gate_types) {
    applyHorizon(2, twoQGatesCount);

    EXPECT_EQ(twoQGatesCount.size(), 2);
    EXPECT_EQ(twoQGatesCount.at(std::make_pair(0, 1)), 10);
    EXPECT_EQ(twoQGatesCount.at(std::make_pair(1, 2)), 9);
}

TEST_F(IpHorizonTest, horizon_greater_than_number_of_different_2q_gates) {
    applyHorizon(20, twoQGatesCount);

    EXPECT_EQ(twoQGatesCount.size(), 6);
    EXPECT_EQ(originalTwoQGatesCount, twoQGatesCount);
}

TEST_F(IpHorizonTest, horizon_is_0) {
    applyHorizon(0, twoQGatesCount);

    EXPECT_EQ(twoQGatesCount.size(), 6);
    EXPECT_EQ(originalTwoQGatesCount, twoQGatesCount);
}

}  // namespace ql::pass::map::qubits::place_mip::detail
