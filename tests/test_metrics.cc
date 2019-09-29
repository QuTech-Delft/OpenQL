#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>

#include <time.h>

#include <ql/openql.h>

#include "ql/utils.h"
#include <fstream>

#include <iostream>
#include <vector>


using namespace std;


void
test_oneD4(std::string v, std::string mapopt, std::string scheduler_commuteopt, std::string maplookaheadopt)
{
    int n = 5;
    std::string prog_name = "test_" + v + "_mapopt=" + mapopt + "_scheduler_commute=" + scheduler_commuteopt + "_maplookahead=" + maplookaheadopt;
    std::string kernel_name = "test_" + v + "_mapopt=" + mapopt + "_scheduler_commute=" + scheduler_commuteopt + "_maplookahead=" + maplookaheadopt;
    float sweep_points[] = { 1 };

    ql::quantum_platform starmon("starmon", "test_mapper.json");
    ql::set_platform(starmon);
    ql::quantum_program prog(prog_name, starmon, n, 0);
    ql::quantum_kernel k(kernel_name, starmon, n, 0);
    prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

    k.gate("x", 2);
    k.gate("x", 4);

    // one cnot, but needs several swaps
    k.gate("cnot", 2,4);

    k.gate("x", 2);
    k.gate("x", 4);

    prog.add(k);

    ql::utils::logger::set_log_level("LOG_INFO");
    ql::options::set("mapper", mapopt);
    ql::options::set("scheduler_commute", scheduler_commuteopt);
    ql::options::set("maplookahead", maplookaheadopt);
    prog.compile( );
/*
    std::string fidelity_estimator = "bounded_fidelity";
    std::string output_mode = "worst";


    ql::metrics::Metrics estimator(n, 0.999, 0.99, 4500/20, fidelity_estimator, output_mode);
    std::vector<double> fids;
    IOUT(k.qasm());
    double result = estimator.bounded_fidelity(k.c, fids);
    IOUT("Fidelity ("+ output_mode + ") = " << result);

*/  ql::quantum_kernel k1 = prog.kernels.at(0);

    IOUT(k1.qasm());
    for (auto gate : k1.c )
        IOUT("Gate " + gate->name + "(" +  to_string(gate->operands.at(0)) + ") at cycle " + to_string(gate->cycle) );

    // ofstream myfile;
    // myfile.open ("C:/Users/Diogo/Desktop/fidelity.txt");
    // // Google Drive/TUDelft/THESIS/Code/MYCODE/OpenQL/cbuild/tests/test_output/
    // myfile <<  std::to_string(result);
    // myfile.close();

}


int main(int argc, char ** argv)
{
    ql::utils::logger::set_log_level("LOG_DEBUG");
    ql::options::set("scheduler", "ALAP");
    ql::options::set("mapinitone2one", "yes"); 
    ql::options::set("initialplace", "no"); 
    ql::options::set("mapusemoves", "yes"); 
    ql::options::set("maptiebreak", "first"); 
    ql::options::set("mappathselect", "all"); 
    ql::options::set("mapdecomposer", "no");

    test_oneD4("oneD4", "base", "yes", "critical");

    // ql::metrics::Depolarizing_model estimator(5, .999, .99 );




    return 0;
}
