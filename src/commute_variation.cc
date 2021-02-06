/** \file
 * Find circuit variations from commutable sets of gates and select shortest.
 *
 * \see commute_variation.h
 */

#include "commute_variation.h"

#include "utils/num.h"
#include "utils/list.h"
#include "utils/map.h"
#include "utils/filesystem.h"
#include "scheduler.h"
#include "options.h"
#include "report.h"

namespace ql {

using namespace utils;

/*
    The generation of all variations is done as follows:
    - At each node in the dependence graph, check its incoming dependences
      whether this node is such a first non-Zrotate or first non-Xrotate use;
      those incoming dependences are ordered by their dependence type and their cause (the qubit causing the dependence),
      - when DAZ/XAZ then we have commutation on a Zrotate operand (1st operand of CNOT, both operands of CZ),
        the cause represents the operand qubit
      - when DAX/ZAX then we have commutation on an Xrotate operand (2nd operand of CNOT),
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
      in the depgraph this order can be enforced by adding to the depgraph ZAZ (for sets of Control-Unitaries)
      or XAX (for sets of CNOT 2nd operand commutable gates) dependences between the gates in the set, from first to last.
    - Then for each variation:
      - the dependences are added
      - tested whether the dependence graph is still acyclic; when the dependence graph became cyclic
        after having added the ZAZ/XAX dependences, some commutable sets were interfering, i.e. there were
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
typedef UInt VarCode;

// Scheduler class extension with entries to find the variations based on the dependence graph.
class Depgraph : public Scheduler {
private:
    // variation encoding multiply function aiming to catch overflow
    VarCode mult(VarCode a, VarCode b) {
        VarCode    r = a * b;
        if (r < a || r < b) {
            QL_FATAL("Error: number of variations more than fits in unsigned long");
        }
        return r;
    }

public:
    // after scheduling, delete the added arcs (ZAZ/XAX) from the depgraph to restore it to the original state
    void clean_variation( List<lemon::ListDigraph::Arc>& newarcslist) {
        for (auto a : newarcslist) {
            QL_DOUT("...... erasing arc with id " << graph.id(a) << " from " << instruction[graph.source(a)]->qasm() << " to " << instruction[graph.target(a)]->qasm() << " as " << DepTypeName[depType[a]] << " by q" << cause[a]);
            graph.erase(a);
        }
        newarcslist.clear();
    }

    // return encoding of variation var as a string for debugging output
    Str varstring(
        List<List<lemon::ListDigraph::Arc>>& varslist,
        VarCode var
    ) {
        StrStrm ss;

        Int varslist_index = 1;
        for (auto subvarslist : varslist) {
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
    // the sequentialization is done by adding ZAZ/XAX dependences to the dependence graph;
    // those are kept for removal again from the depgraph after scheduling
    // the original varslist is copied locally to a recipe_varslist that is gradually reduced to empty while generating
    void gen_variation(
        List<List<lemon::ListDigraph::Arc>>& varslist,
        List<lemon::ListDigraph::Arc>& newarcslist,
        VarCode var
    ) {
        List<List<lemon::ListDigraph::Arc>> recipe_varslist = varslist;  // deepcopy, i.e. must copy each sublist of list as well
        QL_DOUT("... variation " << var << " (" << varstring(varslist,var) << "):");
        QL_DOUT("... recipe_varslist.size()=" << recipe_varslist.size());
        Int varslist_index = 1;
        for (auto subvarslist : recipe_varslist) {
            QL_DOUT("... subvarslist index=" << varslist_index << " subvarslist.size()=" << subvarslist.size());
            Bool prevvalid = false;             // add arc between each pair of nodes so skip 1st arc in subvarslist
            lemon::ListDigraph::Node    prevn = s;     // previous node when prevvalid==true; fake initialization by s
            for (auto svs = subvarslist.size(); svs != 0; svs--) {
                auto thisone = var % svs;     // gives 0 <= thisone < subvarslist.size()
                QL_DOUT("...... var=" << var << " svs=" << svs << " thisone=var%svs=" << thisone << " nextvar=var/svs=" << var/svs);
                auto li = subvarslist.begin();
                std::advance(li, thisone);
                lemon::ListDigraph::Arc a = *li;   // i.e. select the thisone's element in this subvarslist
                lemon::ListDigraph::Node n  = graph.source(a);
                QL_DOUT("...... set " << varslist_index << " take " << thisone << ": " << instruction[n]->qasm() << " as " << DepTypeName[depType[a]] << " by q" << cause[a]);
                if (prevvalid) {
                    QL_DOUT("...... adding new arc from " << instruction[prevn]->qasm() << " to " << instruction[n]->qasm());
                    auto newarc = graph.addArc(prevn, n);
                    weight[newarc] = weight[a];
                    cause[newarc] = cause[a];
                    depType[newarc] = (depType[a] == DAZ ? ZAZ : XAX);
                    QL_DOUT("...... added new arc with id " << graph.id(newarc) << " from " << instruction[prevn]->qasm() << " to " << instruction[n]->qasm() << " as " << DepTypeName[depType[newarc]] << " by q" << cause[newarc]);
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
        List<lemon::ListDigraph::Arc>& arclist,
        List<List<lemon::ListDigraph::Arc>>& varslist,
        VarCode& var_count
    ) {
        while (arclist.size() > 1) {
            List<lemon::ListDigraph::Arc> TMParclist = arclist;
            Int  operand = cause[arclist.front()];
            TMParclist.remove_if([this,operand](lemon::ListDigraph::Arc a) { return cause[a] != operand; });
            if (TMParclist.size() > 1) {
                QL_DOUT("At " << instruction[graph.target(TMParclist.front())]->qasm() << " found commuting gates on q" << operand << ":");
                Int perm_index = 0;
                VarCode perm_count = 1;
                List<lemon::ListDigraph::Arc> subvarslist;
                for (auto a : TMParclist) {
                    lemon::ListDigraph::Node srcNode  = graph.source(a);
                    QL_DOUT("... " << instruction[srcNode]->qasm() << " as " << DepTypeName[depType[a]] << " by q" << cause[a]);
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
    void show_sets(List<List<lemon::ListDigraph::Arc>>& varslist) {
        VarCode var_count = 1;
        auto list_index = 1;
        for ( auto subvarslist : varslist ) {
            QL_DOUT("Commuting set " << list_index << ":");
            Int perm_index = 0;
            VarCode perm_count = 1;
            for (auto a : subvarslist) {
                lemon::ListDigraph::Node srcNode  = graph.source(a);
                QL_DOUT("... " << instruction[srcNode]->qasm() << " as " << DepTypeName[depType[a]] << " by q" << cause[a]);
                perm_index++;
                perm_count *= perm_index;
            }
            QL_DOUT("Giving rise to " << perm_count << " variations");
            var_count *= perm_count;
            list_index++;
        }
        QL_DOUT("Total " << var_count << " variations");
    }

    // for each node scan all incoming dependences
    // - when DAZ/XAZ then we have commutation on a Zrotate operand (1st operand of CNOT, both operands of CZ);
    //   those incoming dependences are collected in Zarclist and further split by their cause in add_variations
    // - when DAX/ZAX then we have commutation on an Xrotate operand (2nd operand of CNOT);
    //   those incoming dependences are collected in Xarclist and further split by their cause in add_variations
    void find_variations(
        List<List<lemon::ListDigraph::Arc>>& varslist,
        VarCode& total
    ) {
        for (lemon::ListDigraph::NodeIt n(graph); n != lemon::INVALID; ++n) {
            QL_DOUT("Incoming unfiltered dependences of node " << ": " << instruction[n]->qasm() << " :");
            List<lemon::ListDigraph::Arc> Zarclist;
            List<lemon::ListDigraph::Arc> Xarclist;
            for( lemon::ListDigraph::InArcIt arc(graph,n); arc != lemon::INVALID; ++arc ) {
                if (
                    depType[arc] == RAR
                    ||  depType[arc] == RAW
                    ||  depType[arc] == WAR
                    ||  depType[arc] == WAW
                    ||  depType[arc] == DAD
                    ||  depType[arc] == ZAD
                    ||  depType[arc] == XAD
                ) {
                    continue;
                }
                QL_DOUT("... Encountering relevant " << DepTypeName[depType[arc]] << " by q" << cause[arc] << " from " << instruction[graph.source(arc)]->qasm());
                if (
                    depType[arc] == DAZ
                    ||  depType[arc] == XAZ
                ) {
                    Zarclist.push_back(arc);
                }
                else if (
                    depType[arc] == DAX
                    ||  depType[arc] == ZAX
                ) {
                    Xarclist.push_back(arc);
                } else {
                    QL_FATAL("Unknown dependence type " << DepTypeName[depType[arc]] << " by q" << cause[arc] << " from " << instruction[graph.source(arc)]->qasm());
                }
            }
            add_variations(Zarclist, varslist, total);
            add_variations(Xarclist, varslist, total);
        }
    }

    // schedule the constructed depgraph for the platform with resource constraints and return the resulting depth
    UInt schedule_rc(const quantum_platform& platform) {
        Str schedopt = options::get("scheduler");
        Str dot;
        if ("ASAP" == schedopt) {
            arch::resource_manager_t rm(platform, forward_scheduling);
            schedule_asap(rm, platform, dot);
        }
        else if ("ALAP" == schedopt) {
            arch::resource_manager_t rm(platform, backward_scheduling);
            schedule_alap(rm, platform, dot);
        } else {
            QL_FATAL("Unknown scheduler");
        }

        // next code is copy of report::get_circuit_latency(); this function should be a circuit method
        UInt cycle_time = platform.cycle_time;
        UInt depth;;
        if (circp->empty() || circp->back()->cycle == MAX_CYCLE) {
            depth = 0;
        } else {
            depth = circp->back()->cycle + (circp->back()->duration+cycle_time-1)/cycle_time - circp->front()->cycle;
        }
        return depth;
    }
};

// generate variations and keep the one with the least depth in the current kernel's circuit
class commute_variation_c {
private:

    // print current circuit to a file in qasm format
    // use the variation number to create the file name
    // note that the scheduler has reordered the circuit's gates according to their assigned cycle value
    void print(
        const quantum_program *programp,
        quantum_kernel& kernel,
        VarCode varno
    ) {
        // next code is copy of what report::write_qasm does for one kernel; should be a circuit method
        StrStrm ss_output_file;
        ss_output_file << options::get("output_dir") << "/" << kernel.name << "_" << varno << ".qasm";
        QL_DOUT("... writing variation to '" << ss_output_file.str() << "' ...");
        StrStrm ss_qasm;
        ss_qasm << "." << kernel.name << "_" << varno << '\n';
        circuit &ckt = kernel.c;
        for (auto gp : ckt) {
            ss_qasm << '\t' << gp->qasm();
            // ss_qasm << "\t# " << gp->cycle
            ss_qasm << '\n';
        }
    
        // next code is copy of report::get_circuit_latency(); this function should be a circuit method
        UInt cycle_time = kernel.cycle_time;
        UInt depth;;
        if (ckt.empty() || ckt.back()->cycle == MAX_CYCLE) {
            depth = 0;
        } else {
            depth = ckt.back()->cycle + (ckt.back()->duration+cycle_time-1)/cycle_time - ckt.front()->cycle;
        }
        ss_qasm <<  "# Depth=" << depth << '\n';
        OutFile(ss_output_file.str()).write(ss_qasm.str());
    }

public:

    void generate(
        const quantum_program *programp,
        quantum_kernel& kernel,
        const quantum_platform & platform
    ) {
        QL_DOUT("Generate commutable variations of kernel circuit ...");
        circuit &ckt = kernel.c;
        if (ckt.empty()) {
            QL_DOUT("Empty kernel " << kernel.name);
            return;
        }
        if (options::get("scheduler_commute") == "no") {
            QL_COUT("Scheduler_commute option is \"no\": don't generate commutation variations");
            QL_DOUT("Scheduler_commute option is \"no\": don't generate commutation variations");
            return;
        }
    
        QL_DOUT("Create a dependence graph and recognize commutation");
        Depgraph sched;
        sched.init(ckt, platform, platform.qubit_number, kernel.creg_count, kernel.breg_count);
    
        QL_DOUT("Finding sets of commutable gates ...");
        List<List<lemon::ListDigraph::Arc>> varslist;
        VarCode total = 1;
        sched.find_variations(varslist, total);
        sched.show_sets(varslist);
    
        QL_DOUT("Start enumerating " << total << " variations ...");
        QL_DOUT("=========================\n\n");
    
        List<lemon::ListDigraph::Arc> newarcslist;        // new deps generated
        Map<UInt, List<VarCode>> vars_per_depth;
    
        for (VarCode varno = 0; varno < total; varno++) {
            // generate additional (ZAZ or XAX) dependences to sequentialize this variation
            sched.gen_variation(varslist, newarcslist, varno);
            if (!dag(sched.graph)) {
                // there are cycles among the dependences so this variation is infeasible
                QL_DOUT("... variation " << varno << " (" << sched.varstring(varslist,varno) << ") results in a dependence cycle, skip it");
            } else {
                QL_DOUT("... schedule variation " << varno << " (" << sched.varstring(varslist,varno) << ")");
                auto depth = sched.schedule_rc(platform);
                vars_per_depth.set(depth).push_back(varno);
                QL_DOUT("... scheduled variation " << varno << " (" << sched.varstring(varslist,varno) << "), depth=" << depth);
    
                // QL_DOUT("... printing qasm code for this variation " << varno << " (" << sched.varstring(varslist,varno) << ")");
                // print(programp, kernel, varno);
            }
            // delete additional dependences generated so restore old depgraph with all commutation
            sched.clean_variation(newarcslist);
            // QL_DOUT("... ready with variation " << varno << " (" << sched.varstring(varslist,varno) << ")");
            // QL_DOUT("=========================\n");
        }
        QL_DOUT("Generate commutable variations of kernel circuit [Done]");
    
        QL_DOUT("Find circuit with minimum depth while exploiting commutation");
        for (auto vit = vars_per_depth.begin(); vit != vars_per_depth.end(); ++vit) {
            QL_DOUT("... depth " << vit->first << ": " << vit->second.size() << " variations");
        }
        auto mit = vars_per_depth.begin();
        auto min_depth = mit->first;
        auto vars = mit->second;
        auto result_varno = vars.front();       // just the first one, could be more sophisticated
        QL_DOUT("Min depth=" << min_depth << ", number of variations=" << vars.size() << ", selected varno=" << result_varno);
    
        // Find out which depth heuristics would find
        auto hdepth = sched.schedule_rc(platform);
        QL_DOUT("Note that heuristics would find a schedule of the circuit with depth " << hdepth);
    
        // Set kernel.c representing result variation by regenerating it and scheduling it ...
        sched.gen_variation(varslist, newarcslist, result_varno);
        (void) sched.schedule_rc(platform); // sets kernel.c reflecting the additional deps of the variation
        sched.clean_variation(newarcslist);
        QL_DOUT("Find circuit with minimum depth while exploiting commutation [Done]");
    
        options::set("scheduler_commute", "no");    // next schedulers will respect the commutation order found
    }
};

static void commute_variation_kernel(
    quantum_program *programp,
    quantum_kernel &kernel,
    const quantum_platform &platform
) {
    QL_DOUT("Commute variation ...");
    if (!kernel.c.empty()) {
        if( options::get("vary_commutations") == "yes" ) {
            // find the shortest circuit by varying on gate commutation; replace kernel.c by it
            commute_variation_c   cv;
            cv.generate(programp, kernel, platform);
        }
    }

    QL_DOUT("Commute variation [DONE]");
}

void commute_variation(
    quantum_program *programp,              // updates the circuits of the program
    const quantum_platform &platform,
    const Str &passname
) {
    report_statistics(programp, platform, "in", passname, "# ");
    report_qasm(programp, platform, "in", passname);

    for (UInt k = 0; k < programp->kernels.size(); ++k) {
        commute_variation_kernel(programp, programp->kernels[k], platform);
    }

    report_statistics(programp, platform, "out", passname, "# ");
    report_qasm(programp, platform, "out", passname);
}

} // namespace ql

