#include <openql>

#include <gtest/gtest.h>
#include <string>
#include <vector>


// all cnots with operands that are neighbors in s7
// no or hardly any significant difference between pre179 and post179 scheduling
void test_cnot_mixed_commute(const std::string &v, const std::string &sched_opt, const std::string &sched_post179opt) {
    int n = 7;
    std::string prog_name = "test_" + v + "_sched_opt=" + sched_opt + "_sched_post179opt=" + sched_post179opt;
    std::string kernel_name = "test_" + v + "_sched_opt=" + sched_opt + "_sched_post179opt=" + sched_post179opt;

    auto starmon = ql::Platform("starmon",  "res/v1x/json/test_179.json");
    auto prog = ql::Program(prog_name, starmon, n, 0);
    auto k = ql::Kernel(kernel_name, starmon, n, 0);

    for (int j=0; j<7; j++) { k.gate("x", j); }

    // a list of all cnots that are ok in trivial mapping
    k.gate("cnot", 0,2);
    k.gate("cnot", 0,3);
    k.gate("cnot", 1,3);
    k.gate("cnot", 1,4);
    k.gate("cnot", 2,0);
    k.gate("cnot", 2,5);
    k.gate("cnot", 3,0);
    k.gate("cnot", 3,1);
    k.gate("cnot", 3,5);
    k.gate("cnot", 3,6);
    k.gate("cnot", 4,1);
    k.gate("cnot", 4,6);
    k.gate("cnot", 5,2);
    k.gate("cnot", 5,3);
    k.gate("cnot", 6,3);
    k.gate("cnot", 6,4);

    for (int j=0; j<7; j++) { k.gate("x", j); }

    prog.add_kernel(k);

    ql::set_option("scheduler", sched_opt);
    ql::set_option("scheduler_post179", sched_post179opt);
    prog.compile();
}

// test cnot control operand commutativity
// i.e. best result is the reverse original order
void test_cnot_control_commute(const std::string &v, const std::string &sched_opt, const std::string &sched_post179opt) {
    int n = 7;
    std::string prog_name = "test_" + v + "_sched_opt=" + sched_opt + "_sched_post179opt=" + sched_post179opt;
    std::string kernel_name = "test_" + v + "_sched_opt=" + sched_opt + "_sched_post179opt=" + sched_post179opt;

    auto starmon = ql::Platform("starmon", "res/v1x/json/test_179.json");
    auto prog = ql::Program(prog_name, starmon, n, 0);
    auto k = ql::Kernel(kernel_name, starmon, n, 0);

    k.gate("cnot", 3,0);
    k.gate("cnot", 3,6);
    k.gate("t", 6);
    k.gate("y", 6);
    k.gate("cnot", 3,1);
    k.gate("t", 1);
    k.gate("y", 1);
    k.gate("t", 1);
    k.gate("y", 1);
    k.gate("cnot", 3,5);
    k.gate("t", 5);
    k.gate("y", 5);
    k.gate("t", 5);
    k.gate("y", 5);
    k.gate("t", 5);
    k.gate("y", 5);

    prog.add_kernel(k);

    ql::set_option("scheduler", sched_opt);
    ql::set_option("scheduler_post179", sched_post179opt);
    prog.compile();
}

// test cnot target operand commutativity
// i.e. best result is the reverse original order
void test_cnot_target_commute(const std::string &v, const std::string &sched_opt, const std::string &sched_post179opt) {
    int n = 7;
    std::string prog_name = "test_" + v + "_sched_opt=" + sched_opt + "_sched_post179opt=" + sched_post179opt;
    std::string kernel_name = "test_" + v + "_sched_opt=" + sched_opt + "_sched_post179opt=" + sched_post179opt;

    auto starmon = ql::Platform("starmon", "res/v1x/json/test_179.json");
    auto prog = ql::Program(prog_name, starmon, n, 0);
    auto k = ql::Kernel(kernel_name, starmon, n, 0);

    k.gate("cnot", 0,3);
    k.gate("cnot", 6,3);
    k.gate("t", 6);
    k.gate("y", 6);
    k.gate("cnot", 1,3);
    k.gate("t", 1);
    k.gate("y", 1);
    k.gate("t", 1);
    k.gate("y", 1);
    k.gate("cnot", 5,3);
    k.gate("t", 5);
    k.gate("y", 5);
    k.gate("t", 5);
    k.gate("y", 5);
    k.gate("t", 5);
    k.gate("y", 5);

    prog.add_kernel(k);

    ql::set_option("scheduler", sched_opt);
    ql::set_option("scheduler_post179", sched_post179opt);
    prog.compile();
}

// test cz any operand commutativity
// i.e. best result is the reverse original order
void test_cz_any_commute(const std::string &v, const std::string &sched_opt, const std::string &sched_post179opt) {
    int n = 7;
    std::string prog_name = "test_" + v + "_sched_opt=" + sched_opt + "_sched_post179opt=" + sched_post179opt;
    std::string kernel_name = "test_" + v + "_sched_opt=" + sched_opt + "_sched_post179opt=" + sched_post179opt;

    auto starmon = ql::Platform("starmon", "res/v1x/json/test_179.json");
    auto prog = ql::Program(prog_name, starmon, n, 0);
    auto k = ql::Kernel(kernel_name, starmon, n, 0);

    k.gate("cz", 0,3);
    k.gate("cz", 3,6);
    k.gate("t", 6);
    k.gate("y", 6);
    k.gate("cz", 1,3);
    k.gate("t", 1);
    k.gate("y", 1);
    k.gate("t", 1);
    k.gate("y", 1);
    k.gate("cz", 3,5);
    k.gate("t", 5);
    k.gate("y", 5);
    k.gate("t", 5);
    k.gate("y", 5);
    k.gate("t", 5);
    k.gate("y", 5);

    prog.add_kernel(k);

    ql::set_option("scheduler", sched_opt);
    ql::set_option("scheduler_post179", sched_post179opt);
    prog.compile();
}

TEST(v1x, test_179) {
    ql::utils::logger::set_log_level("LOG_DEBUG");

    test_cnot_control_commute("cnot_control_commute", "ASAP", "no");
    test_cnot_control_commute("cnot_control_commute", "ASAP", "yes");
    test_cnot_control_commute("cnot_control_commute", "ALAP", "no");
    test_cnot_control_commute("cnot_control_commute", "ALAP", "yes");
    test_cnot_target_commute("cnot_target_commute", "ASAP", "no");
    test_cnot_target_commute("cnot_target_commute", "ASAP", "yes");
    test_cnot_target_commute("cnot_target_commute", "ALAP", "no");
    test_cnot_target_commute("cnot_target_commute", "ALAP", "yes");
    test_cz_any_commute("cz_any_commute", "ASAP", "no");
    test_cz_any_commute("cz_any_commute", "ASAP", "yes");
    test_cz_any_commute("cz_any_commute", "ALAP", "no");
    test_cz_any_commute("cz_any_commute", "ALAP", "yes");
    test_cnot_mixed_commute("cnot_mixed_commute", "ASAP", "no");
    test_cnot_mixed_commute("cnot_mixed_commute", "ASAP", "yes");
    test_cnot_mixed_commute("cnot_mixed_commute", "ALAP", "no");
    test_cnot_mixed_commute("cnot_mixed_commute", "ALAP", "yes");
}
