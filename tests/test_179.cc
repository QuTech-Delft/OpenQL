#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>

#include <time.h>

#include <openql.h>



// all cnots with operands that are neighbors in s7
// no or hardly any significant difference between pre179 and post179 scheduling
void
test_cnot_mixedcommute(std::string v, std::string schedopt, std::string sched_post179opt)
{
    int n = 7;
    std::string prog_name = "test_" + v + "_schedopt=" + schedopt + "_sched_post179opt=" + sched_post179opt;
    std::string kernel_name = "test_" + v + "_schedopt=" + schedopt + "_sched_post179opt=" + sched_post179opt;
    double sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_179.json");
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(double));

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
    double sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_179.json");
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(double));

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
    double sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_179.json");
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(double));

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
    double sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_179.json");
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(double));

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

test_cnot_controlcommute("cnot_controlcommute", "ASAP", "no");
test_cnot_controlcommute("cnot_controlcommute", "ASAP", "yes");
test_cnot_controlcommute("cnot_controlcommute", "ALAP", "no");
test_cnot_controlcommute("cnot_controlcommute", "ALAP", "yes");
test_cnot_targetcommute("cnot_targetcommute", "ASAP", "no");
test_cnot_targetcommute("cnot_targetcommute", "ASAP", "yes");
test_cnot_targetcommute("cnot_targetcommute", "ALAP", "no");
test_cnot_targetcommute("cnot_targetcommute", "ALAP", "yes");
test_cz_anycommute("cz_anycommute", "ASAP", "no");
test_cz_anycommute("cz_anycommute", "ASAP", "yes");
test_cz_anycommute("cz_anycommute", "ALAP", "no");
test_cz_anycommute("cz_anycommute", "ALAP", "yes");
test_cnot_mixedcommute("cnot_mixedcommute", "ASAP", "no");
test_cnot_mixedcommute("cnot_mixedcommute", "ASAP", "yes");
test_cnot_mixedcommute("cnot_mixedcommute", "ALAP", "no");
test_cnot_mixedcommute("cnot_mixedcommute", "ALAP", "yes");


    return 0;
}
