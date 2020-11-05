/**
 * @file   commute_variation.cc
 * @date   02/2019
 * @author Hans van Someren
 * @brief  find circuit variations from commutable sets of gates and select shortest
 */

#include "commute_variation.h"

#include "utils.h"
#include "scheduler.h"
#include "options.h"
#include "report.h"

namespace ql {

/*
    The generation of all variations is done as follows:
    - At each node in the dependence graph, check its incoming dependences
      whether this node is such a first non-Read or first non-D use;
      those incoming dependences are ordered by their dependence type and their cause (the qubit causing the dependence),
      - when WAR/DAR then we have commutation on a Read operand (1st operand of CNOT, both operands of CZ),
        the cause represents the operand qubit
      - when WAD/RAD then we have commutation on a D operand (2nd operand of CNOT),
        the cause represents the operand qubit
      and the possibly several sets of commutable gates are filtered out from these incoming dependences.
      Each commutable set is represented by a list of arcs in the depgraph, i.e. arcs representing dependences
      from the node representing one of the commutable gates and to the gate with the first non-Read/D use.
      Note that in one set, of all incoming dependences the deptypes (WAR, DAR, WAD or RAD) must agree
      and the causes must agree.
      Each such set of commutable gates gives rise to a set of variations: all permutations of the gates.
      The number of those is the factorial of the size of the commutable set.
    - All these sets of commutable gates are stored in a list of such, the varslist.
      All sets together lead to a maximum number of variations that is the multiplication of those factorials.
      All variations can be enumerated by varying lexicographically
      through those combinations of permutations (a kind of goedelisation).
      One permutation of one commutable set stands for a particular order of the gates in the set;
      in the depgraph this order can be enforced by adding to the depgraph RAR (for sets of Control-Unitaries)
      or DAD (for sets of CNOT 2nd operand commutable gates) dependences between the gates in the set, from first to last.
    - Then for each variation:
      - the dependences are added
      - tested whether the dependence graph is still acyclic; when the dependence graph became cyclic
        after having added the RAR/DAD dependences, some commutable sets were interfering, i.e. there were
        additional dependences (on the other operands) between members of those commutable sets that enforce an order
        between particular pairs of members of those sets;
        when the dependence graph became cyclic, this variation is not feasible and can be skipped
      - a schedule is computed and its depth and variation number are kept
      - the schedule is optionally printed with the variation number in its name
      - and in any case then the added dependences are deleted so that the depgraph is restored to its original state.
    One of the variations with the least depth is stored in the current circuit as result of this variation search.
    Also, the scheduler_commute option is turned off so that future schedulers will respect the found order.
*/

// each variation is encoded in a number
typedef unsigned long   vc_t;

// Scheduler class extension with entries to find the variations based on the dependence graph.
class Depgraph : public Scheduler {
private:
    // variation encoding multiply function aiming to catch overflow
    vc_t mult(vc_t a, vc_t b) {
        vc_t    r = a * b;
        if (r < a || r < b) {
            FATAL("Error: number of variations more than fits in unsigned long");
        }
        return r;
    }

public:
    // after scheduling, delete the added arcs (RAR/DAD) from the depgraph to restore it to the original state
    void clean_variation( std::list<lemon::ListDigraph::Arc>& newarcslist) {
        for ( auto a : newarcslist) {
            // DOUT("...... erasing arc from " << instruction[graph.source(a)]->qasm() << " to " << instruction[graph.target(a)]->qasm() << " as " << DepTypesNames[depType[a]] << " by q" << cause[a]);
            graph.erase(a);
        }
        newarcslist.clear();
    }

    // return encoding of variation var as a string for debugging output
    std::string varstring(
        std::list<std::list<lemon::ListDigraph::Arc>>& varslist,
        vc_t var
    ) {
        std::ostringstream ss;

        int varslist_index = 1;
        for ( auto subvarslist : varslist ) {
            if (varslist_index != 1) {
                ss << "|";
            }
            for (auto svs = subvarslist.size(); svs != 0; svs--) {
                if (svs != subvarslist.size()) {
                    ss << "-";
                }
                auto thisone = var % svs;
                ss << thisone;
                var = var / svs;
            }
            varslist_index++;
        }
        return ss.str();
    }

    // make this variation effective by generating a sequentialization for the nodes in each subvarslist
    // the sequentialization is done by adding RAR/DAD dependences to the dependence graph;
    // those are kept for removal again from the depgraph after scheduling
    // the original varslist is copied locally to a recipe_varslist that is gradually reduced to empty while generating
    void gen_variation(
        std::list<std::list<lemon::ListDigraph::Arc>>& varslist,
        std::list<lemon::ListDigraph::Arc>& newarcslist,
        vc_t var
    ) {
        std::list<std::list<lemon::ListDigraph::Arc>> recipe_varslist = varslist;  // deepcopy, i.e. must copy each sublist of list as well
        DOUT("... variation " << var << " (" << varstring(varslist,var) << "):");
        // DOUT("... recipe_varslist.size()=" << recipe_varslist.size());
        int varslist_index = 1;
        for ( auto subvarslist : recipe_varslist ) {
            // DOUT("... subvarslist index=" << varslist_index << " subvarslist.size()=" << subvarslist.size());
            bool prevvalid = false;             // add arc between each pair of nodes so skip 1st arc in subvarslist
            lemon::ListDigraph::Node    prevn = s;     // previous node when prevvalid==true; fake initialization by s
            for (auto svs = subvarslist.size(); svs != 0; svs--) {
                auto thisone = var % svs;     // gives 0 <= thisone < subvarslist.size()
                // DOUT("...... var=" << var << " svs=" << svs << " thisone=var%svs=" << thisone << " nextvar=var/svs=" << var/svs);
                auto li = subvarslist.begin();
                std::advance(li, thisone);
                lemon::ListDigraph::Arc a = *li;   // i.e. select the thisone's element in this subvarslist
                lemon::ListDigraph::Node n  = graph.source(a);
                // DOUT("...... set " << varslist_index << " take " << thisone << ": " << instruction[n]->qasm() << " as " << DepTypesNames[depType[a]] << " by q" << cause[a]);
                if (prevvalid) {
                    // DOUT("...... adding arc from " << instruction[prevn]->qasm() << " to " << instruction[n]->qasm());
                    auto newarc = graph.addArc(prevn, n);
                    weight[newarc] = weight[a];
                    cause[newarc] = cause[a];
                    depType[newarc] = (depType[a] == WAR ? RAR : DAD);
                    // DOUT("...... added arc from " << instruction[prevn]->qasm() << " to " << instruction[n]->qasm() << " as " << DepTypesNames[depType[newarc]] << " by q" << cause[newarc]);
                    newarcslist.push_back(newarc);
                }
                prevvalid = true;
                prevn = n;
                subvarslist.erase(li);      // take thisone's element out of the subvarslist, reducing it to list one element shorter
                var = var / svs;              // take out the current subvarslist.size() out of the encoding
            }
            varslist_index++;
        }
    }

    // split the incoming dependences (in arclist) into a separate set for each qubit cause
    // at the same time, compute the size of the resulting sets and from that the number of variations it results in
    // the running total (var_count) is multiplied by each of these resulting numbers to give the total number of variations
    // the individual sets are added as separate lists to varlist, which is a list of those individual sets
    // the individual sets are implemented as lists and are called subvarslist here
    void add_variations(
        std::list<lemon::ListDigraph::Arc>& arclist,
        std::list<std::list<lemon::ListDigraph::Arc>>& varslist,
        vc_t& var_count
    ) {
        while (arclist.size() > 1) {
            std::list<lemon::ListDigraph::Arc> TMParclist = arclist;
            int  operand = cause[arclist.front()];
            TMParclist.remove_if([this,operand](lemon::ListDigraph::Arc a) { return cause[a] != operand; });
            if (TMParclist.size() > 1) {
                // DOUT("At " << instruction[graph.target(TMParclist.front())]->qasm() << " found commuting gates on q" << operand << ":");
                int perm_index = 0;
                vc_t perm_count = 1;
                std::list<lemon::ListDigraph::Arc> subvarslist;
                for ( auto a : TMParclist ) {
                    lemon::ListDigraph::Node srcNode  = graph.source(a);
                    // DOUT("... " << instruction[srcNode]->qasm() << " as " << DepTypesNames[depType[a]] << " by q" << cause[a]);
                    perm_index++;
                    perm_count = mult(perm_count, perm_index);
                    subvarslist.push_back(a);
                }
                varslist.push_back(subvarslist);
                var_count = mult(var_count, perm_count);
            }
            arclist.remove_if([this,operand](lemon::ListDigraph::Arc a) { return cause[a] == operand; });
        }
    }

    // show the sets of commutable gates for debugging
    void show_sets(std::list<std::list<lemon::ListDigraph::Arc>>& varslist) {
        vc_t var_count = 1;
        auto list_index = 1;
        for ( auto subvarslist : varslist ) {
            DOUT("Commuting set " << list_index << ":");
            int perm_index = 0;
            vc_t perm_count = 1;
            for (auto a : subvarslist) {
                lemon::ListDigraph::Node srcNode  = graph.source(a);
                DOUT("... " << instruction[srcNode]->qasm() << " as " << DepTypesNames[depType[a]] << " by q" << cause[a]);
                perm_index++;
                perm_count *= perm_index;
            }
            DOUT("Giving rise to " << perm_count << " variations");
            var_count *= perm_count;
            list_index++;
        }
        DOUT("Total " << var_count << " variations");
    }

    // for each node scan all incoming dependences
    // - when WAR/DAR then we have commutation on a Read operand (1st operand of CNOT, both operands of CZ);
    //   those incoming dependences are collected in Rarclist and further split by their cause in add_variations
    // - when WAD/RAD then we have commutation on a D operand (2nd operand of CNOT);
    //   those incoming dependences are collected in Darclist and further split by their cause in add_variations
    void find_variations(
        std::list<std::list<lemon::ListDigraph::Arc>>& varslist,
        vc_t& total
    ) {
        for (lemon::ListDigraph::NodeIt n(graph); n != lemon::INVALID; ++n) {
            // DOUT("Incoming unfiltered dependences of node " << ": " << instruction[n]->qasm() << " :");
            std::list<lemon::ListDigraph::Arc> Rarclist;
            std::list<lemon::ListDigraph::Arc> Darclist;
            for( lemon::ListDigraph::InArcIt arc(graph,n); arc != lemon::INVALID; ++arc ) {
                lemon::ListDigraph::Node srcNode  = graph.source(arc);
                if (
                    depType[arc] == WAW
                    ||  depType[arc] == RAW
                    ||  depType[arc] == DAW
                ) {
                    continue;
                }
                // DOUT("... Encountering relevant " << DepTypesNames[depType[arc]] << " by q" << cause[arc] << " from " << instruction[srcNode]->qasm());
                if (
                    depType[arc] == WAR
                    ||  depType[arc] == DAR
                ) {
                    Rarclist.push_back(arc);
                }
                else if (
                    depType[arc] == WAD
                    ||  depType[arc] == RAD
                ) {
                    Darclist.push_back(arc);
                }
            }
            add_variations(Rarclist, varslist, total);
            add_variations(Darclist, varslist, total);
        }
    }

    // schedule the constructed depgraph for the platform with resource constraints and return the resulting depth
    size_t schedule_rc(const ql::quantum_platform& platform) {
        std::string schedopt = ql::options::get("scheduler");
        std::string dot;
        if ("ASAP" == schedopt) {
            arch::resource_manager_t rm(platform, forward_scheduling);
            schedule_asap(rm, platform, dot);
        }
        else if ("ALAP" == schedopt) {
            arch::resource_manager_t rm(platform, backward_scheduling);
            schedule_alap(rm, platform, dot);
        } else {
            FATAL("Unknown scheduler");
        }

        // next code is copy of report::get_circuit_latency(); this function should be a circuit method
	    size_t cycle_time = platform.cycle_time;
	    size_t depth;;
	    if (circp->empty() || circp->back()->cycle == MAX_CYCLE) {
	        depth = 0;
	    } else {
	        depth = circp->back()->cycle + (circp->back()->duration+cycle_time-1)/cycle_time - circp->front()->cycle;
	    }
        return depth;
    }
};

// generate variations and keep the one with the least depth in the current kernel's circuit
class commute_variation_c
{
private:

    // print current circuit to a file in qasm format
    // use the variation number to create the file name
    // note that the scheduler has reordered the circuit's gates according to their assigned cycle value
	void print(
	    const ql::quantum_program *programp,
	    quantum_kernel& kernel,
	    vc_t varno
	) {
        // next code is copy of what report::write_qasm does for one kernel; should be a circuit method
	    std::stringstream ss_output_file;
	    ss_output_file << ql::options::get("output_dir") << "/" << kernel.name << "_" << varno << ".qasm";
	    DOUT("... writing variation to '" << ss_output_file.str() << "' ...");
	    std::stringstream ss_qasm;
	    ss_qasm << "." << kernel.name << "_" << varno << '\n';
	    ql::circuit& ckt = kernel.c;
	    for (auto gp : ckt) {
	        ss_qasm << '\t' << gp->qasm();
	        // ss_qasm << "\t# " << gp->cycle
	        ss_qasm << '\n';
	    }

        // next code is copy of report::get_circuit_latency(); this function should be a circuit method
	    size_t cycle_time = kernel.cycle_time;
	    size_t depth;;
	    if (ckt.empty() || ckt.back()->cycle == MAX_CYCLE) {
	        depth = 0;
	    } else {
	        depth = ckt.back()->cycle + (ckt.back()->duration+cycle_time-1)/cycle_time - ckt.front()->cycle;
	    }
	    ss_qasm <<  "# Depth=" << depth << '\n';
	    ql::utils::write_file(ss_output_file.str(), ss_qasm.str());
	}

public:

	void generate(
	    const ql::quantum_program *programp,
	    quantum_kernel& kernel,
	    const ql::quantum_platform & platform
	) {
	    DOUT("Generate commutable variations of kernel circuit ...");
	    ql::circuit& ckt = kernel.c;
	    if (ckt.empty()) {
	        DOUT("Empty kernel " << kernel.name);
	        return;
	    }
	    if (ql::options::get("scheduler_commute") == "no") {
	        COUT("Scheduler_commute option is \"no\": don't generate commutation variations");
	        DOUT("Scheduler_commute option is \"no\": don't generate commutation variations");
	        return;
	    }
	
	    DOUT("Create a dependence graph and recognize commutation");
	    Depgraph sched;
	    sched.init(ckt, platform, platform.qubit_number, kernel.creg_count);
	
	    DOUT("Finding sets of commutable gates ...");
	    std::list<std::list<lemon::ListDigraph::Arc>> varslist;
	    vc_t total = 1;
	    sched.find_variations(varslist, total);
	    sched.show_sets(varslist);
	
	    DOUT("Start enumerating " << total << " variations ...");
	    DOUT("=========================\n\n");
	
	    std::list<lemon::ListDigraph::Arc> newarcslist;        // new deps generated
	    std::map<size_t,std::list<vc_t>> vars_per_depth;
	
	    for (vc_t varno = 0; varno < total; varno++) {
	        // generate additional (RAR or DAD) dependences to sequentialize this variation
	        sched.gen_variation(varslist, newarcslist, varno);
	        if ( !dag(sched.graph))
	        {
	            // there are cycles among the dependences so this variation is infeasible
	            DOUT("... variation " << varno << " (" << sched.varstring(varslist,varno) << ") results in a dependence cycle, skip it");
	        }
	        else
	        {
	            // DOUT("... schedule variation " << varno << " (" << sched.varstring(varslist,varno) << ")");
	            auto depth = sched.schedule_rc(platform);
	            vars_per_depth[depth].push_back(varno);
	            DOUT("... scheduled variation " << varno << " (" << sched.varstring(varslist,varno) << "), depth=" << depth);
	
	            // DOUT("... printing qasm code for this variation " << varno << " (" << sched.varstring(varslist,varno) << ")");
	            // print(programp, kernel, varno);
	        }
	        // delete additional dependences generated so restore old depgraph with all commutation
	        sched.clean_variation(newarcslist);
	        // DOUT("... ready with variation " << varno << " (" << sched.varstring(varslist,varno) << ")");
	        // DOUT("=========================\n");
	    }
	    DOUT("Generate commutable variations of kernel circuit [Done]");
	
	    DOUT("Find circuit with minimum depth while exploiting commutation");
	    for (auto vit = vars_per_depth.begin(); vit != vars_per_depth.end(); ++vit) {
	        DOUT("... depth " << vit->first << ": " << vit->second.size() << " variations");
	    }
	    auto mit = vars_per_depth.begin();
	    auto min_depth = mit->first;
	    auto vars = mit->second;
	    auto result_varno = vars.front();       // just the first one, could be more sophisticated
	    DOUT("Min depth=" << min_depth << ", number of variations=" << vars.size() << ", selected varno=" << result_varno);
	
	    // Find out which depth heuristics would find
	    auto hdepth = sched.schedule_rc(platform);
	    DOUT("Note that heuristics would find a schedule of the circuit with depth " << hdepth);
	
	    // Set kernel.c representing result variation by regenerating it and scheduling it ...
	    sched.gen_variation(varslist, newarcslist, result_varno);
	    (void) sched.schedule_rc(platform); // sets kernel.c reflecting the additional deps of the variation
	    sched.clean_variation(newarcslist);
	    DOUT("Find circuit with minimum depth while exploiting commutation [Done]");
	
	    ql::options::set("scheduler_commute", "no");    // next schedulers will respect the commutation order found
	}
};

static void commute_variation_kernel(
    ql::quantum_program *programp,
    ql::quantum_kernel &kernel,
    const ql::quantum_platform &platform
) {
    DOUT("Commute variation ...");
    if (! kernel.c.empty()) {
        if( ql::options::get("vary_commutations") == "yes" ) {
            // find the shortest circuit by varying on gate commutation; replace kernel.c by it
            commute_variation_c   cv;
            cv.generate(programp, kernel, platform);
        }
    }

    DOUT("Commute variation [DONE]");
}

void commute_variation(
    ql::quantum_program *programp,              // updates the circuits of the program
    const ql::quantum_platform &platform,
    const std::string &passname
) {
    ql::report_statistics(programp, platform, "in", passname, "# ");
    ql::report_qasm(programp, platform, "in", passname);

    for (size_t k = 0; k < programp->kernels.size(); ++k) {
        commute_variation_kernel(programp, programp->kernels[k], platform);
    }

    ql::report_statistics(programp, platform, "out", passname, "# ");
    ql::report_qasm(programp, platform, "out", passname);
}

} // namespace ql

