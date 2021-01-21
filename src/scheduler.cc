/** \file
 * ASAP/ALAP critical path and UNIFORM scheduling with and without resource
 * constraint.
 * 
 * Below there really are two classes: the dependency graph definition and the
 * scheduler definition. All schedulers require dependency graph creation as
 * preprocessor, and don't modify it. For each kernel's circuit a private
 * dependency graph is created. The schedulers modify the order of gates in the
 * circuit, initialize the cycle field of each gate, and generate/return the
 * bundles, a list of bundles in which gates starting in the same cycle are
 * grouped.
 *
 * The dependency graph (represented by the graph field below) is created in the
 * Init method, and the graph is constructed from and referring to the gates in
 * the sequence of gates in the kernel's circuit. In this graph, the nodes refer
 * to the gates in the circuit, and the edges represent the dependencies between
 * two gates. Init scans the gates of the circuit from start to end, inspects
 * their parameters, and for each gate depending on the gate type and parameter
 * value and previous gates operating on the same parameters, it creates a
 * dependency of the current gate on that previous gate. Such a dependency has a
 * type (RAW, WAW, etc.), cause (the qubit or classical register used as
 * parameter), and a weight (the cycles the previous gate takes to complete its
 * execution, after which the current gate can start execution).
 *
 * In dependency graph creation, each qubit/classical register (creg,breg) use
 * in each gate is seen as an "event". The following events are distinguished:
 *
 *  - W for Write: such a use must sequentialize with any previous and later
 *    uses of the same qubit/creg/breg. This is the default for qubits in a gate
 *    and for assignment/modifications in classical code.
 *  - Z/R for Z-rotate or Read: such uses can be arbitrarily reordered (as long as other
 *    dependencies allow that). This event applies to all operands of CZ, the
 *    first operand of CNOT gates, all Z rotations (RZ,Z,Z90(SDAG),ZM90(S)),
 *    and to all reads in classical code.
 *    It also applies in general to the control operand of Control Unitaries.
 *    It represents commutativity between the gates which such use: CU(a,b),
 *    CZ(a,c), CZ(d,a), CNOT(a,e), RZ(a), S(a), all commute.
 *  - X/D for X-rotate: such uses can be arbitrarily reordered but are sequentialized with W
 *    and Z events on the same qubit. This event applies to the second operand
 *    of CNOT gates, and all X rotations: CNOT(a,d), CNOT(b,d), RX(d), all commute.
 *  With this, we effectively get the following table of event transitions (from left-bottom to right-up),
 *  in which 'no' indicates no dependency from left event to top event
 *  and '/' indicates a dependency from left to top.
 *
 *             W   Z/R X/D                W   Z/R X/D
 *        W    /   /   /              W   WAW RAW DAW
 *        Z/R  /   no  /              Z/R WAR RAR DAR
 *        X/D  /   /   no             X/D WAD RAD DAD
 *
 * When the 'no' dependencies are created (RAR and/or DAD), the respective
 * commutatable gates are sequentialized according to the original circuit's
 * order. With all 'no's replaced by '/', all event types become equivalent
 * (i.e. as if they were Write).
 *
 * Schedulers come essentially in the following forms:
 *  - ASAP: a plain forward scheduler using dependencies only, aiming at
 *    execution each gate as soon as possible
 *  - ASAP with resource constraints: similar but taking resource constraints of
 *    the gates of the platform into account
 *  - ALAP: as ASAP but then aiming at execution of each gate as late as
 *    possible
 *  - ALAP with resource constraints: similar but taking resource constraints of
 *    the gates of the platform into account
 *  - ALAP with UNIFORM bundle lengths: using dependencies only, aim at ALAP but
 *    with equally length bundles
 *
 * ASAP/ALAP can be controlled by the "scheduler" option. Similarly for UNIFORM
 * ("scheduler_uniform"). With/out resource constraints are separate method
 * calls.
 *
 * Commutation support during scheduling in general produces more
 * efficient/shorter scheduled circuits. It is enabled by option
 * "scheduler_commute".
 */

#include "scheduler.h"

#include "utils/vec.h"
#include "utils/filesystem.h"

namespace ql {

using namespace utils;
using ListDigraph = lemon::ListDigraph;
using ListDigraphPath = lemon::Path<ListDigraph>;

Scheduler::Scheduler() :
    instruction(graph),
    name(graph),
    weight(graph),
    cause(graph),
    depType(graph)
{
}

// ins->name may contain parameters, so must be stripped first before checking it for gate's name
void Scheduler::stripname(Str &name) {
    UInt p = name.find(' ');
    if (p != Str::npos) {
        name = name.substr(0,p);
    }
}

// Add a dependency between two nodes: from node fromID to node toID
// deptype is one of {RAW, WAW, WAR, RAR, RAD, DAR, DAD, WAD, DAW};
// combo is the operand encoded in a qubit+creg+breg combined index space:
// - 0 <= combo < qubit_count:                        combo is a qubit index
// - 0 <= combo-qubit_count < creg_count:             combo-qubit_count is a classical register index
// - 0 <= combo-qubit_count-creg_count < breg_count:  combo-qubit_count-creg_count is a bit register index
void Scheduler::add_dep(Int fromID, Int toID, enum DepTypes deptype, UInt comboperand) {
    QL_DOUT(".. adddep ... from fromID " << fromID << " to toID " << toID << "   opnd=" << comboperand << ", dep=" << DepTypesNames[deptype]);
    auto fromNode = graph.nodeFromId(fromID);
    auto toNode = graph.nodeFromId(toID);
    auto arc = graph.addArc(fromNode, toNode);
    weight[arc] = Int(ceil(static_cast<Real>(instruction[fromNode]->duration) / cycle_time));
    cause[arc] = comboperand;
    UInt operand;
    depType[arc] = deptype;
    Str s;
    if (comboperand < qubit_count) {
        s = "q";
        operand = comboperand;
    } else if (comboperand - qubit_count < creg_count) {
        operand = comboperand - qubit_count;
        s = "c";
    } else {
        operand = comboperand - (qubit_count + creg_count);
        s = "b";
    }
    QL_DOUT("... dep " << name[fromNode] << " -> " << name[toNode] << " (opnd=" << s << "[" << operand << "], dep=" << DepTypesNames[deptype] << ", wght=" << weight[arc] << ")");
}

// Signal a new event to the depgraph constructor:
// the new event is of type currEvent (which is Writer, Reader or D)
// and concerns the current gate encoded by currID
// and its given combooperand (see add_dep for this encoding);
// commutes indicates whether this event is allowed to commute with the other events of its type.
// This event drives a state machine to do one step (one state transition).
// It accepts the following event sequence per combooperand: Writer { Writer | Reader+ | D+ }* Writer,
// in which the first Writer is the SOURCE and the last Writer is the SINK.
// This state machine has as state { LastEvent[], LastWriter[], LastReaders[] and LastDs[] },
// which are vectors over all possible combooperands (all qubits, all classical registers and all bit registers).
void Scheduler::new_event(
    int currID,
    UInt combooperand,
    EventType currEvent,
    bool commutes
) {
    QL_DOUT(".. " << EventTypeNames[currEvent] << " on operand: " << combooperand << " while in " << EventTypeNames[LastEvent[combooperand]]);
    switch (currEvent) {
    case Writer:
        if (LastEvent[combooperand] == Writer) {
            add_dep(LastWriter[combooperand], currID, WAW, combooperand);
        }
        if (LastEvent[combooperand] == Reader) {
            for (auto &RgateID : LastReaders[combooperand]) {
                add_dep(RgateID, currID, WAR, combooperand);
            }
        }
        if (LastEvent[combooperand] == D) {
            for (auto &DgateID : LastDs[combooperand]) {
                add_dep(DgateID, currID, WAD, combooperand);
            }
        }
        LastWriter[combooperand] = currID;
        break;

    case Reader:
        add_dep(LastWriter[combooperand], currID, RAW, combooperand);
        if (LastEvent[combooperand] != Reader) {
            LastReaders[combooperand].clear();
        }
        if (LastEvent[combooperand] == Reader) {
            if (!commutes) {
                for (auto &RgateID : LastReaders[combooperand]) {
                    add_dep(RgateID, currID, RAR, combooperand);
                }
            }
        }
        for (auto &DgateID : LastDs[combooperand]) {
            add_dep(DgateID, currID, RAD, combooperand);
        }
        LastReaders[combooperand].push_back(currID);
        break;

    case D:
        add_dep(LastWriter[combooperand], currID, DAW, combooperand);
        if (LastEvent[combooperand] != D) {
            LastDs[combooperand].clear();
        }
        for (auto &RgateID : LastReaders[combooperand]) {
            add_dep(RgateID, currID, DAR, combooperand);
        }
        if (LastEvent[combooperand] == D) {
            if (!commutes) {
                for (auto &DgateID : LastDs[combooperand]) {
                    add_dep(DgateID, currID, DAD, combooperand);
                }
            }
        }
        LastDs[combooperand].push_back(currID);
        break;
    }
    LastEvent[combooperand] = currEvent;
}

// construct the dependency graph ('graph') with nodes from the circuit and adding arcs for their dependencies
void Scheduler::init(
    circuit &ckt,
    const quantum_platform &platform,
    UInt qcount,        // number of qubits
    UInt ccount,        // number of classical registers
    UInt bcount         // number of bit registers
) {
    QL_DOUT("dependency graph creation ... #qubits = " << platform.qubit_number);
    qubit_count = qcount; ///@todo-rn: DDG creation should not depend on #qubits
    creg_count = ccount; ///@todo-rn: DDG creation should not depend on #cregs
    breg_count = bcount; ///@todo-rn: DDG creation should not depend on #bregs
    
    // combo encoding initialization:
    UInt creg_base = qubit_count;
    UInt breg_base = qubit_count + creg_count;
    UInt total_reg_count = qubit_count + creg_count + breg_count;
    QL_DOUT("Scheduler.init: qubit_count=" << qubit_count << ", creg_count=" << creg_count << ", breg_count=" << breg_count << ", total=" << total_reg_count);

    cycle_time = platform.cycle_time;
    circp = &ckt;

    // dependencies are created with a current gate as target
    // and with those previous gates as source that have an operand match with the current gate:
    // this dependence creation is done by a state matchine triggered to step on each operand of each gate encountered.
    // operands can be a qubit, a classical register or a bit register
    // the indices in LastReaders, LastDs and LastWriter are operand indices in the combined index space (see add_dep)

    // start filling the dependency graph by creating the s node, the top of the graph
    {
        // add dummy source node
        auto srcNode = graph.addNode();
        instruction[srcNode] = new SOURCE();    // so SOURCE is defined as instruction[s], not unique in itself
        node.set(instruction[srcNode]) = srcNode;
        name[srcNode] = instruction[srcNode]->qasm();
        s = srcNode;
    }
    Int srcID = graph.id(s);

    // start the state machine as if the SOURCE gate with a write on all possible operands has been seen
    LastEvent.resize(total_reg_count, Writer);      // SOURCE virtually writes to all qubits/cregs/bregs
    LastWriter.resize(total_reg_count, srcID);
    LastReaders.resize(total_reg_count);            // LastReaders and LastDs start off as empty lists
    LastDs.resize(total_reg_count);

    // for each gate pointer ins in the circuit, add a node and add dependencies on previous gates to it
    for (auto ins : ckt) {
        QL_DOUT("Current instruction's name: `" << ins->name << "'");
        QL_DOUT(".. Qasm(): " << ins->qasm());
        for (auto operand : ins->operands) {
            QL_DOUT(".. Operand: `q[" << operand << "]'");
        }
        for (auto coperand : ins->creg_operands) {
            QL_DOUT(".. Classical operand: `c[" << coperand << "]'");
        }
        for (auto boperand : ins->breg_operands) {
            QL_DOUT(".. Bit operand: `b[" << boperand << "]'");
        }
        if (ins->is_conditional()) {
            QL_DOUT(".. Condition: `" << ins->cond_qasm() << "'");
        }

        auto iname = ins->name; // copy!!!!
        stripname(iname);

        // Add node
        ListDigraph::Node currNode = graph.addNode();
        int currID = graph.id(currNode);
        instruction[currNode] = ins;
        node.set(ins) = currNode;
        name[currNode] = ins->qasm();   // and this includes any condition!

        // Add edges (arcs)
        // In quantum computing there are no real Reads and Writes on qubits because they cannot be cloned.
        // Every qubit use influences the qubit, updates it, so would be considered a Read+Write at the same time.
        // In dependency graph construction, this leads to WAW-dependency chains of all uses of the same qubit,
        // and hence in a scheduler using this graph to a sequentialization of those uses in the original program order.
        //
        // For a scheduler, only the presence of a dependency counts, not its type (RAW/WAW/etc.).
        // A dependency graph also has other uses apart from the scheduler: e.g. to find chains of live qubits,
        // from their creation (Prep etc.) to their destruction (Measure, etc.) in allocation of virtual to real qubits.
        // For those uses it makes sense to make a difference with a gate doing a Read+Write, just a Write or just a Read:
        // a Prep creates a new 'value' (Write); wait, display, x, swap, cnot, all pass this value on (so Read+Write),
        // while a Measure 'destroys' the 'value' (Read+Write of the qubit, Write of the creg),
        // the destruction aspect of a Measure being implied by it being followed by a Prep (Write only) on the same qubit.
        // Furthermore Writes can model barriers on a qubit (see Wait, Display, etc.), because Writes sequentialize.
        // The dependency graph creation below models a graph suitable for all functions, including chains of live qubits.

        // Control-operands of Controlled Unitaries commute, independent of the Unitary,
        // i.e. these gates need not be kept in order.
        // But, of course, those qubit uses should be ordered after (/before) the last (/next) non-control use of the qubit.
        // In this way, those control-operand qubit uses would be like pure Reads in dependency graph construction.
        // A problem might be that the gates with the same control-operands might be scheduled in parallel then.
        // In a non-resource scheduler that will happen but it doesn't do harm because it is not a real machine.
        // In a resource-constrained scheduler the resource constraint that prohibits more than one use
        // of the same qubit being active at the same time, will prevent this parallelism.
        // So ignoring Read After Read (RAR) dependencies enables the scheduler to take advantage
        // of the commutation property of Controlled Unitaries without disadvantages.
        //
        // In more detail:
        // 1. CU1(a,b) and CU2(a,c) commute (for any U1, U2, so also can be equal and/or be CNOT and/or be CZ)
        // 2. CNOT(a,b) and CNOT(c,b) commute (property of CNOT only).
        // 3. CZ(a,b) and CZ(b,a) are identical (property of CZ only).
        // 4. CNOT(a,b) commutes with CZ(a,c) (from 1.) and thus with CZ(c,a) (from 3.)
        // 5. CNOT(a,b) does not commute with CZ(c,b) (and thus not with CZ(b,c), from 3.)
        // To support this, next to R and W a D (for controlleD operand :-) is introduced for the target operand of CNOT.
        // The events (instead of just Read and Write) become then:
        // - Both operands of CZ are just Read.
        // - The control operand of CNOT is Read, the target operand is D.
        // - Of any other Control Unitary, the control operand is Read and the target operand is Write (not D!)
        // - Of any other gate the operands are Read+Write or just Write (as usual to represent flow).
        // With this, we effectively get the following table of event transitions (from left-bottom to right-up),
        // in which 'no' indicates no dependency from left event to top event and '/' indicates a dependency from left to top.
        //
        //             W   R   D                  w   R   D
        //        W    /   /   /              W   WAW RAW DAW
        //        R    /   no  /              R   WAR RAR DAR
        //        D    /   /   no             D   WAD RAD DAD
        //
        // In addition to LastReaders, we introduce LastDs.
        // Either one is cleared when dependencies are generated from them, and extended otherwise.
        // From the table it can be seen that the D 'behaves' as a Write to Read, and as a Read to Write,
        // that there is no order among Ds nor among Rs, but D after R and R after D sequentialize.
        // With this, the dependency graph is claimed to represent the commutations as above.
        //
        // The schedulers are list schedulers, i.e. they maintain a list of gates in their algorithm,
        // of gates available for being scheduled because they are not blocked by dependencies on non-scheduled gates.
        // Therefore, the schedulers are able to select the best one from a set of commutable gates.

        // FIXME: define signature in .json file similar to how llvm/scaffold/gcc defines instructions
        // and then have a signature interpreter here; then we don't have this long if-chain
        // and, more importantly, we don't have the knowledge of particular gates here;
        // the default signature would be that of a default gate, modifying each qubit operand.

        // every gate can have a condition with condition operands (which are bit register indices) that are read
        for (auto boperand : ins->cond_operands) {
            QL_DOUT(".. Condition operand: " << boperand);
            new_event(currID, breg_base+boperand, Reader, true);
        }

        // each type of gate has a different 'signature' of events; switch out to each one
        if (iname == "measure") {
            QL_DOUT(". considering " << name[currNode] << " as measure");
            // Write each operand + Write each classical operand + Write each bit operand
            for (auto operand : ins->operands) {
                new_event(currID, operand, Writer, false);
            }
            for (auto coperand : ins->creg_operands) {
                new_event(currID, creg_base+coperand, Writer, false);
            }
            for (auto boperand : ins->breg_operands) {
                new_event(currID, breg_base+boperand, Writer, false);
            }
            QL_DOUT(". measure done");
        } else if (iname == "display") {
            QL_DOUT(". considering " << name[currNode] << " as display");
            // no operands, display all qubits and cregs
            // Write each one
            Vec<UInt> qubits(total_reg_count);
            std::iota(qubits.begin(), qubits.end(), 0);

            for (auto operand : qubits) {
                new_event(currID, operand, Writer, false);
            }
        } else if (ins->type() == gate_type_t::__classical_gate__) {
            QL_DOUT(". considering " << name[currNode] << " as classical gate");
            // Write each classical operand
            for (auto coperand : ins->creg_operands) {
                new_event(currID, creg_base+coperand, Writer, false);
            }
        } else if (iname == "cnot") {
            QL_DOUT(". considering " << name[currNode] << " as cnot");
            // CNOTs first operand is control and a Z, second operand is target and an X
            QL_ASSERT(ins->operands.size() == 2);

            new_event(currID, ins->operands[0], Reader, options::get("scheduler_commute") == "yes");
            new_event(currID, ins->operands[1], D, options::get("scheduler_commute") == "yes");
        } else if (iname == "cz" || iname == "cphase") {
            QL_DOUT(". considering " << name[currNode] << " as cz");
            // CZs operands are both Zs
            QL_ASSERT(ins->operands.size() == 2);
            new_event(currID, ins->operands[0], Reader, options::get("scheduler_commute") == "yes");
            new_event(currID, ins->operands[1], Reader, options::get("scheduler_commute") == "yes");
        } else if (
                iname == "rz"
                || iname == "z"
                || iname == "pauli_z"
                || iname == "rz180"
                || iname == "z90"
                || iname == "rz90"
                || iname == "zm90"
                || iname == "mrz90"
                || iname == "s"
                || iname == "sdag"
                || iname == "t"
                || iname == "tdag"
            ) {
            QL_DOUT(". considering " << name[currNode] << " as Z rotation");
            // Z rotations on single operand
            QL_ASSERT(ins->operands.size() == 1);
            new_event(currID, ins->operands[0], Reader, options::get("scheduler_commute_rotations") == "yes");
        } else if (
                iname == "rx"
                || iname == "x"
                || iname == "pauli_x"
                || iname == "rx180"
                || iname == "x90"
                || iname == "rx90"
                || iname == "xm90"
                || iname == "mrx90"
                || iname == "x45"
            ) {
            QL_DOUT(". considering " << name[currNode] << " as X rotation");
            // X rotations on single operand
            QL_ASSERT(ins->operands.size() == 1);
            new_event(currID, ins->operands[0], D, options::get("scheduler_commute_rotations") == "yes");
        } else {
            QL_DOUT(". considering " << name[currNode] << " as no special gate (catch-all, generic rules)");
            // Write on each quantum operand
            // Write on each classical operand
            // Write on each bit operand
            for (auto operand : ins->operands) {
                new_event(currID, operand, Writer, false);
            }
            for (auto coperand : ins->creg_operands) {
                new_event(currID, creg_base+coperand, Writer, false);
            }
            for (auto boperand : ins->breg_operands) {
                new_event(currID, breg_base+boperand, Writer, false);
            }
        } // end of if/else
        QL_DOUT(". instruction done: " << ins->qasm());
    } // end of instruction for

    QL_DOUT("adding deps to SINK");
    // finish filling the dependency graph by creating the t node, the bottom of the graph
    {
        // add dummy target node
        ListDigraph::Node currNode = graph.addNode();
        int currID = graph.id(currNode);
        instruction[currNode] = new SINK();    // so SINK is defined as instruction[t], not unique in itself
        node.set(instruction[currNode]) = currNode;
        name[currNode] = instruction[currNode]->qasm();
        t = currNode;

        // add deps to the dummy target node to close the dependency chains
        // it behaves as a W to every qubit, creg and breg
        //
        // to guarantee that exactly at start of execution of dummy SINK,
        // all still executing nodes complete, give arc weight of those nodes;
        // this is relevant for ALAP (which starts backward from SINK for all these nodes);
        // also for accurately computing the circuit's depth (which includes full completion);
        // and also for implementing scheduling and mapping across control-flow (so that it is
        // guaranteed that on a jump and on start of target circuit, the source circuit completed).
        //
        // note that there always is a LastWriter: the dummy source node wrote to every qubit and class. reg
        Vec<UInt> all_operands(total_reg_count);
        std::iota(all_operands.begin(), all_operands.end(), 0);
        for (auto operand : all_operands) {
            QL_DOUT(".. Sink operand, adding dep: " << operand);
            add_dep(LastWriter[operand], currID, WAW, operand);
            for (auto &RgateID : LastReaders[operand]) {
                add_dep(RgateID, currID, WAR, operand);
            }
            for (auto &DgateID : LastDs[operand]) {
                add_dep(DgateID, currID, WAD, operand);
            }
        }

        // useless because there is nothing after t but destruction
        for (auto operand : all_operands) {
            QL_DOUT(".. Sink operand, clearing: " << operand);
            LastWriter[operand] = currID;
            LastReaders[operand].clear();
            LastDs[operand].clear();
        }
    }

    // when in doubt about dependence graph, enable next line to get a dump of it in debugging output
    DPRINTDepgraph("init");

    // useless as well because by construction, there cannot be cycles
    // but when afterwards dependencies are added, cycles may be created,
    // and after doing so (a copy of) this test should certainly be done because
    // a cyclic dependency graph cannot be scheduled;
    // this test here is a kind of debugging aid whether dependency creation was done well
    if (!dag(graph)) {
        QL_FATAL("The dependency graph is not a DAG.");
    }
    QL_DOUT("dependency graph creation Done.");
}

// print depgraph for debugging with string parameter identifying where
void Scheduler::DPRINTDepgraph(const Str &s) const {
    if (logger::log_level >= logger::LogLevel::LOG_DEBUG) {
        std::cout << "Depgraph " << s << std::endl;
        for (ListDigraph::NodeIt n(graph); n != lemon::INVALID; ++n) {
            std::cout << "Node " << graph.id(n) << " \"" << name[n] << "\" :" << std::endl;
            std::cout << "    out:";
            for (ListDigraph::OutArcIt arc(graph,n); arc != lemon::INVALID; ++arc) {
                std::cout << " Arc(" << graph.id(arc) << "," << DepTypesNames[ depType[arc] ] << "," << cause[arc] << ")->node(" << graph.id(graph.target(arc)) << ")";
            }
            std::cout << std::endl;
            std::cout << "    in:";
            for (ListDigraph::InArcIt arc(graph,n); arc != lemon::INVALID; ++arc) {
                std::cout << " Arc(" << graph.id(arc) << "," << DepTypesNames[ depType[arc] ] << "," << cause[arc] << ")<-node(" << graph.id(graph.source(arc)) << ")";
            }
            std::cout << std::endl;
        }
        std::cout << "End Depgraph" << std::endl;
    }
}

void Scheduler::print() const {
    QL_COUT("Printing dependency Graph ");
    digraphWriter(graph).
        nodeMap("name", name).
        arcMap("cause", cause).
        arcMap("weight", weight).
        // arcMap("depType", depType).
        node("source", s).
        node("target", t).
        run();
}

void Scheduler::write_dependence_matrix() const {
    QL_COUT("Printing dependency Matrix ...");
    Str datfname( options::get("output_dir") + "/dependenceMatrix.dat");
    OutFile fout(datfname);

    UInt totalInstructions = countNodes(graph);
    Vec<Vec<Bool> > Matrix(totalInstructions, Vec<Bool>(totalInstructions));

    // now print the edges
    for (ListDigraph::ArcIt arc(graph); arc != lemon::INVALID; ++arc) {
        auto srcNode = graph.source(arc);
        auto dstNode = graph.target(arc);
        UInt srcID = graph.id( srcNode );
        UInt dstID = graph.id( dstNode );
        Matrix[srcID][dstID] = true;
    }

    for (UInt i = 1; i < totalInstructions - 1; i++) {
        for (UInt j = 1; j < totalInstructions - 1; j++) {
            fout << Matrix[j][i] << "\t";
        }
        fout << "\n";
    }
}

// cycle assignment without RC depending on direction: forward:ASAP, backward:ALAP;
// set_cycle iterates over the circuit's gates and set_cycle_gate over the dependences of each gate,
// without RC, this is all there is to schedule a circuit;
// on return, cycle will have been set
//
// when it finds a next gate with undefined cycle value,
// set_cycle_gate recurses to force it getting defined, and then proceeds;
// the latter never happens when the depgraph was constructed directly from the circuit
// but when in between the depgraph was updated (as done in commute_variation),
// dependences may have been inserted in the opposite circuit direction and then the recursion kicks in
void Scheduler::set_cycle_gate(gate *gp, scheduling_direction_t dir) {
    ListDigraph::Node currNode = node.at(gp);
    UInt  currCycle;
    if (forward_scheduling == dir) {
        currCycle = 0;
        for (ListDigraph::InArcIt arc(graph,currNode); arc != lemon::INVALID; ++arc) {
            auto nextgp = instruction[graph.source(arc)];
            if (nextgp->cycle == MAX_CYCLE) {
                set_cycle_gate(nextgp, dir);
            }
            currCycle = max<UInt>(currCycle, nextgp->cycle + weight[arc]);
        }
    } else {
        currCycle = ALAP_SINK_CYCLE;
        for (ListDigraph::OutArcIt arc(graph,currNode); arc != lemon::INVALID; ++arc) {
            auto nextgp = instruction[graph.target(arc)];
            if (nextgp->cycle == MAX_CYCLE) {
                set_cycle_gate(nextgp, dir);
            }
            currCycle = min<UInt>(currCycle, nextgp->cycle - weight[arc]);
        }
    }
    gp->cycle = currCycle;
    QL_DOUT("... set_cycle of " << gp->qasm() << " cycles " << gp->cycle);
}

void Scheduler::set_cycle(scheduling_direction_t dir) {
    // note when iterating that graph contains SOURCE and SINK whereas the circuit doesn't
    for (ListDigraph::NodeIt n(graph); n != lemon::INVALID; ++n) {
        instruction[n]->cycle = MAX_CYCLE;       // not yet visited successfully by set_cycle_gate
    }
    if (forward_scheduling == dir) {
        set_cycle_gate(instruction[s], dir);
        for (auto gpit = circp->begin(); gpit != circp->end(); gpit++) {
            if ((*gpit)->cycle == MAX_CYCLE) {
                set_cycle_gate(*gpit, dir);
            }
        }
        set_cycle_gate(instruction[t], dir);
    } else {
        set_cycle_gate(instruction[t], dir);
        for (auto gpit = circp->rbegin(); gpit != circp->rend(); gpit++) {
            if ((*gpit)->cycle == MAX_CYCLE) {
                set_cycle_gate(*gpit, dir);
            }
        }
        set_cycle_gate(instruction[s], dir);

        // readjust cycle values of gates so that SOURCE is at 0
        UInt  SOURCECycle = instruction[s]->cycle;
        QL_DOUT("... readjusting cycle values by -" << SOURCECycle);

        instruction[t]->cycle -= SOURCECycle;
        for (auto &gp : *circp) {
            gp->cycle -= SOURCECycle;
        }
        instruction[s]->cycle -= SOURCECycle;   // i.e. becomes 0
    }
}

static Bool cycle_lessthan(gate *gp1, gate *gp2) {
    return gp1->cycle < gp2->cycle;
}

// sort circuit by the gates' cycle attribute in non-decreasing order
void Scheduler::sort_by_cycle(circuit *cp) {
    QL_DOUT("... before sorting on cycle value");
    // for ( circuit::iterator gpit = cp->begin(); gpit != cp->end(); gpit++)
    // {
    //     gate*           gp = *gpit;
    //     DOUT("...... (@" << gp->cycle << ") " << gp->qasm());
    // }

    // std::sort doesn't preserve the original order of elements that have equal values but std::stable_sort does
    std::stable_sort(cp->begin(), cp->end(), cycle_lessthan);

    QL_DOUT("... after sorting on cycle value");
    // for ( circuit::iterator gpit = cp->begin(); gpit != cp->end(); gpit++)
    // {
    //     gate*           gp = *gpit;
    //     DOUT("...... (@" << gp->cycle << ") " << gp->qasm());
    // }
}

// ASAP scheduler without RC, setting gate cycle values and sorting the resulting circuit
void Scheduler::schedule_asap(Str &sched_dot) {
    QL_DOUT("Scheduling ASAP ...");
    set_cycle(forward_scheduling);
    sort_by_cycle(circp);

    if (options::get("print_dot_graphs") == "yes") {
        StrStrm ssdot;
        get_dot(false, true, ssdot);
        sched_dot = ssdot.str();
    }

    QL_DOUT("Scheduling ASAP [DONE]");
}

// ALAP scheduler without RC, setting gate cycle values and sorting the resulting circuit
void Scheduler::schedule_alap(Str &sched_dot) {
    QL_DOUT("Scheduling ALAP ...");
    set_cycle(backward_scheduling);
    sort_by_cycle(circp);

    if (options::get("print_dot_graphs") == "yes") {
        StrStrm ssdot;
        get_dot(false, true, ssdot);
        sched_dot = ssdot.str();
    }

    QL_DOUT("Scheduling ALAP [DONE]");
}

// remaining[node] == cycles until end of schedule; nodes with highest remaining are most critical
// it is without RC and depends on direction: forward:ASAP so cycles until SINK, backward:ALAP so cycles until SOURCE;
// remaining[node] is complementary to node's cycle value,
// so the implementation below is also a systematically modified copy of that of set_cycle_gate and set_cycle
void Scheduler::set_remaining_gate(gate* gp, scheduling_direction_t dir) {
    ListDigraph::Node currNode = node.at(gp);
    UInt currRemain = 0;
    QL_DOUT("... set_remaining of node " << graph.id(currNode) << ": " << gp->qasm() << " ...");
    if (forward_scheduling == dir) {
        for (ListDigraph::OutArcIt arc(graph,currNode); arc != lemon::INVALID; ++arc) {
            auto nextNode = graph.target(arc);
            QL_DOUT("...... target of arc " << graph.id(arc) << " to node " << graph.id(nextNode));
            if (remaining.at(nextNode) == MAX_CYCLE) {
                set_remaining_gate(instruction[nextNode], dir);
            }
            currRemain = max<UInt>(currRemain, remaining.at(nextNode) + weight[arc]);
        }
    } else {
        for (ListDigraph::InArcIt arc(graph,currNode); arc != lemon::INVALID; ++arc) {
            auto nextNode = graph.source(arc);
            QL_DOUT("...... source of arc " << graph.id(arc) << " from node " << graph.id(nextNode));
            if (remaining.at(nextNode) == MAX_CYCLE) {
                set_remaining_gate(instruction[nextNode], dir);
            }
            currRemain = max<UInt>(currRemain, remaining.at(nextNode) + weight[arc]);
        }
    }
    remaining.set(currNode) = currRemain;
    QL_DOUT("... set_remaining of node " << graph.id(currNode) << ": " << gp->qasm() << " remaining " << currRemain);
}

void Scheduler::set_remaining(scheduling_direction_t dir) {
    // note when iterating that graph contains SOURCE and SINK whereas the circuit doesn't;
    // regretfully, the order of visiting the nodes while iterating over the graph, is undefined
    // and in set_remaining (and set_cycle) the order matters (i.e. in circuit order or reversed circuit order)
    for (ListDigraph::NodeIt n(graph); n != lemon::INVALID; ++n) {
        remaining.set(n) = MAX_CYCLE;               // not yet visited successfully by set_remaining_gate
    }
    if (forward_scheduling == dir) {
        // remaining until SINK (i.e. the SINK.cycle-ALAP value)
        set_remaining_gate(instruction[t], dir);
        for (auto gpit = circp->rbegin(); gpit != circp->rend(); gpit++) {
            if (remaining.at(node.at(*gpit)) == MAX_CYCLE) {
                set_remaining_gate(*gpit, dir);
            }
        }
        set_remaining_gate(instruction[s], dir);
    } else {
        // remaining until SOURCE (i.e. the ASAP value)
        set_remaining_gate(instruction[s], dir);
        for (auto gpit = circp->begin(); gpit != circp->end(); gpit++) {
            if (remaining.at(node.at(*gpit)) == MAX_CYCLE) {
                set_remaining_gate(*gpit, dir);
            }
        }
        set_remaining_gate(instruction[t], dir);
    }
}

gate *Scheduler::find_mostcritical(List<gate*> &lg) {
    UInt maxRemain = 0;
    gate *mostCriticalGate = nullptr;
    for (auto gp : lg) {
        UInt gr = remaining.at(node.at(gp));
        if (gr > maxRemain) {
            mostCriticalGate = gp;
            maxRemain = gr;
        }
    }
    QL_DOUT("... most critical gate: " << mostCriticalGate->qasm() << " with remaining=" << maxRemain);
    return mostCriticalGate;
}

// Set the curr_cycle of the scheduling algorithm to start at the appropriate end as well;
// note that the cycle attributes will be shifted down to start at 1 after backward scheduling.
void Scheduler::init_available(
    List<ListDigraph::Node> &avlist,
    scheduling_direction_t dir,
    UInt &curr_cycle
) {
    avlist.clear();
    if (forward_scheduling == dir) {
        curr_cycle = 0;
        instruction[s]->cycle = curr_cycle;
        avlist.push_back(s);
    } else {
        curr_cycle = ALAP_SINK_CYCLE;
        instruction[t]->cycle = curr_cycle;
        avlist.push_back(t);
    }
}

// collect the list of directly depending nodes
// (i.e. those necessarily scheduled after the given node) without duplicates;
// dependencies that are duplicates from the perspective of the scheduler
// may be present in the dependency graph because the scheduler ignores dependency type and cause
void Scheduler::get_depending_nodes(
    ListDigraph::Node n,
    scheduling_direction_t dir,
    List<ListDigraph::Node> &ln
) {
    if (forward_scheduling == dir) {
        for (ListDigraph::OutArcIt succArc(graph,n); succArc != lemon::INVALID; ++succArc) {
            auto succNode = graph.target(succArc);
            // DOUT("...... succ of " << instruction[n]->qasm() << " : " << instruction[succNode]->qasm());
            Bool found = false;             // filter out duplicates
            for (auto anySuccNode : ln) {
                if (succNode == anySuccNode) {
                    // DOUT("...... duplicate: " << instruction[succNode]->qasm());
                    found = true;           // duplicate found
                }
            }
            if (!found) {                   // found new one
                ln.push_back(succNode);     // new node to ln
            }
        }
        // ln contains depending nodes of n without duplicates
    } else {
        for (ListDigraph::InArcIt predArc(graph,n); predArc != lemon::INVALID; ++predArc) {
            ListDigraph::Node predNode = graph.source(predArc);
            // DOUT("...... pred of " << instruction[n]->qasm() << " : " << instruction[predNode]->qasm());
            Bool found = false;             // filter out duplicates
            for (auto anyPredNode : ln) {
                if (predNode == anyPredNode) {
                    // DOUT("...... duplicate: " << instruction[predNode]->qasm());
                    found = true;           // duplicate found
                }
            }
            if (!found) {                   // found new one
                ln.push_back(predNode);     // new node to ln
            }
        }
        // ln contains depending nodes of n without duplicates
    }
}

// Compute of two nodes whether the first one is less deep-critical than the second, for the given scheduling direction;
// criticality of a node is given by its remaining[node] value which is precomputed;
// deep-criticality takes into account the criticality of depending nodes (in the right direction!);
// this function is used to order the avlist in an order from highest deep-criticality to lowest deep-criticality;
// it is the core of the heuristics of the critical path list scheduler.
Bool Scheduler::criticality_lessthan(
    ListDigraph::Node n1,
    ListDigraph::Node n2,
    scheduling_direction_t dir
) {
    if (n1 == n2) return false;             // because not <

    if (remaining.at(n1) < remaining.at(n2)) return true;
    if (remaining.at(n1) > remaining.at(n2)) return false;
    // so: remaining[n1] == remaining[n2]

    List<ListDigraph::Node> ln1;
    List<ListDigraph::Node> ln2;

    get_depending_nodes(n1, dir, ln1);
    get_depending_nodes(n2, dir, ln2);
    if (ln2.empty()) return false;          // strictly < only when ln1.empty and ln2.not_empty
    if (ln1.empty()) return true;           // so when both empty, it is equal, so not strictly <, so false
    // so: ln1.non_empty && ln2.non_empty

    ln1.sort([this](const ListDigraph::Node &d1, const ListDigraph::Node &d2) { return remaining.at(d1) < remaining.at(d2); });
    ln2.sort([this](const ListDigraph::Node &d1, const ListDigraph::Node &d2) { return remaining.at(d1) < remaining.at(d2); });

    UInt crit_dep_n1 = remaining.at(ln1.back());    // the last of the list is the one with the largest remaining value
    UInt crit_dep_n2 = remaining.at(ln2.back());

    if (crit_dep_n1 < crit_dep_n2) return true;
    if (crit_dep_n1 > crit_dep_n2) return false;
    // so: crit_dep_n1 == crit_dep_n2, call this crit_dep

    ln1.remove_if([this,crit_dep_n1](ListDigraph::Node n) { return remaining.at(n) < crit_dep_n1; });
    ln2.remove_if([this,crit_dep_n2](ListDigraph::Node n) { return remaining.at(n) < crit_dep_n2; });
    // because both contain element with remaining == crit_dep: ln1.non_empty && ln2.non_empty

    if (ln1.size() < ln2.size()) return true;
    if (ln1.size() > ln2.size()) return false;
    // so: ln1.size() == ln2.size() >= 1

    ln1.sort([this,dir](const ListDigraph::Node &d1, const ListDigraph::Node &d2) { return criticality_lessthan(d1, d2, dir); });
    ln2.sort([this,dir](const ListDigraph::Node &d1, const ListDigraph::Node &d2) { return criticality_lessthan(d1, d2, dir); });
    return criticality_lessthan(ln1.back(), ln2.back(), dir);
}

// Make node n available
// add it to the avlist because the condition for that is fulfilled:
//  all its predecessors were scheduled (forward scheduling) or
//  all its successors were scheduled (backward scheduling)
// update its cycle attribute to reflect these dependencies;
// avlist is initialized with s or t as first element by init_available
// avlist is kept ordered on deep-criticality, non-increasing (i.e. highest deep-criticality first)
void Scheduler::MakeAvailable(
    ListDigraph::Node n,
    List<ListDigraph::Node> &avlist,
    scheduling_direction_t dir
) {
    Bool already_in_avlist = false;  // check whether n is already in avlist
    // originates from having multiple arcs between pair of nodes
    List<ListDigraph::Node>::iterator first_lower_criticality_inp; // for keeping avlist ordered
    Bool first_lower_criticality_found = false;                          // for keeping avlist ordered

    QL_DOUT(".... making available node " << name[n] << " remaining: " << remaining.dbg(n));
    for (auto inp = avlist.begin(); inp != avlist.end(); inp++) {
        if (*inp == n) {
            already_in_avlist = true;
            QL_DOUT("...... duplicate when making available: " << name[n]);
        } else {
            // scanning avlist from front to back (avlist is ordered from high to low criticality)
            // when encountering first node *inp with less criticality,
            // that is where new node n should be inserted just before,
            // to keep avlist in desired order
            //
            // consequence is that
            // when a node has same criticality as n, new node n is put after it, as last one of set of same criticality,
            // so order of calling MakeAvailable (and probably original circuit, and running other scheduler first) matters,
            // also when all dependency sets (and so remaining values) are identical!
            if (criticality_lessthan(*inp, n, dir) && !first_lower_criticality_found) {
                first_lower_criticality_inp = inp;
                first_lower_criticality_found = true;
            }
        }
    }
    if (!already_in_avlist) {
        set_cycle_gate(instruction[n], dir);        // for the schedulers to inspect whether gate has completed
        if (first_lower_criticality_found) {
            // add n to avlist just before the first with lower criticality
            avlist.insert(first_lower_criticality_inp, n);
        } else {
            // add n to end of avlist, if none found with less criticality
            avlist.push_back(n);
        }
        QL_DOUT("...... made available node(@" << instruction[n]->cycle << "): " << name[n] << " remaining: " << remaining.dbg(n));
    }
}

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
void Scheduler::TakeAvailable(
    ListDigraph::Node n,
    List<ListDigraph::Node> &avlist,
    Map<gate*,Bool> &scheduled,
    scheduling_direction_t dir
) {
    scheduled.set(instruction[n]) = true;
    avlist.remove(n);

    if (forward_scheduling == dir) {
        for (ListDigraph::OutArcIt succArc(graph,n); succArc != lemon::INVALID; ++succArc) {
            auto succNode = graph.target(succArc);
            Bool schedulable = true;
            for (ListDigraph::InArcIt predArc(graph,succNode); predArc != lemon::INVALID; ++predArc) {
                ListDigraph::Node predNode = graph.source(predArc);
                if (!scheduled.at(instruction[predNode])) {
                    schedulable = false;
                    break;
                }
            }
            if (schedulable) {
                MakeAvailable(succNode, avlist, dir);
            }
        }
    } else {
        for (ListDigraph::InArcIt predArc(graph,n); predArc != lemon::INVALID; ++predArc) {
            auto predNode = graph.source(predArc);
            Bool schedulable = true;
            for (ListDigraph::OutArcIt succArc(graph,predNode); succArc != lemon::INVALID; ++succArc) {
                auto succNode = graph.target(succArc);
                if (!scheduled.at(instruction[succNode])) {
                    schedulable = false;
                    break;
                }
            }
            if (schedulable) {
                MakeAvailable(predNode, avlist, dir);
            }
        }
    }
}

// advance curr_cycle
// when no node was selected from the avlist, advance to the next cycle
// and try again; this makes nodes/instructions to complete execution for one more cycle,
// and makes resources finally available in case of resource constrained scheduling
// so it contributes to proceeding and to finally have an empty avlist
void Scheduler::AdvanceCurrCycle(scheduling_direction_t dir, UInt &curr_cycle) {
    if (forward_scheduling == dir) {
        curr_cycle++;
    } else {
        curr_cycle--;
    }
}

// a gate must wait until all its operand are available, i.e. the gates having computed them have completed,
// and must wait until all resources required for the gate's execution are available;
// return true when immediately schedulable
// when returning false, isres indicates whether resource occupation was the reason or operand completion (for debugging)
Bool Scheduler::immediately_schedulable(
    ListDigraph::Node n,
    scheduling_direction_t dir,
    const UInt curr_cycle,
    const quantum_platform& platform,
    arch::resource_manager_t &rm,
    Bool &isres
) {
    gate *gp = instruction[n];
    isres = true;
    // have dependent gates completed at curr_cycle?
    if (
        (forward_scheduling == dir && gp->cycle <= curr_cycle)
        || (backward_scheduling == dir && curr_cycle <= gp->cycle)
        ) {
        // are resources available?
        if (
            n == s || n == t
            || gp->type() == gate_type_t::__dummy_gate__
            || gp->type() == gate_type_t::__classical_gate__
            || gp->type() == gate_type_t::__wait_gate__
            ) {
            return true;
        }
        if (rm.available(curr_cycle, gp, platform)) {
            return true;
        }
        isres = true;;
        return false;
    } else {
        isres = false;
        return false;
    }
}

// select a node from the avlist
// the avlist is deep-ordered from high to low criticality (see criticality_lessthan above)
ListDigraph::Node Scheduler::SelectAvailable(
    List<ListDigraph::Node> &avlist,
    scheduling_direction_t dir,
    const UInt curr_cycle,
    const quantum_platform &platform,
    arch::resource_manager_t &rm,
    Bool &success
) {
    success = false;                        // whether a node was found and returned

    QL_DOUT("avlist(@" << curr_cycle << "):");
    for (auto n : avlist) {
        QL_DOUT("...... node(@" << instruction[n]->cycle << "): " << name[n] << " remaining: " << remaining.dbg(n));
    }

    // select the first (most critical) immediately schedulable gate that has duration 0
    for (auto n : avlist) {
        Bool isres;
        if (instruction[n]->duration == 0 && immediately_schedulable(n, dir, curr_cycle, platform, rm, isres)) {
            QL_DOUT("... node (@" << instruction[n]->cycle << "): " << name[n] << " duration 0 and immediately schedulable, remaining=" << remaining.dbg(n) << ", selected");
            success = true;
            return n;
        }
    }
    // select the first (most critical) immediately schedulable, if any, otherwise
    // since avlist is deep-criticality ordered, highest first, the first is the most deep-critical
    for (auto n : avlist) {
        Bool isres;
        if (immediately_schedulable(n, dir, curr_cycle, platform, rm, isres)) {
            QL_DOUT("... node (@" << instruction[n]->cycle << "): " << name[n] << " immediately schedulable, remaining=" << remaining.dbg(n) << ", selected");
            success = true;
            return n;
        } else {
            QL_DOUT("... node (@" << instruction[n]->cycle << "): " << name[n] << " remaining=" << remaining.dbg(n) << ", waiting for " << (isres ? "resource" : "dependent completion"));
        }
    }

    success = false;
    return s;   // fake return value
}

// ASAP/ALAP scheduler with RC
//
// schedule the circuit that is in the dependency graph
// for the given direction, with the given platform and resource manager;
// what is done, is:
// - the cycle attribute of the gates will be set according to the scheduling method
// - *circp (the original and result circuit) is sorted in the new cycle order
// the bundles are returned, with private start/duration attributes
void Scheduler::schedule(
    circuit *circp,
    scheduling_direction_t dir,
    const quantum_platform &platform,
    arch::resource_manager_t &rm,
    Str &sched_dot
) {
    QL_DOUT("Scheduling " << (forward_scheduling == dir ? "ASAP" : "ALAP") << " with RC ...");

    // scheduled[gp] :=: whether gate *gp has been scheduled, init all false
    Map<gate*, Bool> scheduled;
    // avlist :=: list of schedulable nodes, initially (see below) just s or t
    List<ListDigraph::Node> avlist;

    // initializations for this scheduler
    // note that dependency graph is not modified by a scheduler, so it can be reused
    QL_DOUT("... initialization");
    for (ListDigraph::NodeIt n(graph); n != lemon::INVALID; ++n) {
        scheduled.set(instruction[n]) = false;   // none were scheduled, including SOURCE/SINK
    }
    UInt  curr_cycle;         // current cycle for which instructions are sought
    init_available(avlist, dir, curr_cycle);     // first node (SOURCE/SINK) is made available and curr_cycle set
    set_remaining(dir);         // for each gate, number of cycles until end of schedule

    QL_DOUT("... loop over avlist until it is empty");
    while (!avlist.empty()) {
        Bool success;
        ListDigraph::Node selected_node;

        selected_node = SelectAvailable(avlist, dir, curr_cycle, platform, rm, success);
        if (!success) {
            // i.e. none from avlist was found suitable to schedule in this cycle
            AdvanceCurrCycle(dir, curr_cycle);
            // so try again; eventually instrs complete and machine is empty
            continue;
        }

        // commit selected_node to the schedule
        gate* gp = instruction[selected_node];
        QL_DOUT("... selected " << gp->qasm() << " in cycle " << curr_cycle);
        gp->cycle = curr_cycle;                     // scheduler result, including s and t
        if (
            selected_node != s
            && selected_node != t
            && gp->type() != gate_type_t::__dummy_gate__
            && gp->type() != gate_type_t::__classical_gate__
            && gp->type() != gate_type_t::__wait_gate__
            ) {
            rm.reserve(curr_cycle, gp, platform);
        }
        TakeAvailable(selected_node, avlist, scheduled, dir);   // update avlist/scheduled/cycle
        // more nodes that could be scheduled in this cycle, will be found in an other round of the loop
    }

    QL_DOUT("... sorting on cycle value");
    sort_by_cycle(circp);

    if (dir == backward_scheduling) {
        // readjust cycle values of gates so that SOURCE is at 0
        UInt SOURCECycle = instruction[s]->cycle;
        QL_DOUT("... readjusting cycle values by -" << SOURCECycle);

        instruction[t]->cycle -= SOURCECycle;
        for (auto & gp : *circp) {
            gp->cycle -= SOURCECycle;
        }
        instruction[s]->cycle -= SOURCECycle;   // i.e. becomes 0
    }
    // FIXME HvS cycles_valid now

    if (options::get("print_dot_graphs") == "yes") {
        StrStrm ssdot;
        get_dot(false, true, ssdot);
        sched_dot = ssdot.str();
    }

    // end scheduling

    QL_DOUT("Scheduling " << (forward_scheduling == dir ? "ASAP" : "ALAP") << " with RC [DONE]");
}

void Scheduler::schedule_asap(
    arch::resource_manager_t &rm,
    const quantum_platform &platform,
    Str &sched_dot
) {
    QL_DOUT("Scheduling ASAP");
    schedule(circp, forward_scheduling, platform, rm, sched_dot);
    QL_DOUT("Scheduling ASAP [DONE]");
}

void Scheduler::schedule_alap(
    arch::resource_manager_t &rm,
    const quantum_platform &platform,
    Str &sched_dot
) {
    QL_DOUT("Scheduling ALAP");
    schedule(circp, backward_scheduling, platform, rm, sched_dot);
    QL_DOUT("Scheduling ALAP [DONE]");
}

void Scheduler::schedule_alap_uniform() {
    // algorithm based on "Balanced Scheduling and Operation Chaining in High-Level Synthesis for FPGA Designs"
    // by David C. Zaretsky, Gaurav Mittal, Robert P. Dick, and Prith Banerjee
    // Figure 3. Balanced scheduling algorithm
    // Modifications:
    // - dependency analysis in article figure 2 is O(n^2) because of set union
    //   this has been left out, using our own linear dependency analysis creating a digraph
    //   and using the alap values as measure instead of the dep set size computed in article's D[n]
    // - balanced scheduling algorithm dominates with its O(n^2) when it cannot find a node to forward
    //   no test has been devised yet to break the loop (figure 3, line 14-35)
    // - targeted bundle size is adjusted each cycle and is number_of_gates_to_go/number_of_non_empty_bundles_to_go
    //   this is more greedy, preventing oscillation around a target size based on all bundles,
    //   because local variations caused by local dep chains create small bundles and thus leave more gates still to go
    //
    // Oddly enough, it starts off with an ASAP schedule.
    // This creates bundles which on average are larger at lower cycle values (opposite to ALAP).
    // After this, it moves gates up in the direction of the higher cycles but, of course, at most to their ALAP cycle
    // to fill up the small bundles at the higher cycle values to the targeted uniform length, without extending the circuit.
    // It does this in a backward scan (as ALAP scheduling would do), so bundles at the highest cycles are filled up first,
    // and such that the circuit's depth is not enlarged and the dependencies/latencies are obeyed.
    // Hence, the result resembles an ALAP schedule with excess bundle lengths solved by moving nodes down ("rolling pin").

    QL_DOUT("Scheduling ALAP UNIFORM to get bundles ...");

    // initialize gp->cycle as ASAP cycles as first approximation of result;
    // note that the circuit doesn't contain the SOURCE and SINK gates but the dependency graph does;
    // from SOURCE is a weight 1 dep to the first nodes using each qubit and classical register, and to the SINK gate
    // is a dep from each unused qubit/classical register result with as weight the duration of the last operation.
    // SOURCE (node s) is at cycle 0 and the first circuit's gates are at cycle 1.
    // SINK (node t) is at the earliest cycle that all gates/operations have completed.
    set_cycle(forward_scheduling);
    UInt cycle_count = instruction[t]->cycle - 1;
    // so SOURCE at cycle 0, then all circuit's gates at cycles 1 to cycle_count, and finally SINK at cycle cycle_count+1

    // compute remaining which is the opposite of the alap cycle value (remaining[node] :=: SINK->cycle - alapcycle[node])
    // remaining[node] indicates number of cycles remaining in schedule from node's execution start to SINK,
    // and indicates the latest cycle that the node can be scheduled so that the circuit's depth is not increased.
    set_remaining(forward_scheduling);

    // DOUT("Creating gates_per_cycle");
    // create gates_per_cycle[cycle] = for each cycle the list of gates at cycle cycle
    // this is the basic map to be operated upon by the uniforming scheduler below;
    Map<UInt, List<gate*>> gates_per_cycle;
    for (auto gp : *circp) {
        gates_per_cycle.set(gp->cycle).push_back(gp);
    }

    // DOUT("Displaying circuit and bundle statistics");
    // to compute how well the algorithm is doing, two measures are computed:
    // - the largest number of gates in a cycle in the circuit,
    // - and the average number of gates in non-empty cycles
    // this is done before and after uniform scheduling, and printed
    UInt max_gates_per_cycle = 0;
    UInt non_empty_bundle_count = 0;
    UInt gate_count = 0;
    for (UInt curr_cycle = 1; curr_cycle <= cycle_count; curr_cycle++) {
        max_gates_per_cycle = max<UInt>(max_gates_per_cycle, gates_per_cycle.get(curr_cycle).size());
        if (!gates_per_cycle.get(curr_cycle).empty()) {
            non_empty_bundle_count++;
        }
        gate_count += gates_per_cycle.get(curr_cycle).size();
    }
    Real avg_gates_per_cycle = Real(gate_count)/cycle_count;
    Real avg_gates_per_non_empty_cycle = Real(gate_count)/non_empty_bundle_count;
    QL_DOUT("... before uniform scheduling:"
             << " cycle_count=" << cycle_count
             << "; gate_count=" << gate_count
             << "; non_empty_bundle_count=" << non_empty_bundle_count
    );
    QL_DOUT("... and max_gates_per_cycle=" << max_gates_per_cycle
                                           << "; avg_gates_per_cycle=" << avg_gates_per_cycle
                                           << "; avg_gates_per_non_empty_cycle=" << avg_gates_per_non_empty_cycle
    );

    // in a backward scan, make non-empty bundles max avg_gates_per_non_empty_cycle long;
    // an earlier version of the algorithm aimed at making bundles max avg_gates_per_cycle long
    // but that flawed because of frequent empty bundles causing this estimate for a uniform length being too low
    // DOUT("Backward scan uniform scheduling");
    for (UInt curr_cycle = cycle_count; curr_cycle >= 1; curr_cycle--) {
        // Backward with pred_cycle from curr_cycle-1 down to 1, look for node(s) to fill up current too small bundle.
        // After an iteration at cycle curr_cycle, all bundles from curr_cycle to cycle_count have been filled up,
        // and all bundles from 1 to curr_cycle-1 still have to be done.
        // This assumes that current bundle is never too long, excess having been moved away earlier, as ASAP does.
        // When such a node cannot be found, this loop scans the whole circuit for each original node to fill up
        // and this creates a O(n^2) time complexity.
        //
        // A test to break this prematurely based on the current data structure, wasn't devised yet.
        // A solution is to use the dep graph instead to find a node to fill up the current node,
        // i.e. maintain a so-called "available list" of nodes free to schedule, as in the non-uniform scheduling algorithm,
        // which is not hard at all but which is not according to the published algorithm.
        // When the complexity becomes a problem, it is proposed to rewrite the algorithm accordingly.

        long pred_cycle = curr_cycle - 1;    // signed because can become negative

        // target size of each bundle is number of gates still to go divided by number of non-empty cycles to go
        // it averages over non-empty bundles instead of all bundles because the latter would be very strict
        // it is readjusted during the scan to cater for dips in bundle size caused by local dependency chains
        if (non_empty_bundle_count == 0) break;     // nothing to do
        avg_gates_per_cycle = Real(gate_count)/curr_cycle;
        avg_gates_per_non_empty_cycle = Real(gate_count)/non_empty_bundle_count;
        QL_DOUT("Cycle=" << curr_cycle << " number of gates=" << gates_per_cycle.get(curr_cycle).size()
                         << "; avg_gates_per_cycle=" << avg_gates_per_cycle
                         << "; avg_gates_per_non_empty_cycle=" << avg_gates_per_non_empty_cycle);

        while (Real(gates_per_cycle.get(curr_cycle).size()) < avg_gates_per_non_empty_cycle && pred_cycle >= 1) {
            QL_DOUT("pred_cycle=" << pred_cycle);
            QL_DOUT("gates_per_cycle[curr_cycle].size()=" << gates_per_cycle.get(curr_cycle).size());
            UInt min_remaining_cycle = MAX_CYCLE;
            gate *best_predgp;
            Bool best_predgp_found = false;

            // scan bundle at pred_cycle to find suitable candidate to move forward to curr_cycle
            for (auto predgp : gates_per_cycle.get(pred_cycle)) {
                Bool forward_predgp = true;
                UInt predgp_completion_cycle;
                ListDigraph::Node pred_node = node.at(predgp);
                QL_DOUT("... considering: " << predgp->qasm() << " @cycle=" << predgp->cycle << " remaining=" << remaining.dbg(pred_node));

                // candidate's result, when moved, must be ready before end-of-circuit and before used
                predgp_completion_cycle = curr_cycle + UInt(ceil(static_cast<Real>(predgp->duration)/cycle_time));
                // predgp_completion_cycle = curr_cycle + (predgp->duration+cycle_time-1)/cycle_time;
                if (predgp_completion_cycle > cycle_count + 1) { // at SINK is ok, later not
                    forward_predgp = false;
                    QL_DOUT("... ... rejected (after circuit): " << predgp->qasm() << " would complete @" << predgp_completion_cycle << " SINK @" << cycle_count + 1);
                } else {
                    for (ListDigraph::OutArcIt arc(graph,pred_node); arc != lemon::INVALID; ++arc) {
                        gate *target_gp = instruction[graph.target(arc)];
                        UInt target_cycle = target_gp->cycle;
                        if (predgp_completion_cycle > target_cycle) {
                            forward_predgp = false;
                            QL_DOUT("... ... rejected (after succ): " << predgp->qasm() << " would complete @" << predgp_completion_cycle << " target=" << target_gp->qasm() << " target_cycle=" << target_cycle);
                        }
                    }
                }

                // when multiple nodes in bundle qualify, take the one with lowest remaining
                // because that is the most critical one and thus deserves a cycle as high as possible (ALAP)
                if (forward_predgp && remaining.at(pred_node) < min_remaining_cycle) {
                    min_remaining_cycle = remaining.at(pred_node);
                    best_predgp_found = true;
                    best_predgp = predgp;
                }
            }

            // when candidate was found in this bundle, move it, and search for more in this bundle, if needed
            // otherwise, continue scanning backward
            if (best_predgp_found) {
                // move predgp from pred_cycle to curr_cycle;
                // adjust all bookkeeping that is affected by this
                gates_per_cycle.at(pred_cycle).remove(best_predgp);
                if (gates_per_cycle.at(pred_cycle).empty()) {
                    // source bundle was non-empty, now it is empty
                    non_empty_bundle_count--;
                }
                if (gates_per_cycle.get(curr_cycle).empty()) {
                    // target bundle was empty, now it will be non_empty
                    non_empty_bundle_count++;
                }
                best_predgp->cycle = curr_cycle;        // what it is all about
                gates_per_cycle.set(curr_cycle).push_back(best_predgp);

                // recompute targets
                if (non_empty_bundle_count == 0) break;     // nothing to do
                avg_gates_per_cycle = Real(gate_count)/curr_cycle;
                avg_gates_per_non_empty_cycle = Real(gate_count)/non_empty_bundle_count;
                QL_DOUT("... moved " << best_predgp->qasm() << " with remaining=" << remaining.dbg(node.at(best_predgp))
                                     << " from cycle=" << pred_cycle << " to cycle=" << curr_cycle
                                     << "; new avg_gates_per_cycle=" << avg_gates_per_cycle
                                     << "; avg_gates_per_non_empty_cycle=" << avg_gates_per_non_empty_cycle
                );
            } else {
                pred_cycle --;
            }
        }   // end for finding a bundle to forward a node from to the current cycle

        // curr_cycle ready, recompute counts for remaining cycles
        // mask current cycle and its gates from the target counts:
        // - gate_count, non_empty_bundle_count, curr_cycle (as cycles still to go)
        gate_count -= gates_per_cycle.get(curr_cycle).size();
        if (!gates_per_cycle.get(curr_cycle).empty()) {
            // bundle is non-empty
            non_empty_bundle_count--;
        }
    }   // end curr_cycle loop; curr_cycle is bundle which must be enlarged when too small

    // new cycle values computed; reflect this in circuit's gate order
    sort_by_cycle(circp);
    // FIXME HvS cycles_valid now

    // recompute and print statistics reporting on uniform scheduling performance
    max_gates_per_cycle = 0;
    non_empty_bundle_count = 0;
    gate_count = 0;
    // cycle_count was not changed
    for (UInt curr_cycle = 1; curr_cycle <= cycle_count; curr_cycle++) {
        max_gates_per_cycle = max<UInt>(max_gates_per_cycle, gates_per_cycle.get(curr_cycle).size());
        if (!gates_per_cycle.get(curr_cycle).empty()) {
            non_empty_bundle_count++;
        }
        gate_count += gates_per_cycle.get(curr_cycle).size();
    }
    avg_gates_per_cycle = Real(gate_count)/cycle_count;
    avg_gates_per_non_empty_cycle = Real(gate_count)/non_empty_bundle_count;
    QL_DOUT("... after uniform scheduling:"
             << " cycle_count=" << cycle_count
             << "; gate_count=" << gate_count
             << "; non_empty_bundle_count=" << non_empty_bundle_count
    );
    QL_DOUT("... and max_gates_per_cycle=" << max_gates_per_cycle
                                           << "; avg_gates_per_cycle=" << avg_gates_per_cycle
                                           << "; ..._per_non_empty_cycle=" << avg_gates_per_non_empty_cycle
    );

    QL_DOUT("Scheduling ALAP UNIFORM [DONE]");
}

// printing dot of the dependency graph
void Scheduler::get_dot(
    Bool WithCritical,
    Bool WithCycles,
    std::ostream &dotout
) {
    QL_DOUT("Get_dot");
    ListDigraphPath p;
    ListDigraph::ArcMap<Bool> isInCritical{graph};
    if (WithCritical) {
        for (ListDigraph::ArcIt a(graph); a != lemon::INVALID; ++a) {
            isInCritical[a] = false;
            for (ListDigraphPath::ArcIt ap(p); ap != lemon::INVALID; ++ap) {
                if (a == ap) {
                    isInCritical[a] = true;
                    break;
                }
            }
        }
    }

    Str NodeStyle(" fontcolor=black, style=filled, fontsize=16");
    Str EdgeStyle1(" color=black");
    Str EdgeStyle2(" color=red");
    Str EdgeStyle = EdgeStyle1;

    dotout << "digraph {\ngraph [ rankdir=TD; ]; // or rankdir=LR"
           << "\nedge [fontsize=16, arrowhead=vee, arrowsize=0.5];"
           << std::endl;

    // first print the nodes
    for (ListDigraph::NodeIt n(graph); n != lemon::INVALID; ++n) {
        dotout  << "\"" << graph.id(n) << "\""
                << " [label=\" " << name[n] <<" \""
                << NodeStyle
                << "];" << std::endl;
    }

    if (WithCycles) {
        // Print cycle numbers as timeline, as shown below
        UInt TotalCycles;
        if (circp->empty()) {
            TotalCycles = 1;    // +1 is SOURCE's duration in cycles
        } else {
            TotalCycles = circp->back()->cycle + (circp->back()->duration+cycle_time-1)/cycle_time
                          - circp->front()->cycle + 1;    // +1 is SOURCE's duration in cycles
        }
        dotout << "{\nnode [shape=plaintext, fontsize=16, fontcolor=blue]; \n";
        for (UInt cn = 0; cn <= TotalCycles; ++cn) {
            if (cn > 0) {
                dotout << " -> ";
            }
            dotout << "Cycle" << cn;
        }
        dotout << ";\n}\n";

        // Now print ranks, as shown below
        dotout << "{ rank=same; Cycle" << instruction[s]->cycle <<"; " << graph.id(s) << "; }\n";
        for (auto gp : *circp) {
            dotout << "{ rank=same; Cycle" << gp->cycle <<"; " << graph.id(node.at(gp)) << "; }\n";
        }
        dotout << "{ rank=same; Cycle" << instruction[t]->cycle <<"; " << graph.id(t) << "; }\n";
    }

    // now print the edges
    for (ListDigraph::ArcIt arc(graph); arc != lemon::INVALID; ++arc) {
        auto srcNode = graph.source(arc);
        auto dstNode = graph.target(arc);
        Int srcID = graph.id( srcNode );
        Int dstID = graph.id( dstNode );

        if (WithCritical) {
            EdgeStyle = (isInCritical[arc] == true) ? EdgeStyle2 : EdgeStyle1;
        }

        dotout << std::dec
               << "\"" << srcID << "\""
               << "->"
               << "\"" << dstID << "\""
               << "[ label=\""
               << "q" << cause[arc]
               << " , " << weight[arc]
               << " , " << DepTypesNames[ depType[arc] ]
               <<"\""
               << " " << EdgeStyle << " "
               << "]"
               << std::endl;
    }

    dotout << "}" << std::endl;
    QL_DOUT("Get_dot[DONE]");
}

void Scheduler::get_dot(Str &dot) {
    set_cycle(forward_scheduling);
    sort_by_cycle(circp);

    StrStrm ssdot;
    get_dot(false, true, ssdot);
    dot = ssdot.str();
}

// schedule support for program.h::schedule()
void schedule_kernel(
    quantum_kernel &kernel,
    const quantum_platform &platform,
    Str &dot,
    Str &sched_dot
) {
    Str scheduler = options::get("scheduler");
    Str scheduler_uniform = options::get("scheduler_uniform");

    QL_IOUT(scheduler << " scheduling the quantum kernel '" << kernel.name << "'...");

    Scheduler sched;
    sched.init(kernel.c, platform, kernel.qubit_count, kernel.creg_count, kernel.breg_count);

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
        QL_FATAL("Not supported scheduler option: scheduler=" << scheduler);
    }
    QL_DOUT(scheduler << " scheduling the quantum kernel '" << kernel.name << "' DONE");
    kernel.cycles_valid = true;
}

/*
 * main entry to the non resource-constrained scheduler
 */
void schedule(
    quantum_program *programp,
    const quantum_platform &platform,
    const Str &passname
) {
    if (options::get("prescheduler") == "yes") {
        report_statistics(programp, platform, "in", passname, "# ");
        report_qasm(programp, platform, "in", passname);

        QL_IOUT("scheduling the quantum program");
        for (auto &k : programp->kernels) {
            Str dot;
            Str kernel_sched_dot;
            schedule_kernel(k, platform, dot, kernel_sched_dot);

            if (options::get("print_dot_graphs") == "yes") {
                Str fname;
                fname = options::get("output_dir") + "/" + k.get_name() + "_dependence_graph.dot";
                QL_IOUT("writing scheduled dot to '" << fname << "' ...");
                OutFile(fname).write(dot);

                Str scheduler_opt = options::get("scheduler");
                fname = options::get("output_dir") + "/" + k.get_name() + scheduler_opt + "_scheduled.dot";
                QL_IOUT("writing scheduled dot to '" << fname << "' ...");
                OutFile(fname).write(kernel_sched_dot);
            }
        }

        report_statistics(programp, platform, "out", passname, "# ");
        report_qasm(programp, platform, "out", passname);
    }
}

void rcschedule_kernel(
    quantum_kernel &kernel,
    const quantum_platform &platform,
    Str &dot,
    UInt nqubits,
    UInt ncreg,
    UInt nbreg
) {
    QL_IOUT("Resource constraint scheduling ...");

    Str schedopt = options::get("scheduler");
    if (schedopt == "ASAP") {
        Scheduler sched;
        sched.init(kernel.c, platform, nqubits, ncreg, nbreg);

        arch::resource_manager_t rm(platform, forward_scheduling);
        sched.schedule_asap(rm, platform, dot);
    } else if (schedopt == "ALAP") {
        Scheduler sched;
        sched.init(kernel.c, platform, nqubits, ncreg, nbreg);

        arch::resource_manager_t rm(platform, backward_scheduling);
        sched.schedule_alap(rm, platform, dot);
    } else {
        QL_FATAL("Not supported scheduler option: scheduler=" << schedopt);
    }

    QL_IOUT("Resource constraint scheduling [Done].");
}

/*
 * main entry point of the rcscheduler
 */
void rcschedule(
    quantum_program *programp,
    const quantum_platform &platform,
    const Str &passname
) {
    report_statistics(programp, platform, "in", passname, "# ");
    report_qasm(programp, platform, "in", passname);

    for (auto &kernel : programp->kernels) {
        QL_IOUT("Scheduling kernel: " << kernel.name);
        if (!kernel.c.empty()) {
            auto num_creg = kernel.creg_count;
            auto num_breg = kernel.breg_count;
            Str sched_dot;

            rcschedule_kernel(kernel, platform, sched_dot, platform.qubit_number, num_creg, num_breg);
            kernel.cycles_valid = true; // FIXME HvS move this back into call to right after sort_cycle

            if (options::get("print_dot_graphs") == "yes") {
                StrStrm fname;
                fname << options::get("output_dir") << "/" << kernel.name << "_" << passname << ".dot";
                QL_IOUT("writing " << passname << " dependency graph dot file to '" << fname.str() << "' ...");
                OutFile(fname.str()).write(sched_dot);
            }
        }
    }

    report_statistics(programp, platform, "out", passname, "# ");
    report_qasm(programp, platform, "out", passname);
}

} // namespace ql
