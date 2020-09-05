#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>

#include <time.h>

#include <openql.h>


// all tests below should be run with scheduler_post179 yes and no
// when scheduler_post179 is yes, also scheduler_commute should be yes (but that is default)
// the tests that start with #ifdef NEED_NOT_BE_CONVERTED_TO_A_GOLDEN_TEST_IN_PYTHON need not be converted to golden tests
#define NEED_NOT_BE_CONVERTED_TO_A_GOLDEN_TEST_IN_PYTHON




// test hilo bundles
// tests uniform scheduling in the presence of pre179/post179
void
test_hilo(std::string v, std::string schedopt, std::string sched_post179opt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_schedopt=" + schedopt + "_sched_post179opt=" + sched_post179opt;
    std::string kernel_name = "test_" + v + "_schedopt=" + schedopt + "_sched_post179opt=" + sched_post179opt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_179.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    for (int j=0; j<7; j++) { k.gate("x", j); }
    k.gate("cz", 0, 3);
    k.gate("cz", 4, 6);
    k.gate("cz", 3, 6);
    k.gate("cz", 2, 5);

    prog.add(k);

    ql::options::set("scheduler", schedopt);
    ql::options::set("scheduler_post179", sched_post179opt);
    prog.compile( );
}


// test wait as gate
void
test_wait(std::string v, std::string schedopt, std::string sched_post179opt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_schedopt=" + schedopt + "_sched_post179opt=" + sched_post179opt;
    std::string kernel_name = "test_" + v + "_schedopt=" + schedopt + "_sched_post179opt=" + sched_post179opt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_179.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    std::vector<size_t> operands = {0};

    k.gate("x", 0);
    k.wait(operands, 40);
    k.gate("x", 0);

    prog.add(k);

    ql::options::set("scheduler", schedopt);
    ql::options::set("scheduler_post179", sched_post179opt);
    prog.compile( );
}

// all cnots with operands that are neighbors in s7
// no or hardly any significant difference between pre179 and post179 scheduling
void
test_cnot_mixedcommute(std::string v, std::string schedopt, std::string sched_post179opt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_schedopt=" + schedopt + "_sched_post179opt=" + sched_post179opt;
    std::string kernel_name = "test_" + v + "_schedopt=" + schedopt + "_sched_post179opt=" + sched_post179opt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_179.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

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

    prog.add(k);

    ql::options::set("scheduler", schedopt);
    ql::options::set("scheduler_post179", sched_post179opt);
    prog.compile( );
}

// test cnot control operand commutativity
// i.e. best result is the reverse original order
void
test_cnot_controlcommute(std::string v, std::string schedopt, std::string sched_post179opt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_schedopt=" + schedopt + "_sched_post179opt=" + sched_post179opt;
    std::string kernel_name = "test_" + v + "_schedopt=" + schedopt + "_sched_post179opt=" + sched_post179opt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_179.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

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

    prog.add(k);

    ql::options::set("scheduler", schedopt);
    ql::options::set("scheduler_post179", sched_post179opt);
    prog.compile();
}

// test cnot target operand commutativity
// i.e. best result is the reverse original order
void
test_cnot_targetcommute(std::string v, std::string schedopt, std::string sched_post179opt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_schedopt=" + schedopt + "_sched_post179opt=" + sched_post179opt;
    std::string kernel_name = "test_" + v + "_schedopt=" + schedopt + "_sched_post179opt=" + sched_post179opt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_179.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

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

    prog.add(k);

    ql::options::set("scheduler", schedopt);
    ql::options::set("scheduler_post179", sched_post179opt);
    prog.compile( );
}

// test cz any operand commutativity
// i.e. best result is the reverse original order
void
test_cz_anycommute(std::string v, std::string schedopt, std::string sched_post179opt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_schedopt=" + schedopt + "_sched_post179opt=" + sched_post179opt;
    std::string kernel_name = "test_" + v + "_schedopt=" + schedopt + "_sched_post179opt=" + sched_post179opt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_179.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

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

    prog.add(k);

    ql::options::set("scheduler", schedopt);
    ql::options::set("scheduler_post179", sched_post179opt);
    prog.compile( );
}

int main(int argc, char ** argv)
{
    ql::utils::logger::set_log_level("LOG_DEBUG");
    ql::options::set("scheduler_uniform", "no");

//  test_singledim("singledim", "ASAP", "no");
//  test_singledim("singledim", "ASAP", "yes");
//  test_singledim("singledim", "ALAP", "no");
//  test_singledim("singledim", "ALAP", "yes");
//  test_qwg("qwg", "ASAP", "no");
//  test_qwg("qwg", "ASAP", "yes");
//  test_qwg("qwg", "ALAP", "no");
//  test_qwg("qwg", "ALAP", "yes");
//  test_edge("edge", "ASAP", "no");
//  test_edge("edge", "ASAP", "yes");
//  test_edge("edge", "ALAP", "no");
//  test_edge("edge", "ALAP", "yes");
//  test_detuned("detuned", "ASAP", "no");
//  test_detuned("detuned", "ASAP", "yes");
//  test_detuned("detuned", "ALAP", "no");
//  test_detuned("detuned", "ALAP", "yes");
//  test_oneNN("oneNN", "ASAP", "no");
//  test_oneNN("oneNN", "ASAP", "yes");
//  test_oneNN("oneNN", "ALAP", "no");
//  test_oneNN("oneNN", "ALAP", "yes");
//  test_hilo("hilo", "ASAP", "no");
//  test_hilo("hilo", "ASAP", "yes");
//  test_hilo("hilo", "ALAP", "no");
//  test_hilo("hilo", "ALAP", "yes");
//  test_cnot_controlcommute("cnot_controlcommute", "ASAP", "no");
//  test_cnot_controlcommute("cnot_controlcommute", "ASAP", "yes");
//  test_cnot_controlcommute("cnot_controlcommute", "ALAP", "no");
//  test_cnot_controlcommute("cnot_controlcommute", "ALAP", "yes");
//  test_cnot_targetcommute("cnot_targetcommute", "ASAP", "no");
//  test_cnot_targetcommute("cnot_targetcommute", "ASAP", "yes");
//  test_cnot_targetcommute("cnot_targetcommute", "ALAP", "no");
//  test_cnot_targetcommute("cnot_targetcommute", "ALAP", "yes");
//  test_cz_anycommute("cz_anycommute", "ASAP", "no");
//  test_cz_anycommute("cz_anycommute", "ASAP", "yes");
//  test_cz_anycommute("cz_anycommute", "ALAP", "no");
//  test_cz_anycommute("cz_anycommute", "ALAP", "yes");
//  test_steaneqec("steaneqec", "ASAP", "no");
//  test_steaneqec("steaneqec", "ASAP", "yes");
//  test_steaneqec("steaneqec", "ALAP", "no");
//  test_steaneqec("steaneqec", "ALAP", "yes");
//  test_cnot_mixedcommute("cnot_mixedcommute", "ASAP", "no");
//  test_cnot_mixedcommute("cnot_mixedcommute", "ASAP", "yes");
//  test_cnot_mixedcommute("cnot_mixedcommute", "ALAP", "no");
//  test_cnot_mixedcommute("cnot_mixedcommute", "ALAP", "yes");

    test_wait("wait", "ASAP", "no");
    test_wait("wait", "ASAP", "yes");
    test_wait("wait", "ALAP", "no");
    test_wait("wait", "ALAP", "yes");

//  ql::options::set("scheduler_uniform", "yes");
//  test_hilo("hilo_uniform", "ALAP", "no");
//  test_hilo("hilo_uniform", "ALAP", "yes");

    return 0;
}
