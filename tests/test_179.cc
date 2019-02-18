#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>

#include <time.h>

#include <ql/openql.h>

// test qwg resource constraints mapping
// no difference between pre179 and post179 scheduling
void
test_qwg(std::string v, std::string schedopt, std::string sched_post179opt)
{
    int n = 2;
    std::string prog_name = "test_" + v + "_schedopt=" + schedopt + "_sched_post179opt=" + sched_post179opt;
    std::string kernel_name = "test_" + v + "_schedopt=" + schedopt + "_sched_post179opt=" + sched_post179opt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_179.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    // no dependency, only a conflict in qwg resource
    k.gate("x", 0);
    k.gate("y", 1);

    prog.add(k);

    ql::options::set("scheduler", schedopt);
    ql::options::set("scheduler_post179", sched_post179opt);
    prog.compile( );
}

// demo single dimension resource constraint representation simple
void
test_singledim(std::string v, std::string schedopt, std::string sched_post179opt)
{
    int n = 5;
    std::string prog_name = "test_" + v + "_schedopt=" + schedopt + "_sched_post179opt=" + sched_post179opt;
    std::string kernel_name = "test_" + v + "_schedopt=" + schedopt + "_sched_post179opt=" + sched_post179opt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_179.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    // independent gates but interfering qwg unit use
    // in surface-7, q2, q3 and q4 all use qwg1;
    // the y q3 must be in an other cycle than both x's because x conflicts with y in qwg1 (different gates);
    //
    // the x q2 and x q4 can be in parallel but the y q3 in between prohibits this pre179
    // because the scheduler doesn't look ahead for operations that can be done in a same cycle:
    // after x q2 the qwg1 resource is busy on x in cycle 0,
    // then the scheduler only looks at the y q3, which requires to go to cycle 1 because of qwg1 being busy;
    // and then for the x q4 the scheduler only looks at the current cycle (cycle 1),
    // in which qwg1 is busy with the y, so for the x it is busy,
    // and the only option is to delay that x q4 to cycle 2;
    //
    // post179, the scheduler looks at the dep graph and sees all 3 operations to be ready for scheduling,
    // i.e. any order would be ok when not taking resources into account;
    // when the x q2 would be scheduled in cycle 0, it considers doing y q3 and x q4 in the same cycle;
    // for y q3 this fails on the qwg1 resource but for x q4 this is ok because it uses the same gate as x q2;
    // so x q2 and x q4 are done in cycle 0; y q3 is then put in cycle 1
    k.gate("x", 2);
    k.gate("y", 3);
    k.gate("x", 4);

    prog.add(k);

    ql::options::set("scheduler", schedopt);
    ql::options::set("scheduler_post179", sched_post179opt);
    prog.compile( );
}

// test edge resource constraints mapping
// no difference between pre179 and post179 scheduling
void
test_edge(std::string v, std::string schedopt, std::string sched_post179opt)
{
    int n = 5;
    std::string prog_name = "test_" + v + "_schedopt=" + schedopt + "_sched_post179opt=" + sched_post179opt;
    std::string kernel_name = "test_" + v + "_schedopt=" + schedopt + "_sched_post179opt=" + sched_post179opt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_179.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    // no dependency, only a conflict in edge resource
    k.gate("cz", 1,4);
    k.gate("cz", 0,3);

    prog.add(k);

    ql::options::set("scheduler", schedopt);
    ql::options::set("scheduler_post179", sched_post179opt);
    prog.compile( );
}

// test detuned_qubits resource constraints mapping
// no swaps generated
// no difference between pre179 and post179 scheduling
void
test_detuned(std::string v, std::string schedopt, std::string sched_post179opt)
{
    int n = 5;
    std::string prog_name = "test_" + v + "_schedopt=" + schedopt + "_sched_post179opt=" + sched_post179opt;
    std::string kernel_name = "test_" + v + "_schedopt=" + schedopt + "_sched_post179opt=" + sched_post179opt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_179.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    // preferably cz's parallel, but not with x 3
    k.gate("cz", 0,2);
    k.gate("cz", 1,4);
    k.gate("x", 3);

    // likewise, while y 3, no cz on 0,2 or 1,4
    k.gate("y", 3);
    k.gate("cz", 0,2);
    k.gate("cz", 1,4);

    prog.add(k);

    ql::options::set("scheduler", schedopt);
    ql::options::set("scheduler_post179", sched_post179opt);
    prog.compile( );
}

// one cnot with operands that are neighbors in s7
// no difference between pre179 and post179 scheduling
void
test_oneNN(std::string v, std::string schedopt, std::string sched_post179opt)
{
    int n = 3;
    std::string prog_name = "test_" + v + "_schedopt=" + schedopt + "_sched_post179opt=" + sched_post179opt;
    std::string kernel_name = "test_" + v + "_schedopt=" + schedopt + "_sched_post179opt=" + sched_post179opt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_179.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    k.gate("x", 0);
    k.gate("x", 2);

    // one cnot that is ok in trivial mapping
    k.gate("cnot", 0,2);

    k.gate("x", 0);
    k.gate("x", 2);

    prog.add(k);

    ql::options::set("scheduler", schedopt);
    ql::options::set("scheduler_post179", sched_post179opt);
    prog.compile( );
}

// test hilo bundles
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

// steane qec on s7 with cnots
void
test_steaneqec(std::string v, std::string schedopt, std::string sched_post179opt)
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

    k.gate("prepz", 3);
    k.gate("prepz", 5);
    k.gate("h", 5);
    k.gate("cnot", 5, 3);
    k.gate("cnot", 0, 3);
    k.gate("cnot", 1, 3);
    k.gate("cnot", 6, 3);
    k.gate("cnot", 2, 5);
    k.gate("cnot", 5, 3);
    k.gate("h", 5);
    k.gate("measure", 3);
    k.gate("measure", 5);

    prog.add(k);
    
    ql::options::set("scheduler", schedopt);
    ql::options::set("scheduler_post179", sched_post179opt);
    prog.compile( );
}

// all cnots with operands that are neighbors in s7
// no or hardly any significant difference between pre179 and post179 scheduling,
// slight differences may occur when the json file maps cnot to its constituent primitive gates
void
test_manyNN(std::string v, std::string schedopt, std::string sched_post179opt)
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

    // for (int j=0; j<7; j++) { k.gate("x", j); }

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

    // for (int j=0; j<7; j++) { k.gate("x", j); }

    prog.add(k);

    ql::options::set("scheduler", schedopt);
    ql::options::set("scheduler_post179", sched_post179opt);
    prog.compile( );
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

    // for (int j=0; j<7; j++) { k.gate("x", j); }

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

    // for (int j=0; j<7; j++) { k.gate("x", j); }

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

    // for (int j=0; j<7; j++) { k.gate("x", j); }

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

    // for (int j=0; j<7; j++) { k.gate("x", j); }

    prog.add(k);

    ql::options::set("scheduler", schedopt);
    ql::options::set("scheduler_post179", sched_post179opt);
    prog.compile( );
}

int main(int argc, char ** argv)
{
    ql::utils::logger::set_log_level("LOG_DEBUG");
    ql::options::set("scheduler_uniform", "no");

    test_singledim("singledim", "ASAP", "no");
    test_singledim("singledim", "ASAP", "yes");
    test_singledim("singledim", "ALAP", "no");
    test_singledim("singledim", "ALAP", "yes");
    test_qwg("qwg", "ASAP", "no");
    test_qwg("qwg", "ASAP", "yes");
    test_qwg("qwg", "ALAP", "no");
    test_qwg("qwg", "ALAP", "yes");
    test_edge("edge", "ASAP", "no");
    test_edge("edge", "ASAP", "yes");
    test_edge("edge", "ALAP", "no");
    test_edge("edge", "ALAP", "yes");
    test_detuned("detuned", "ASAP", "no");
    test_detuned("detuned", "ASAP", "yes");
    test_detuned("detuned", "ALAP", "no");
    test_detuned("detuned", "ALAP", "yes");
    test_oneNN("oneNN", "ASAP", "no");
    test_oneNN("oneNN", "ASAP", "yes");
    test_oneNN("oneNN", "ALAP", "no");
    test_oneNN("oneNN", "ALAP", "yes");
    test_hilo("hilo", "ASAP", "no");
    test_hilo("hilo", "ASAP", "yes");
    test_hilo("hilo", "ALAP", "no");
    test_hilo("hilo", "ALAP", "yes");
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
    test_steaneqec("steaneqec", "ASAP", "no");
    test_steaneqec("steaneqec", "ASAP", "yes");
    test_steaneqec("steaneqec", "ALAP", "no");
    test_steaneqec("steaneqec", "ALAP", "yes");
    test_manyNN("manyNN", "ASAP", "no");
    test_manyNN("manyNN", "ASAP", "yes");
    test_manyNN("manyNN", "ALAP", "no");
    test_manyNN("manyNN", "ALAP", "yes");

    ql::options::set("scheduler_uniform", "yes");
    test_hilo("hilo_uniform", "ALAP", "no");
    test_hilo("hilo_uniform", "ALAP", "yes");

    return 0;
}
