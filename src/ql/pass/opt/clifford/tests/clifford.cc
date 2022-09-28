#include "ql/pass/testsupport/passtest.h"
#include "ql/pass/opt/clifford/optimize.h"


namespace ql {
namespace pass {

using CliffordOptimizePass = pass::opt::clifford::optimize::CliffordOptimizePass;
class CliffordOptimizePassTest : public PassTest<CliffordOptimizePass> {};

TEST_F(CliffordOptimizePassTest, EmptyProgram) {
  EXPECT_EQ(run(), 0);
  EXPECT_TRUE(kernel->gates.empty());
}

TEST_F(CliffordOptimizePassTest, SingleHadamardGate) {
  kernel->hadamard(0);
  EXPECT_EQ(run(), 0);

  checkGates({"ry90 q[0]", "x q[0]"});
}

TEST_F(CliffordOptimizePassTest, DoubleHadamardGate) {
  kernel->hadamard(0);
  kernel->hadamard(0);
  EXPECT_EQ(run(), 0);

  checkGates({});
}

} // namespace pass
} // namespace ql