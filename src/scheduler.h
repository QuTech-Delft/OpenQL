/**
 * @file   scheduler.h
 * @date   01/2017
 * @author Imran Ashraf
 * @author Hans van Someren
 * @brief  ASAP/ALAP critical path and UNIFORM scheduling with and without resource constraint
 */

#pragma once

/*
    Summary

    Below there really are two classes: the dependence graph definition and the scheduler definition.
    All schedulers require dependence graph creation as preprocessor, and don't modify it.
    For each kernel's circuit a private dependence graph is created.
    The schedulers modify the order of gates in the circuit, initialize the cycle field of each gate,
    and generate/return the bundles, a list of bundles in which gates starting in the same cycle are grouped.

    The dependence graph (represented by the graph field below) is created in the Init method,
    and the graph is constructed from and referring to the gates in the sequence of gates in the kernel's circuit.
    In this graph, the nodes refer to the gates in the circuit, and the edges represent the dependences between two gates.
    Init scans the gates of the circuit from start to end, inspects their parameters, and for each gate
    depending on the gate type and parameter value and previous gates operating on the same parameters,
    it creates a dependence of the current gate on that previous gate.
    Such a dependence has a type (RAW, WAW, etc.), cause (the qubit or classical register used as parameter), and a weight
    (the cycles the previous gate takes to complete its execution, after which the current gate can start execution).

    In dependence graph creation, each qubit/classical register (creg) use in each gate is seen as an "event".
    The following events are distinguished:
    - W for Write: such a use must sequentialize with any previous and later uses of the same qubit/creg.
        This is the default for qubits in a gate and for assignment/modifications in classical code.
    - R for Read: such uses can be arbitrarily reordered (as long as other dependences allow that).
        This event applies to all operands of CZ, the first operand of CNOT gates, and to all reads in classical code.
        It also applies in general to the control operand of Control Unitaries.
        It represents commutativity between the gates which such use: CU(a,b), CZ(a,c), CZ(d,a) and CNOT(a,e) all commute.
    - D: such uses can be arbitrarily reordered but are sequentialized with W and R events on the same qubit.
        This event applies to the second operand of CNOT gates: CNOT(a,d) and CNOT(b,d) commute.
    With this, we effectively get the following table of event transitions (from left-bottom to right-up),
    in which 'no' indicates no dependence from left event to top event and '/' indicates a dependence from left to top.
             W   R   D                  w   R   D
        W    /   /   /              W   WAW RAW DAW
        R    /   no  /              R   WAR RAR DAR
        D    /   /   no             D   WAD RAD DAD
    When the 'no' dependences are created (RAR and/or DAD),
    the respective commutatable gates are sequentialized according to the original circuit's order.
    With all 'no's replaced by '/', all event types become equivalent (i.e. as if they were Write).

    Schedulers come essentially in the following forms:
    - ASAP: a plain forward scheduler using dependences only, aiming at execution each gate as soon as possible
    - ASAP with resource constraints: similar but taking resource constraints of the gates of the platform into account
    - ALAP: as ASAP but then aiming at execution of each gate as late as possible
    - ALAP with resource constraints: similar but taking resource constraints of the gates of the platform into account
    - ALAP with UNIFORM bundle lengths: using dependences only, aim at ALAP but with equally length bundles
    ASAP/ALAP can be controlled by the "scheduler" option. Similarly for UNIFORM ("scheduler_uniform").
    With/out resource constraints are separate method calls.

    Commutation support during scheduling in general produces more efficient/shorter scheduled circuits.
    It is enabled by option "scheduler_commute".
 */

#include <lemon/list_graph.h>
#include <lemon/lgf_reader.h>
#include <lemon/lgf_writer.h>
#include <lemon/dijkstra.h>
#include <lemon/connectivity.h>

#include "options.h"
#include "utils.h"
#include "gate.h"
#include "kernel.h"
#include "circuit.h"
#include "ir.h"
#include "resource_manager.h"
#include "report.h"

namespace ql {

// see above/below for the meaning of R, W, and D events and their relation to dependences
enum DepTypes{RAW, WAW, WAR, RAR, RAD, DAR, DAD, WAD, DAW};
const std::string DepTypesNames[] = {"RAW", "WAW", "WAR", "RAR", "RAD", "DAR", "DAD", "WAD", "DAW"};

class Scheduler {
public:
    // dependence graph is constructed (see Init) once from the sequence of gates in a kernel's circuit
    // it can be reused as often as needed as long as no gates are added/deleted; it doesn't modify those gates
    lemon::ListDigraph graph;

    // conversion between gate* (pointer to the gate in the circuit) and node (of the dependence graph)
    lemon::ListDigraph::NodeMap<gate*> instruction;// instruction[n] == gate*
    std::map<gate*, lemon::ListDigraph::Node>  node;// node[gate*] == n

    // attributes
    lemon::ListDigraph::NodeMap<std::string> name;     // name[n] == qasm string
    lemon::ListDigraph::ArcMap<int> weight;            // number of cycles of dependence
    lemon::ListDigraph::ArcMap<int> cause;             // qubit/creg index of dependence
    lemon::ListDigraph::ArcMap<int> depType;           // RAW, WAW, ...

    // s and t nodes are the top and bottom of the dependence graph
    lemon::ListDigraph::Node s, t;                     // instruction[s]==SOURCE, instruction[t]==SINK

    // parameters of dependence graph construction
    size_t          cycle_time;                        // to convert durations to cycles as weight of dependence
    size_t          qubit_count;                       // number of qubits, to check/represent qubit as cause of dependence
    size_t          creg_count;                        // number of cregs, to check/represent creg as cause of dependence
    circuit*    circp;                             // current and result circuit, passed from Init to each scheduler

    // scheduler support
    std::map<lemon::ListDigraph::Node,size_t>  remaining;  // remaining[node] == cycles until end; critical path representation

public:
    Scheduler();

    // ins->name may contain parameters, so must be stripped first before checking it for gate's name
    static void stripname(std::string &name);

    // factored out code from Init to add a dependence between two nodes
    // operand is in qubit_creg combined index space
    void add_dep(int srcID, int tgtID, enum DepTypes deptype, int operand);

    // fill the dependence graph ('graph') with nodes from the circuit and adding arcs for their dependences
    void init(
        circuit &ckt,
        const quantum_platform &platform,
        size_t qcount,
        size_t ccount
    );

    void print() const;
    void write_dependence_matrix() const;

private:


// =========== plain schedulers, just ASAP and ALAP, without RC

/*
    Summary

    The schedulers are linear list schedulers, i.e.
    - they scan linearly through the code, forward or backward
    - and while doing, they maintain a list of gates, of gates that are available for being scheduled
      because they are not blocked by dependences on non-scheduled gates.
    Therefore, the schedulers are able to select the best one from multiple available gates.
    Not all gates that are available (not blocked by dependences on non-scheduled gates) can actually be scheduled.
    It must be made sure in addition that:
    - those scheduled gates that it depends on, actually have completed their execution
    - the resources are available for it
    Furthermore, making a selection from the nodes that remain determines the optimality of the scheduling result.
    The schedulers below are critical path schedulers, i.e. they prefer to schedule the most critical node first.
    The criticality of a node is measured by estimating
    the effect of delaying scheduling it on the depth of the resulting circuit.

    The schedulers don't actually scan the circuit themselves but rely on a dependence graph representation of the circuit.
    At the start, depending on the scheduling direction, only the top (or bottom) node is available.
    Then one by one, according to an optimality criterion, a node is selected from the list of available ones
    and added to the schedule. Having scheduled the node, it is taken out of the available list;
    also having scheduled a node, some new nodes may become available because they don't depend on non-scheduled nodes anymore;
    those nodes are found and put in the available list of nodes.
    This continues, filling cycle by cycle from low to high (or from high to low when scheduling backward),
    until the available list gets empty (which happens after scheduling the last node, the bottom (or top when backward).
*/


public:
// use MAX_CYCLE for absolute upperbound on cycle value
// use ALAP_SINK_CYCLE for initial cycle given to SINK in ALAP;
// the latter allows for some growing room when doing latency compensation/buffer-delay insertion
#define ALAP_SINK_CYCLE    (MAX_CYCLE/2)

    // cycle assignment without RC depending on direction: forward:ASAP, backward:ALAP;
    // without RC, this is all there is to schedule, apart from forming the bundles in ir::bundler()
    // set_cycle iterates over the circuit's gates and set_cycle_gate over the dependences of each gate
    // please note that set_cycle_gate expects a caller like set_cycle which iterates gp forward through the circuit
    void set_cycle_gate(gate *gp, scheduling_direction_t dir);
    void set_cycle(scheduling_direction_t dir);

    // sort circuit by the gates' cycle attribute in non-decreasing order
    static void sort_by_cycle(circuit *cp);

    // ASAP scheduler without RC, setting gate cycle values and sorting the resulting circuit
    void schedule_asap(std::string &sched_dot);

    // ALAP scheduler without RC, setting gate cycle values and sorting the resulting circuit
    void schedule_alap(std::string &sched_dot);

// =========== schedulers with RC
    // Most code from here on deals with scheduling with Resource Constraints.
    // Then the cycles as computed from the depgraph alone start to drift because of resource conflicts,
    // and then it is more optimal to at each point consider all available nodes for scheduling
    // to avoid largely suboptimal results (issue 179), i.e. apply list scheduling.

    // In critical-path scheduling, usually more-critical instructions are preferred;
    // an instruction is more-critical when its ASAP and ALAP values differ less.
    // When scheduling with resource constraints, the ideal ASAP/ALAP cycle values cannot
    // be attained because of resource conflicts being in the way, they will 'slip',
    // so actual cycle values cannot be compared anymore to ideal ASAP/ALAP values to compute criticality;
    // but when forward (backward) scheduling, a lower ALAP (higher ASAP) indicates more criticality
    // (i.e. in ASAP scheduling use the ALAP values to know the criticality, and vice-versa);
    // those ALAP/ASAP are then a measure for number of cycles still to fill with gates in the schedule,
    // and are coined 'remaining' cycles here.
    //
    // remaining[node] indicates number of cycles remaining in schedule after start execution of node;
    //
    // Please note that for forward (backward) scheduling we use an
    // adaptation of the ALAP (ASAP) cycle computation to compute the remaining values; with this
    // definition both in forward and backward scheduling, a higher remaining indicates more criticality.
    // This means that criticality has become independent of the direction of scheduling
    // which is easier in the core of the scheduler.

    // Note that set_remaining_gate expects a caller like set_remaining that iterates gp backward over the circuit
    void set_remaining_gate(gate* gp, scheduling_direction_t dir);
    void set_remaining(scheduling_direction_t dir);
    gate* find_mostcritical(std::list<gate*>& lg);

    // ASAP/ALAP list scheduling support code with RC
    // Uses an "available list" (avlist) as interface between dependence graph and scheduler
    // the avlist contains all nodes that wrt their dependences can be scheduled:
    // when forward scheduling:
    //  all its predecessors were scheduled
    // when backward scheduling:
    //  all its successors were scheduled
    // The scheduler fills cycles one by one, with nodes/instructions from the avlist
    // checking before selection whether the nodes/instructions have completed execution
    // and whether the resource constraints are fulfilled.

    // Initialize avlist to the single starting node
    // when forward scheduling:
    //  node s (with SOURCE instruction) is the top of the dependence graph; all instructions depend on it
    // when backward scheduling:
    //  node t (with SINK instruction) is the bottom of the dependence graph; it depends on all instructions

    // Set the curr_cycle of the scheduling algorithm to start at the appropriate end as well;
    // note that the cycle attributes will be shifted down to start at 1 after backward scheduling.
    void init_available(
        std::list<lemon::ListDigraph::Node> &avlist,
        scheduling_direction_t dir,
        size_t &curr_cycle
    );

    // collect the list of directly depending nodes
    // (i.e. those necessarily scheduled after the given node) without duplicates;
    // dependences that are duplicates from the perspective of the scheduler
    // may be present in the dependence graph because the scheduler ignores dependence type and cause
    void get_depending_nodes(
        lemon::ListDigraph::Node n,
        scheduling_direction_t dir,
        std::list<lemon::ListDigraph::Node> &ln
    );

    // Compute of two nodes whether the first one is less deep-critical than the second, for the given scheduling direction;
    // criticality of a node is given by its remaining[node] value which is precomputed;
    // deep-criticality takes into account the criticality of depending nodes (in the right direction!);
    // this function is used to order the avlist in an order from highest deep-criticality to lowest deep-criticality;
    // it is the core of the heuristics of the critical path list scheduler.
    bool criticality_lessthan(
        lemon::ListDigraph::Node n1,
        lemon::ListDigraph::Node n2,
        scheduling_direction_t dir
    );

    // Make node n available
    // add it to the avlist because the condition for that is fulfilled:
    //  all its predecessors were scheduled (forward scheduling) or
    //  all its successors were scheduled (backward scheduling)
    // update its cycle attribute to reflect these dependences;
    // avlist is initialized with s or t as first element by init_available
    // avlist is kept ordered on deep-criticality, non-increasing (i.e. highest deep-criticality first)
    void MakeAvailable(
        lemon::ListDigraph::Node n,
        std::list<lemon::ListDigraph::Node> &avlist,
        scheduling_direction_t dir
    );

    // take node n out of avlist because it has been scheduled;
    // reflect that the node has been scheduled in the scheduled vector;
    // having scheduled it means that its depending nodes might become available:
    // such a depending node becomes available when all its dependent nodes have been scheduled now
    //
    // i.e. when forward scheduling:
    //   this makes its successor nodes available provided all their predecessors were scheduled;
    //   a successor node which has a predecessor which hasn't been scheduled,
    //   will be checked here at least when that predecessor is scheduled
    // i.e. when backward scheduling:
    //   this makes its predecessor nodes available provided all their successors were scheduled;
    //   a predecessor node which has a successor which hasn't been scheduled,
    //   will be checked here at least when that successor is scheduled
    //
    // update (through MakeAvailable) the cycle attribute of the nodes made available
    // because from then on that value is compared to the curr_cycle to check
    // whether a node has completed execution and thus is available for scheduling in curr_cycle
    void TakeAvailable(
        lemon::ListDigraph::Node n,
        std::list<lemon::ListDigraph::Node> &avlist,
        std::map<gate*,bool> &scheduled,
        scheduling_direction_t dir
    );

    // advance curr_cycle
    // when no node was selected from the avlist, advance to the next cycle
    // and try again; this makes nodes/instructions to complete execution for one more cycle,
    // and makes resources finally available in case of resource constrained scheduling
    // so it contributes to proceeding and to finally have an empty avlist
    static void AdvanceCurrCycle(scheduling_direction_t dir, size_t &curr_cycle);

    // a gate must wait until all its operand are available, i.e. the gates having computed them have completed,
    // and must wait until all resources required for the gate's execution are available;
    // return true when immediately schedulable
    // when returning false, isres indicates whether resource occupation was the reason or operand completion (for debugging)
    bool immediately_schedulable(
        lemon::ListDigraph::Node n,
        scheduling_direction_t dir,
        const size_t curr_cycle,
        const quantum_platform& platform,
        arch::resource_manager_t &rm,
        bool &isres
    );

    // select a node from the avlist
    // the avlist is deep-ordered from high to low criticality (see criticality_lessthan above)
    lemon::ListDigraph::Node SelectAvailable(
        std::list<lemon::ListDigraph::Node> &avlist,
        scheduling_direction_t dir,
        const size_t curr_cycle,
        const quantum_platform &platform,
        arch::resource_manager_t &rm,
        bool &success
    );

    // ASAP/ALAP scheduler with RC
    //
    // schedule the circuit that is in the dependence graph
    // for the given direction, with the given platform and resource manager;
    // what is done, is:
    // - the cycle attribute of the gates will be set according to the scheduling method
    // - *circp (the original and result circuit) is sorted in the new cycle order
    // the bundles are returned, with private start/duration attributes
    void schedule(
        circuit *circp,
        scheduling_direction_t dir,
        const quantum_platform &platform,
        arch::resource_manager_t &rm,
        std::string &sched_dot
    );

    void schedule_asap(
        arch::resource_manager_t &rm,
        const quantum_platform &platform,
        std::string &sched_dot
    );

    void schedule_alap(
        arch::resource_manager_t &rm,
        const quantum_platform &platform,
        std::string &sched_dot
    );

    void schedule_alap_uniform();

    // printing dot of the dependence graph
    void get_dot(bool WithCritical, bool WithCycles, std::ostream &dotout);
    void get_dot(std::string &dot);
};

// schedule support for program.h::schedule()
static void schedule_kernel(
    quantum_kernel &kernel,
    const quantum_platform &platform,
    std::string &dot,
    std::string &sched_dot
) {
    std::string scheduler = options::get("scheduler");
    std::string scheduler_uniform = options::get("scheduler_uniform");

    IOUT(scheduler << " scheduling the quantum kernel '" << kernel.name << "'...");

    Scheduler sched;
    sched.init(kernel.c, platform, kernel.qubit_count, kernel.creg_count);

    if (options::get("print_dot_graphs") == "yes") {
        sched.get_dot(dot);
    }

    if (scheduler_uniform == "yes") {
        sched.schedule_alap_uniform(); // result in current kernel's circuit (k.c)
    } else if (scheduler == "ASAP") {
        sched.schedule_asap(sched_dot); // result in current kernel's circuit (k.c)
    } else if (scheduler == "ALAP") {
        sched.schedule_alap(sched_dot); // result in current kernel's circuit (k.c)
    } else {
        FATAL("Not supported scheduler option: scheduler=" << scheduler);
    }
    DOUT(scheduler << " scheduling the quantum kernel '" << kernel.name << "' DONE");
    kernel.cycles_valid = true;
}

/*
 * main entry to the non resource-constrained scheduler
 */
static void schedule(
    quantum_program *programp,
    const quantum_platform &platform,
    const std::string &passname
) {
    if (options::get("prescheduler") == "yes") {
        report_statistics(programp, platform, "in", passname, "# ");
        report_qasm(programp, platform, "in", passname);
    
        IOUT("scheduling the quantum program");
        for (auto &k : programp->kernels) {
            std::string dot;
            std::string kernel_sched_dot;
            schedule_kernel(k, platform, dot, kernel_sched_dot);
    
            if (options::get("print_dot_graphs") == "yes") {
                std::string fname;
                fname = options::get("output_dir") + "/" + k.get_name() + "_dependence_graph.dot";
                IOUT("writing scheduled dot to '" << fname << "' ...");
                utils::write_file(fname, dot);
    
                std::string scheduler_opt = options::get("scheduler");
                fname = options::get("output_dir") + "/" + k.get_name() + scheduler_opt + "_scheduled.dot";
                IOUT("writing scheduled dot to '" << fname << "' ...");
                utils::write_file(fname, kernel_sched_dot);
            }
        }
    
        report_statistics(programp, platform, "out", passname, "# ");
        report_qasm(programp, platform, "out", passname);
    }
}

static void rcschedule_kernel(
    quantum_kernel &kernel,
    const quantum_platform &platform,
    std::string &dot,
    size_t nqubits,
    size_t ncreg = 0
) {
    IOUT("Resource constraint scheduling ...");

    std::string schedopt = options::get("scheduler");
    if (schedopt == "ASAP") {
        Scheduler sched;
        sched.init(kernel.c, platform, nqubits, ncreg);

        arch::resource_manager_t rm(platform, forward_scheduling);
        sched.schedule_asap(rm, platform, dot);
    } else if (schedopt == "ALAP") {
        Scheduler sched;
        sched.init(kernel.c, platform, nqubits, ncreg);

        arch::resource_manager_t rm(platform, backward_scheduling);
        sched.schedule_alap(rm, platform, dot);
    } else {
        FATAL("Not supported scheduler option: scheduler=" << schedopt);
    }

    IOUT("Resource constraint scheduling [Done].");
}

/*
 * main entry point of the rcscheduler
 */
static void rcschedule(
    quantum_program *programp,
    const quantum_platform &platform,
    const std::string &passname
) {
    report_statistics(programp, platform, "in", passname, "# ");
    report_qasm(programp, platform, "in", passname);

    for (auto &kernel : programp->kernels) {
        IOUT("Scheduling kernel: " << kernel.name);
        if (!kernel.c.empty()) {
            auto num_creg = kernel.creg_count;
            std::string sched_dot;

            rcschedule_kernel(kernel, platform, sched_dot, platform.qubit_number, num_creg);
            kernel.cycles_valid = true; // FIXME HvS move this back into call to right after sort_cycle

            if (options::get("print_dot_graphs") == "yes") {
                std::stringstream fname;
                fname << options::get("output_dir") << "/" << kernel.name << "_" << passname << ".dot";
                IOUT("writing " << passname << " dependence graph dot file to '" << fname.str() << "' ...");
                utils::write_file(fname.str(), sched_dot);
            }
        }
    }

    report_statistics(programp, platform, "out", passname, "# ");
    report_qasm(programp, platform, "out", passname);
}

} // namespace ql
