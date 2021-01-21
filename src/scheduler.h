/** \file
 * ASAP/ALAP critical path and UNIFORM scheduling with and without resource
 * constraint.
 *
 * \see scheduler.cc
 */

#pragma once

#include "utils/num.h"
#include "utils/str.h"
#include "utils/list.h"
#include "utils/map.h"

#include <lemon/list_graph.h>
#include <lemon/lgf_reader.h>
#include <lemon/lgf_writer.h>
#include <lemon/dijkstra.h>
#include <lemon/connectivity.h>

#include "options.h"
#include "gate.h"
#include "kernel.h"
#include "circuit.h"
#include "ir.h"
#include "resource_manager.h"
#include "report.h"

namespace ql {

// see above/below for the meaning of R, W, and D events and their relation to dependences
enum DepTypes {RAW, WAW, WAR, RAR, RAD, DAR, DAD, WAD, DAW};
const utils::Str DepTypesNames[] = {"RAW", "WAW", "WAR", "RAR", "RAD", "DAR", "DAD", "WAD", "DAW"};

enum EventType {Writer, Reader, D};
const utils::Str EventTypeNames[] = {"Writer", "Reader", "D"};

typedef utils::Vec<utils::Int> ReadersListType;

class Scheduler {
public:
    // dependence graph is constructed (see Init) once from the sequence of gates in a kernel's circuit
    // it can be reused as often as needed as long as no gates are added/deleted; it doesn't modify those gates
    lemon::ListDigraph graph;

    // conversion between gate* (pointer to the gate in the circuit) and node (of the dependence graph)
    lemon::ListDigraph::NodeMap<gate*> instruction;// instruction[n] == gate*
    utils::Map<gate*, lemon::ListDigraph::Node>  node;// node[gate*] == n

    // attributes
    lemon::ListDigraph::NodeMap<utils::Str> name;     // name[n] == qasm string
    lemon::ListDigraph::ArcMap<utils::Int> weight;    // number of cycles of dependence
    lemon::ListDigraph::ArcMap<utils::Int> cause;     // qubit/creg/breg index of dependence
    lemon::ListDigraph::ArcMap<utils::Int> depType;   // RAW, WAW, ...

    // s and t nodes are the top and bottom of the dependence graph
    lemon::ListDigraph::Node s, t;                     // instruction[s]==SOURCE, instruction[t]==SINK

    // parameters of dependence graph construction
    utils::UInt cycle_time;     // to convert durations to cycles as weight of dependence
    utils::UInt qubit_count;    // number of qubits, to check/represent qubit as cause of dependence
    utils::UInt creg_count;     // number of cregs, to check/represent creg as cause of dependence
    utils::UInt breg_count;     // number of bregs, to check/represent breg as cause of dependence
    circuit *circp;             // current and result circuit, passed from Init to each scheduler

    // scheduler support
    utils::Map<lemon::ListDigraph::Node, utils::UInt>  remaining;  // remaining[node] == cycles until end; critical path representation
private:
    // state of the state machine that is used to construct the dependence graph
    // all vectors are indexed by a combooperand, an encoding of all possible qubits, classical registers and bit registers,
    // that is in qubit_creg_breg combined index space with size qcount+ccount+bcount
    utils::Vec<enum EventType> LastEvent;
    utils::Vec<utils::Int> LastWriter;
    utils::Vec<ReadersListType> LastReaders;
    utils::Vec<ReadersListType> LastDs;

public:
    Scheduler();

    // name may contain parameters, so must be stripped first before checking it for gate's name
    static void stripname(utils::Str &name);

    // signal the state machine of dependence graph construction to do a step as specified by the parameters
    void new_event(
        int currID,
        utils::UInt combooperand,
        enum EventType currEvent,
        bool commutes
    );

    // add a dependence between two nodes
    // operand is in qubit_creg_breg combined index space with size qcount+ccount+bcount
    void add_dep(utils::Int fromID, utils::Int toID, enum DepTypes deptype, utils::UInt comboperand);

    // fill the dependence graph ('graph') with nodes from the circuit and adding arcs for their dependences
    void init(
        circuit &ckt,
        const quantum_platform &platform,
        utils::UInt qcount,
        utils::UInt ccount,
        utils::UInt bcount
    );

    void DPRINTDepgraph(const utils::Str &s) const;
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
    void schedule_asap(utils::Str &sched_dot);

    // ALAP scheduler without RC, setting gate cycle values and sorting the resulting circuit
    void schedule_alap(utils::Str &sched_dot);

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
    gate* find_mostcritical(utils::List<gate*>& lg);

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
        utils::List<lemon::ListDigraph::Node> &avlist,
        scheduling_direction_t dir,
        utils::UInt &curr_cycle
    );

    // collect the list of directly depending nodes
    // (i.e. those necessarily scheduled after the given node) without duplicates;
    // dependences that are duplicates from the perspective of the scheduler
    // may be present in the dependence graph because the scheduler ignores dependence type and cause
    void get_depending_nodes(
        lemon::ListDigraph::Node n,
        scheduling_direction_t dir,
        utils::List<lemon::ListDigraph::Node> &ln
    );

    // Compute of two nodes whether the first one is less deep-critical than the second, for the given scheduling direction;
    // criticality of a node is given by its remaining[node] value which is precomputed;
    // deep-criticality takes into account the criticality of depending nodes (in the right direction!);
    // this function is used to order the avlist in an order from highest deep-criticality to lowest deep-criticality;
    // it is the core of the heuristics of the critical path list scheduler.
    utils::Bool criticality_lessthan(
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
        utils::List<lemon::ListDigraph::Node> &avlist,
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
        utils::List<lemon::ListDigraph::Node> &avlist,
        utils::Map<gate*,utils::Bool> &scheduled,
        scheduling_direction_t dir
    );

    // advance curr_cycle
    // when no node was selected from the avlist, advance to the next cycle
    // and try again; this makes nodes/instructions to complete execution for one more cycle,
    // and makes resources finally available in case of resource constrained scheduling
    // so it contributes to proceeding and to finally have an empty avlist
    static void AdvanceCurrCycle(scheduling_direction_t dir, utils::UInt &curr_cycle);

    // a gate must wait until all its operand are available, i.e. the gates having computed them have completed,
    // and must wait until all resources required for the gate's execution are available;
    // return true when immediately schedulable
    // when returning false, isres indicates whether resource occupation was the reason or operand completion (for debugging)
    utils::Bool immediately_schedulable(
        lemon::ListDigraph::Node n,
        scheduling_direction_t dir,
        const utils::UInt curr_cycle,
        const quantum_platform& platform,
        arch::resource_manager_t &rm,
        utils::Bool &isres
    );

    // select a node from the avlist
    // the avlist is deep-ordered from high to low criticality (see criticality_lessthan above)
    lemon::ListDigraph::Node SelectAvailable(
        utils::List<lemon::ListDigraph::Node> &avlist,
        scheduling_direction_t dir,
        const utils::UInt curr_cycle,
        const quantum_platform &platform,
        arch::resource_manager_t &rm,
        utils::Bool &success
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
        utils::Str &sched_dot
    );

    void schedule_asap(
        arch::resource_manager_t &rm,
        const quantum_platform &platform,
        utils::Str &sched_dot
    );

    void schedule_alap(
        arch::resource_manager_t &rm,
        const quantum_platform &platform,
        utils::Str &sched_dot
    );

    void schedule_alap_uniform();

    // printing dot of the dependence graph
    void get_dot(utils::Bool WithCritical, utils::Bool WithCycles, std::ostream &dotout);
    void get_dot(utils::Str &dot);
};

/*
 * main entry point of the non resource-constrained scheduler
 */
void schedule(
    quantum_program *programp,
    const quantum_platform &platform,
    const utils::Str &passname
);

// kernel-level entry to the non resource-constrained scheduler
void schedule_kernel(
    quantum_kernel &kernel,
    const quantum_platform &platform,
    utils::Str &dot,
    utils::Str &sched_dot
);

/*
 * main entry point of the resource-constrained scheduler
 */
void rcschedule(
    quantum_program *programp,
    const quantum_platform &platform,
    const utils::Str &passname
);

// kernel-level entry to the resource-constrained scheduler
void rcschedule_kernel(
    quantum_kernel &kernel,
    const quantum_platform &platform,
    utils::Str &dot,
    utils::UInt nqubits,
    utils::UInt ncreg = 0,
    utils::UInt nbreg = 0
);

} // namespace ql
