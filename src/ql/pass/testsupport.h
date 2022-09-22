#include "gtest/gtest.h"
#include "ql/utils/ptr.h"
#include "ql/utils/logger.h"
#include "ql/pmgr/factory.h"
#include "ql/ir/compat/program.h"
#include "ql/ir/compat/platform.h"

// fixme: should not be exposed in the public API

namespace ql {
namespace pass {
    

template <typename Pass>
class PassTest : public ::testing::Test {
private:
  Pass victim{utils::Ptr<const pmgr::Factory>(), "TestInstance", "TestType"};
  
  ir::compat::PlatformRef platform = ir::compat::Platform::build("TestPlatform", utils::Str("cc_light"));
  ir::compat::ProgramRef program = utils::make<ir::compat::Program>("TestProgram", platform, 7, 32, 10);
  utils::Options options{};
  pmgr::pass_types::Context context{"TestPass", "TestOutputPrefix", options};
  const utils::UInt numberQubits = 5;

public:
  void checkGates(std::initializer_list<utils::Str> expectedGates) { 

    //fixme: in .cc file
    ASSERT_EQ(expectedGates.size(), kernel->gates.size());
    
    utils::UInt index = 0;
    for (auto& expectedGate: expectedGates) {
      EXPECT_EQ(expectedGate, kernel->gates[index]->qasm());
      ++index;
    }
  };

  ir::compat::KernelRef kernel = utils::make<ir::compat::Kernel>(
            "TestKernel",
            platform,
            numberQubits);

  utils::Int run() {
    return victim.run(program, kernel, context);
  }
};

} // namespace pass
} // namespace ql