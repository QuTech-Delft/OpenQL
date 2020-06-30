/**
 * @file   scheduler.h
 * @date   01/2017
 * @author Imran Ashraf
 * @author Hans van Someren
 * @brief  ASAP/ALAP critical path and UNIFORM scheduling with and without resource constraint
 */

#ifndef _SCHEDULER_H
#define _SCHEDULER_H

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

#include "utils.h"
#include "gate.h"
#include "circuit.h"
#include "ir.h"
#include "resource_manager.h"

using namespace std;
using namespace lemon;

// see above/below for the meaning of R, W, and D events and their relation to dependences
enum DepTypes{RAW, WAW, WAR, RAR, RAD, DAR, DAD, WAD, DAW};
const string DepTypesNames[] = {"RAW", "WAW", "WAR", "RAR", "RAD", "DAR", "DAD", "WAD", "DAW"};

class Scheduler
{
public:
    // dependence graph is constructed (see Init) once from the sequence of gates in a kernel's circuit
    // it can be reused as often as needed as long as no gates are added/deleted; it doesn't modify those gates
    ListDigraph graph;

    // conversion between gate* (pointer to the gate in the circuit) and node (of the dependence graph)
    ListDigraph::NodeMap<ql::gate*> instruction;// instruction[n] == gate*
    std::map<ql::gate*,ListDigraph::Node>  node;// node[gate*] == n

    // attributes
    ListDigraph::NodeMap<std::string> name;     // name[n] == qasm string
    ListDigraph::ArcMap<int> weight;            // number of cycles of dependence
    ListDigraph::ArcMap<int> cause;             // qubit/creg index of dependence
    ListDigraph::ArcMap<int> depType;           // RAW, WAW, ...

    // s and t nodes are the top and bottom of the dependence graph
    ListDigraph::Node s, t;                     // instruction[s]==SOURCE, instruction[t]==SINK

    // parameters of dependence graph construction
    size_t          cycle_time;                 // to convert durations to cycles as weight of dependence
    size_t          qubit_count;                // number of qubits, to check/represent qubit as cause of dependence
    size_t          creg_count;                 // number of cregs, to check/represent creg as cause of dependence
    ql::circuit*    circp;                      // current and result circuit, passed from Init to each scheduler

    // scheduler support
    std::map< std::pair<std::string,std::string>, size_t> buffer_cycles_map;
    std::map<ListDigraph::Node,size_t>  remaining;  // remaining[node] == cycles until end; critical path representation


public:
    Scheduler(): instruction(graph), name(graph), weight(graph),
        cause(graph), depType(graph) {}

    // ins->name may contain parameters, so must be stripped first before checking it for gate's name
    void stripname(std::string& name)
    {
        size_t p = name.find(" ");
        if (p != std::string::npos)
        {
            name = name.substr(0,p);
        }
    }

    // factored out code from Init to add a dependence between two nodes
    // operand is in qubit_creg combined index space
    void add_dep(int srcID, int tgtID, enum DepTypes deptype, int operand)
    {
        DOUT(".. adddep ...");
        ListDigraph::Node srcNode = graph.nodeFromId(srcID);
        ListDigraph::Node tgtNode = graph.nodeFromId(tgtID);
        ListDigraph::Arc arc = graph.addArc(srcNode,tgtNode);
        weight[arc] = int(std::ceil( static_cast<float>(instruction[srcNode]->duration) / cycle_time));
        // weight[arc] = (instruction[srcNode]->duration + cycle_time -1)/cycle_time;
        cause[arc] = operand;
        depType[arc] = deptype;
        DOUT("... dep " << name[srcNode] << " -> " << name[tgtNode] << " (opnd=" << operand << ", dep=" << DepTypesNames[deptype] << ", wght=" << weight[arc] << ")");
    }

    // fill the dependence graph ('graph') with nodes from the circuit and adding arcs for their dependences
    void init(ql::circuit& ckt, ql::quantum_platform platform, size_t qcount, size_t ccount)
    {
        DOUT("Dependence graph creation ...");
        qubit_count = qcount;
        creg_count = ccount;
        size_t qubit_creg_count = qubit_count + creg_count;
        DOUT("Scheduler.init: qubit_count=" << qubit_count << ", creg_count=" << creg_count << ", total=" << qubit_creg_count);
        cycle_time = platform.cycle_time;
        circp = &ckt;

        // populate buffer map
        // 'none' type is a dummy type and 0 buffer cycles will be inserted for
        // instructions of type 'none'
        //
        // this has nothing to do with dependence graph generation but with scheduling
        // so should be in resource-constrained scheduler constructor
        std::vector<std::string> buffer_names = {"none", "mw", "flux", "readout"};
        for(auto & buf1 : buffer_names)
        {
            for(auto & buf2 : buffer_names)
            {
                auto bpair = std::pair<std::string,std::string>(buf1,buf2);
                auto bname = buf1+ "_" + buf2 + "_buffer";
                if(platform.hardware_settings.count(bname) > 0)
                {
                    buffer_cycles_map[ bpair ] = size_t(std::ceil(
                        static_cast<float>(platform.hardware_settings[bname]) / cycle_time));
                }
                // DOUT("Initializing " << bname << ": "<< buffer_cycles_map[bpair]);
            }
        }

        // dependences are created with a current gate as target
        // and with those previous gates as source that have an operand match:
        // - the previous gates that Read r in LastReaders[r]; this is a list
        // - the previous gates that D qubit q in LastDs[q]; this is a list
        // - the previous gate that Wrote r in LastWriter[r]; this can only be one
        // operands can be a qubit or a classical register
        typedef vector<int> ReadersListType;

        vector<ReadersListType> LastReaders;
        LastReaders.resize(qubit_creg_count);

        vector<ReadersListType> LastDs;
        LastDs.resize(qubit_creg_count);

        // start filling the dependence graph by creating the s node, the top of the graph
        {
            // add dummy source node
            ListDigraph::Node srcNode = graph.addNode();
            instruction[srcNode] = new ql::SOURCE();    // so SOURCE is defined as instruction[s], not unique in itself
            node[instruction[srcNode]] = srcNode;
            name[srcNode] = instruction[srcNode]->qasm();
            s=srcNode;
        }
        int srcID = graph.id(s);
        vector<int> LastWriter(qubit_creg_count,srcID);     // it implicitly writes to all qubits and class. regs

        // for each gate pointer ins in the circuit, add a node and add dependences from previous gates to it
        for( auto ins : ckt )
        {
            DOUT("Current instruction's name: `" << ins->name << "'");
            DOUT(".. Qasm(): " << ins->qasm());
            for( auto operand : ins->operands ) DOUT(".. Operand: `" << operand << "'");
            for( auto coperand : ins->creg_operands ) DOUT(".. Classical operand: `" << coperand << "'");

            auto iname = ins->name; // copy!!!!
            stripname(iname);

            // Add node
            ListDigraph::Node consNode = graph.addNode();
            int consID = graph.id(consNode);
            instruction[consNode] = ins;
            node[ins] = consNode;
            name[consNode] = ins->qasm();

            // Add edges (arcs)
            // In quantum computing there are no real Reads and Writes on qubits because they cannot be cloned.
            // Every qubit use influences the qubit, updates it, so would be considered a Read+Write at the same time.
            // In dependence graph construction, this leads to WAW-dependence chains of all uses of the same qubit,
            // and hence in a scheduler using this graph to a sequentialization of those uses in the original program order.
            //
            // For a scheduler, only the presence of a dependence counts, not its type (RAW/WAW/etc.).
            // A dependence graph also has other uses apart from the scheduler: e.g. to find chains of live qubits,
            // from their creation (Prep etc.) to their destruction (Measure, etc.) in allocation of virtual to real qubits.
            // For those uses it makes sense to make a difference with a gate doing a Read+Write, just a Write or just a Read:
            // a Prep creates a new 'value' (Write); wait, display, x, swap, cnot, all pass this value on (so Read+Write),
            // while a Measure 'destroys' the 'value' (Read+Write of the qubit, Write of the creg),
            // the destruction aspect of a Measure being implied by it being followed by a Prep (Write only) on the same qubit.
            // Furthermore Writes can model barriers on a qubit (see Wait, Display, etc.), because Writes sequentialize.
            // The dependence graph creation below models a graph suitable for all functions, including chains of live qubits.

            // Control-operands of Controlled Unitaries commute, independent of the Unitary,
            // i.e. these gates need not be kept in order.
            // But, of course, those qubit uses should be ordered after (/before) the last (/next) non-control use of the qubit.
            // In this way, those control-operand qubit uses would be like pure Reads in dependence graph construction.
            // A problem might be that the gates with the same control-operands might be scheduled in parallel then.
            // In a non-resource scheduler that will happen but it doesn't do harm because it is not a real machine.
            // In a resource-constrained scheduler the resource constraint that prohibits more than one use
            // of the same qubit being active at the same time, will prevent this parallelism.
            // So ignoring Read After Read (RAR) dependences enables the scheduler to take advantage
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
            // in which 'no' indicates no dependence from left event to top event and '/' indicates a dependence from left to top.
            //
            //             W   R   D                  w   R   D
            //        W    /   /   /              W   WAW RAW DAW
            //        R    /   no  /              R   WAR RAR DAR
            //        D    /   /   no             D   WAD RAD DAD
            //
            // In addition to LastReaders, we introduce LastDs.
            // Either one is cleared when dependences are generated from them, and extended otherwise.
            // From the table it can be seen that the D 'behaves' as a Write to Read, and as a Read to Write,
            // that there is no order among Ds nor among Rs, but D after R and R after D sequentialize.
            // With this, the dependence graph is claimed to represent the commutations as above.
            //
            // The schedulers are list schedulers, i.e. they maintain a list of gates in their algorithm,
            // of gates available for being scheduled because they are not blocked by dependences on non-scheduled gates.
            // Therefore, the schedulers are able to select the best one from a set of commutable gates.

            // TODO: define signature in .json file similar to how gcc defines instructions
            // and then have a signature interpreter here; then we don't have this long if-chain
            // and, more importantly, we don't have the knowledge of particular gates here;
            // the default signature would be that of a default gate, modifying each qubit operand.

            // each type of gate has a different 'signature' of events; switch out to each one
            if(iname == "measure")
            {
                DOUT(". considering " << name[consNode] << " as measure");
                // Read+Write each qubit operand + Write corresponding creg
                auto operands = ins->operands;
                for( auto operand : operands )
                {
                    DOUT(".. Operand: " << operand);
                    add_dep(LastWriter[operand], consID, WAW, operand);
                    for(auto & readerID : LastReaders[operand])
                    {
                        add_dep(readerID, consID, WAR, operand);
                    }
                    for(auto & readerID : LastDs[operand])
                    {
                        add_dep(readerID, consID, WAD, operand);
                    }
                }

                for( auto coperand : ins->creg_operands )
                {
                    DOUT(".. Classical operand: " << coperand);
                    add_dep(LastWriter[qubit_count+coperand], consID, WAW, qubit_count+coperand);
                    for(auto & readerID : LastReaders[qubit_count+coperand])
                    {
                        add_dep(readerID, consID, WAR, qubit_count+coperand);
                    }
                }

                // update LastWriter and so clear LastReaders
                for( auto operand : operands )
                {
                    DOUT(".. Update LastWriter for operand: " << operand);
                    LastWriter[operand] = consID;
                    DOUT(".. Clearing LastReaders for operand: " << operand);
                    LastReaders[operand].clear();
                    LastDs[operand].clear();
                    DOUT(".. Update LastWriter done");
                }
                for( auto coperand : ins->creg_operands )
                {
                    DOUT(".. Update LastWriter for coperand: " << coperand);
                    LastWriter[qubit_count+coperand] = consID;
                    DOUT(".. Clearing LastReaders for coperand: " << coperand);
                    LastReaders[qubit_count+coperand].clear();
                    DOUT(".. Update LastWriter done");
                }
                DOUT(". measure done");
            }
            else if(iname == "display")
            {
                DOUT(". considering " << name[consNode] << " as display");
                // no operands, display all qubits and cregs
                // Read+Write each operand
                std::vector<size_t> qubits(qubit_creg_count);
                std::iota(qubits.begin(), qubits.end(), 0);
                for( auto operand : qubits )
                {
                    DOUT(".. Operand: " << operand);
                    add_dep(LastWriter[operand], consID, WAW, operand);
                    for(auto & readerID : LastReaders[operand])
                    {
                        add_dep(readerID, consID, WAR, operand);
                    }
                    for(auto & readerID : LastDs[operand])
                    {
                        add_dep(readerID, consID, WAD, operand);
                    }
                }

                // now update LastWriter and so clear LastReaders/LastDs
                for( auto operand : qubits )
                {
                    LastWriter[operand] = consID;
                    LastReaders[operand].clear();
                    LastDs[operand].clear();
                }
            }
            else if(ins->type() == ql::gate_type_t::__classical_gate__)
            {
                DOUT(". considering " << name[consNode] << " as classical gate");
                // Read+Write each classical operand
                for( auto coperand : ins->creg_operands )
                {
                    DOUT("... Classical operand: " << coperand);
                    add_dep(LastWriter[qubit_count+coperand], consID, WAW, qubit_count+coperand);
                    for(auto & readerID : LastReaders[qubit_count+coperand])
                    {
                        add_dep(readerID, consID, WAR, qubit_count+coperand);
                    }
                    for(auto & readerID : LastDs[qubit_count+coperand])
                    {
                        add_dep(readerID, consID, WAD, qubit_count+coperand);
                    }
                }

                // now update LastWriter and so clear LastReaders/LastDs
                for( auto coperand : ins->creg_operands )
                {
                    LastWriter[qubit_count+coperand] = consID;
                    LastReaders[qubit_count+coperand].clear();
                    LastDs[qubit_count+coperand].clear();
                }
            }
            else if (  iname == "cnot"
                    )
            {
                DOUT(". considering " << name[consNode] << " as cnot");
                // CNOTs Read the first operands, and Ds the second operand
                size_t operandNo=0;
                auto operands = ins->operands;
                for( auto operand : operands )
                {
                    DOUT(".. Operand: " << operand);
                    if( operandNo == 0)
                    {
                        add_dep(LastWriter[operand], consID, RAW, operand);
	                    if (ql::options::get("scheduler_commute") == "no")
                        {
                            for(auto & readerID : LastReaders[operand])
                            {
                                add_dep(readerID, consID, RAR, operand);
                            }
                        }
                        for(auto & readerID : LastDs[operand])
                        {
                            add_dep(readerID, consID, RAD, operand);
                        }
                    }
                    else
                    {
                        add_dep(LastWriter[operand], consID, DAW, operand);
	                    if (ql::options::get("scheduler_commute") == "no")
                        {
                            for(auto & readerID : LastDs[operand])
                            {
                                add_dep(readerID, consID, DAD, operand);
                            }
                        }
                        for(auto & readerID : LastReaders[operand])
                        {
                            add_dep(readerID, consID, DAR, operand);
                        }
                    }
                    operandNo++;
                } // end of operand for

                // now update LastWriter and so clear LastReaders
                operandNo=0;
                for( auto operand : operands )
                {
                    if( operandNo == 0)
                    {
                        // update LastReaders for this operand 0
                        LastReaders[operand].push_back(consID);
                        LastDs[operand].clear();
                    }
                    else
                    {
                        LastDs[operand].push_back(consID);
	                    LastReaders[operand].clear();
                    }
                    operandNo++;
                }
            }
            else if (  iname == "cz"
                    || iname == "cphase"
                    )
            {
                DOUT(". considering " << name[consNode] << " as cz");
                // CZs Read all operands
                size_t operandNo=0;
                auto operands = ins->operands;
                for( auto operand : operands )
                {
                    DOUT(".. Operand: " << operand);
                    if (ql::options::get("scheduler_commute") == "no")
                    {
                        for(auto & readerID : LastReaders[operand])
                        {
                            add_dep(readerID, consID, RAR, operand);
                        }
                    }
                    add_dep(LastWriter[operand], consID, RAW, operand);
                    for(auto & readerID : LastDs[operand])
                    {
                        add_dep(readerID, consID, RAD, operand);
                    }
                    operandNo++;
                } // end of operand for

                // update LastReaders etc.
                operandNo=0;
                for( auto operand : operands )
                {
                    LastDs[operand].clear();
                    LastReaders[operand].push_back(consID);
                    operandNo++;
                }
            }
#ifdef HAVEGENERALCONTROLUNITARIES
            else if (
                    // or is a Control Unitary in general
                    // Read on all operands, Write on last operand
                    // before implementing it, check whether all commutativity on Reads above hold for this Control Unitary
                    )
            {
                DOUT(". considering " << name[consNode] << " as Control Unitary");
                // Control Unitaries Read all operands, and Write the last operand
                size_t operandNo=0;
                auto operands = ins->operands;
                size_t op_count = operands.size();
                for( auto operand : operands )
                {
                    DOUT(".. Operand: " << operand);
                    add_dep(LastWriter[operand], consID, RAW, operand);
                    if (ql::options::get("scheduler_commute") == "no")
                    {
                        for(auto & readerID : LastReaders[operand])
                        {
                            add_dep(readerID, consID, RAR, operand);
                        }
                    }
                    for(auto & readerID : LastDs[operand])
                    {
                        add_dep(readerID, consID, RAD, operand);
                    }

                    if( operandNo < op_count-1 )
                    {
                        LastReaders[operand].push_back(consID);
                        LastDs[operand].clear();
                    }
                    else
                    {
                        add_dep(LastWriter[operand], consID, WAW, operand);
                        for(auto & readerID : LastReaders[operand])
                        {
                            add_dep(readerID, consID, WAR, operand);
                        }
                        for(auto & readerID : LastDs[operand])
                        {
                            add_dep(readerID, consID, WAD, operand);
                        }

                        LastWriter[operand] = consID;
                        LastReaders[operand].clear();
                        LastDs[operand].clear();
                    }
                    operandNo++;
                } // end of operand for
            }
#endif  // HAVEGENERALCONTROLUNITARIES
            else
            {
                DOUT(". considering " << name[consNode] << " as no special gate (catch-all, generic rules)");
                // Read+Write on each quantum operand
                // Read+Write on each classical operand
                auto operands = ins->operands;
                for( auto operand : operands )
                {
                    DOUT(".. Operand: " << operand);
                    add_dep(LastWriter[operand], consID, WAW, operand);
                    for(auto & readerID : LastReaders[operand])
                    {
                        add_dep(readerID, consID, WAR, operand);
                    }
                    for(auto & readerID : LastDs[operand])
                    {
                        add_dep(readerID, consID, WAD, operand);
                    }

                    LastWriter[operand] = consID;
                    LastReaders[operand].clear();
                    LastDs[operand].clear();
                } // end of operand for

                // Read+Write each classical operand
                for( auto coperand : ins->creg_operands )
                {
                    DOUT("... Classical operand: " << coperand);
                    add_dep(LastWriter[qubit_count+coperand], consID, WAW, qubit_count+coperand);
                    for(auto & readerID : LastReaders[qubit_count+coperand])
                    {
                        add_dep(readerID, consID, WAR, qubit_count+coperand);
                    }
                    for(auto & readerID : LastDs[qubit_count+coperand])
                    {
                        add_dep(readerID, consID, WAD, qubit_count+coperand);
                    }

                    // now update LastWriter and so clear LastReaders/LastDs
                    LastWriter[qubit_count+coperand] = consID;
                    LastReaders[qubit_count+coperand].clear();
                    LastDs[qubit_count+coperand].clear();
                } // end of coperand for
            } // end of if/else
            DOUT(". instruction done: " << ins->qasm());
        } // end of instruction for

	    DOUT("adding deps to SINK");
        // finish filling the dependence graph by creating the t node, the bottom of the graph
        {
	        // add dummy target node
	        ListDigraph::Node consNode = graph.addNode();
	        int consID = graph.id(consNode);
	        instruction[consNode] = new ql::SINK();    // so SINK is defined as instruction[t], not unique in itself
	        node[instruction[consNode]] = consNode;
	        name[consNode] = instruction[consNode]->qasm();
	        t=consNode;

	        // add deps to the dummy target node to close the dependence chains
	        // it behaves as a W to every qubit and creg
	        //
	        // to guarantee that exactly at start of execution of dummy SINK,
	        // all still executing nodes complete, give arc weight of those nodes;
	        // this is relevant for ALAP (which starts backward from SINK for all these nodes);
	        // also for accurately computing the circuit's depth (which includes full completion);
	        // and also for implementing scheduling and mapping across control-flow (so that it is
	        // guaranteed that on a jump and on start of target circuit, the source circuit completed).
            //
            // note that there always is a LastWriter: the dummy source node wrote to every qubit and class. reg
	        std::vector<size_t> operands(qubit_creg_count);
	        std::iota(operands.begin(), operands.end(), 0);
	        for( auto operand : operands )
	        {
	            DOUT(".. Sink operand, adding dep: " << operand);
	            add_dep(LastWriter[operand], consID, WAW, operand);
	            for(auto & readerID : LastReaders[operand])
	            {
	                add_dep(readerID, consID, WAR, operand);
	            }
	            for(auto & readerID : LastDs[operand])
	            {
	                add_dep(readerID, consID, WAD, operand);
	            }
	        }

            // useless because there is nothing after t but destruction
	        for( auto operand : operands )
	        {
	            DOUT(".. Sink operand, clearing: " << operand);
	            LastWriter[operand] = consID;
	            LastReaders[operand].clear();
	            LastDs[operand].clear();
	        }
        }

        // useless as well because by construction, there cannot be cycles
        // but when afterwards dependences are added, cycles may be created,
        // and after doing so (a copy of) this test should certainly be done because
        // a cyclic dependence graph cannot be scheduled;
        // this test here is a kind of debugging aid whether dependence creation was done well
        if( !dag(graph) )
        {
            DOUT("The dependence graph is not a DAG.");
            EOUT("The dependence graph is not a DAG.");
        }
        DOUT("Dependence graph creation Done.");
    }

    void print()
    {
        COUT("Printing Dependence Graph ");
        digraphWriter(graph).
        nodeMap("name", name).
        arcMap("cause", cause).
        arcMap("weight", weight).
        // arcMap("depType", depType).
        node("source", s).
        node("target", t).
        run();
    }

    void write_dependence_matrix()
    {
        COUT("Printing Dependence Matrix ...");
        ofstream fout;
        string datfname( ql::options::get("output_dir") + "/dependenceMatrix.dat");
        fout.open( datfname, ios::binary);
        if ( fout.fail() )
        {
            EOUT("opening file " << datfname << std::endl
                     << "Make sure the output directory ("<< ql::options::get("output_dir") << ") exists");
            return;
        }

        size_t totalInstructions = countNodes(graph);
        vector< vector<bool> > Matrix(totalInstructions, vector<bool>(totalInstructions));

        // now print the edges
        for (ListDigraph::ArcIt arc(graph); arc != INVALID; ++arc)
        {
            ListDigraph::Node srcNode = graph.source(arc);
            ListDigraph::Node dstNode = graph.target(arc);
            size_t srcID = graph.id( srcNode );
            size_t dstID = graph.id( dstNode );
            Matrix[srcID][dstID] = true;
        }

        for(size_t i=1; i<totalInstructions-1;i++)
        {
            for(size_t j=1; j<totalInstructions-1;j++)
            {
                fout << Matrix[j][i] << "\t";
            }
            fout << endl;
        }

        fout.close();
    }


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
    // without RC, this is all there is to schedule, apart from forming the bundles in ql::ir::bundler()
    // set_cycle iterates over the circuit's gates and set_cycle_gate over the dependences of each gate
    // please note that set_cycle_gate expects a caller like set_cycle which iterates gp forward through the circuit
    void set_cycle_gate(ql::gate* gp, ql::scheduling_direction_t dir)
    {
        ListDigraph::Node   currNode = node[gp];
        size_t  currCycle;
        if (ql::forward_scheduling == dir)
        {
            currCycle = 0;
            for( ListDigraph::InArcIt arc(graph,currNode); arc != INVALID; ++arc )
            {
                currCycle = std::max(currCycle, instruction[graph.source(arc)]->cycle + weight[arc]);
            }
        }
        else
        {
            currCycle = MAX_CYCLE;
            for( ListDigraph::OutArcIt arc(graph,currNode); arc != INVALID; ++arc )
            {
                currCycle = std::min(currCycle, instruction[graph.target(arc)]->cycle - weight[arc]);
            }
        }
        gp->cycle = currCycle;
    }

    void set_cycle(ql::scheduling_direction_t dir)
    {
        if (ql::forward_scheduling == dir)
        {
            instruction[s]->cycle = 0;
            DOUT("... set_cycle of " << instruction[s]->qasm() << " cycles " << instruction[s]->cycle);
            // *circp is by definition in a topological order of the dependence graph
            for ( ql::circuit::iterator gpit = circp->begin(); gpit != circp->end(); gpit++)
            {
                set_cycle_gate(*gpit, dir);
                DOUT("... set_cycle of " << (*gpit)->qasm() << " cycles " << (*gpit)->cycle);
            }
            set_cycle_gate(instruction[t], dir);
            DOUT("... set_cycle of " << instruction[t]->qasm() << " cycles " << instruction[t]->cycle);
        }
        else
        {
            instruction[t]->cycle = ALAP_SINK_CYCLE;
            // *circp is by definition in a topological order of the dependence graph
            for ( ql::circuit::reverse_iterator gpit = circp->rbegin(); gpit != circp->rend(); gpit++)
            {
                set_cycle_gate(*gpit, dir);
            }
            set_cycle_gate(instruction[s], dir);

            // readjust cycle values of gates so that SOURCE is at 0
            size_t  SOURCECycle = instruction[s]->cycle;
            DOUT("... readjusting cycle values by -" << SOURCECycle);

            instruction[t]->cycle -= SOURCECycle;
            DOUT("... set_cycle of " << instruction[t]->qasm() << " cycles " << instruction[t]->cycle);
            for ( auto & gp : *circp)
            {
                gp->cycle -= SOURCECycle;
                DOUT("... set_cycle of " << gp->qasm() << " cycles " << gp->cycle);
            }
            instruction[s]->cycle -= SOURCECycle;   // i.e. becomes 0
            DOUT("... set_cycle of " << instruction[s]->qasm() << " cycles " << instruction[s]->cycle);
        }
    }

    static bool cycle_lessthan(ql::gate* gp1, ql::gate* gp2)
    {
        return gp1->cycle < gp2->cycle;
    }

    // sort circuit by the gates' cycle attribute in non-decreasing order
    void sort_by_cycle()
    {
        DOUT("... before sorting on cycle value");
        // for ( ql::circuit::iterator gpit = circp->begin(); gpit != circp->end(); gpit++)
        // {
        //     ql::gate*           gp = *gpit;
        //     DOUT("...... (@" << gp->cycle << ") " << gp->qasm());
        // }

        // std::sort doesn't preserve the original order of elements that have equal values but std::stable_sort does
        std::stable_sort(circp->begin(), circp->end(), cycle_lessthan);

        DOUT("... after sorting on cycle value");
        // for ( ql::circuit::iterator gpit = circp->begin(); gpit != circp->end(); gpit++)
        // {
        //     ql::gate*           gp = *gpit;
        //     DOUT("...... (@" << gp->cycle << ") " << gp->qasm());
        // }
    }

    // ASAP scheduler without RC, updating circuit and returning bundles
    ql::ir::bundles_t schedule_asap(std::string & sched_dot)
    {
        DOUT("Scheduling ASAP ...");
        set_cycle(ql::forward_scheduling);
        sort_by_cycle();

        if (ql::options::get("print_dot_graphs") == "yes")
        {
            stringstream ssdot;
            get_dot(false, true, ssdot);
            sched_dot = ssdot.str();
        }

        DOUT("Scheduling ASAP [DONE]");
        return ql::ir::bundler(*circp, cycle_time);
    }

    // ALAP scheduler without RC, updating circuit and returning bundles
    ql::ir::bundles_t schedule_alap(std::string & sched_dot)
    {
        DOUT("Scheduling ALAP ...");
        set_cycle(ql::backward_scheduling);
        sort_by_cycle();

        if (ql::options::get("print_dot_graphs") == "yes")
        {
            stringstream ssdot;
            get_dot(false, true, ssdot);
            sched_dot = ssdot.str();
        }

        DOUT("Scheduling ALAP [DONE]");
        return ql::ir::bundler(*circp, cycle_time);
    }


// =========== schedulers with RC, latency compensation and buffer-buffer delay insertion
    // Most code from here on deals with scheduling with Resource Constraints.
    // Then the cycles as computed from the depgraph alone start to drift because of resource conflicts,
    // and then it is more optimal to at each point consider all available nodes for scheduling
    // to avoid largely suboptimal results (issue 179), i.e. apply list scheduling.

    // latency compensation
    void latency_compensation(ql::circuit* circp, const ql::quantum_platform& platform)
    {
        DOUT("Latency compensation ...");
        bool    compensated_one = false;
        for ( auto & gp : *circp)
        {
            auto & id = gp->name;
            // DOUT("Latency compensating instruction: " << id);
            long latency_cycles=0;

            if(platform.instruction_settings.count(id) > 0)
            {
                if(platform.instruction_settings[id].count("latency") > 0)
                {
                    float latency_ns = platform.instruction_settings[id]["latency"];
                    latency_cycles = long(std::ceil( static_cast<float>(std::abs(latency_ns)) / cycle_time)) *
                                          ql::utils::sign_of(latency_ns);
                    compensated_one = true;

                    gp->cycle = gp->cycle + latency_cycles;
                    DOUT( "... compensated to @" << gp->cycle << " <- " << id << " with " << latency_cycles );
                }
            }
        }

        if (compensated_one)
        {
            DOUT("... sorting on cycle value after latency compensation");
            sort_by_cycle();

            DOUT("... printing schedule after latency compensation");
            for ( auto & gp : *circp)
            {
                DOUT("...... @(" << gp->cycle << "): " << gp->qasm());
            }
        }
        else
        {
            DOUT("... no gate latency compensated");
        }
        DOUT("Latency compensation [DONE]");
    }

    // insert buffer - buffer delays
    void insert_buffer_delays(ql::ir::bundles_t& bundles, const ql::quantum_platform& platform)
    {
        DOUT("Buffer-buffer delay insertion ... ");
        std::vector<std::string> operations_prev_bundle;
        size_t buffer_cycles_accum = 0;
        for(ql::ir::bundle_t & abundle : bundles)
        {
            std::vector<std::string> operations_curr_bundle;
            for( auto secIt = abundle.parallel_sections.begin(); secIt != abundle.parallel_sections.end(); ++secIt )
            {
                for(auto insIt = secIt->begin(); insIt != secIt->end(); ++insIt )
                {
                    auto & id = (*insIt)->name;
                    std::string op_type("none");
                    if(platform.instruction_settings.count(id) > 0)
                    {
                        if(platform.instruction_settings[id].count("type") > 0)
                        {
                            op_type = platform.instruction_settings[id]["type"].get<std::string>();
                        }
                    }
                    operations_curr_bundle.push_back(op_type);
                }
            }

            size_t buffer_cycles = 0;
            for(auto & op_prev : operations_prev_bundle)
            {
                for(auto & op_curr : operations_curr_bundle)
                {
                    auto temp_buf_cycles = buffer_cycles_map[ std::pair<std::string,std::string>(op_prev, op_curr) ];
                    DOUT("... considering buffer_" << op_prev << "_" << op_curr << ": " << temp_buf_cycles);
                    buffer_cycles = std::max(temp_buf_cycles, buffer_cycles);
                }
            }
            DOUT( "... inserting buffer : " << buffer_cycles);
            buffer_cycles_accum += buffer_cycles;
            abundle.start_cycle = abundle.start_cycle + buffer_cycles_accum;
            operations_prev_bundle = operations_curr_bundle;
        }
        DOUT("Buffer-buffer delay insertion [DONE] ");
    }

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
    void set_remaining_gate(ql::gate* gp, ql::scheduling_direction_t dir)
    {
        ListDigraph::Node   currNode = node[gp];
        size_t              currRemain = 0;
        if (ql::forward_scheduling == dir)
        {
            for( ListDigraph::OutArcIt arc(graph,currNode); arc != INVALID; ++arc )
            {
                currRemain = std::max(currRemain, remaining[graph.target(arc)] + weight[arc]);
            }
        }
        else
        {
            for( ListDigraph::InArcIt arc(graph,currNode); arc != INVALID; ++arc )
            {
                currRemain = std::max(currRemain, remaining[graph.source(arc)] + weight[arc]);
            }
        }
        remaining[currNode] = currRemain;
    }

    void set_remaining(ql::scheduling_direction_t dir)
    {
        ql::gate*   gp;
        remaining.clear();
        if (ql::forward_scheduling == dir)
        {
            // remaining until SINK (i.e. the SINK.cycle-ALAP value)
            remaining[t] = 0;
            // *circp is by definition in a topological order of the dependence graph
            for ( ql::circuit::reverse_iterator gpit = circp->rbegin(); gpit != circp->rend(); gpit++)
            {
                ql::gate*   gp2 = *gpit;
                set_remaining_gate(gp2, dir);
                DOUT("... remaining at " << gp2->qasm() << " cycles " << remaining[node[gp2]]);
            }
            gp = instruction[s];
            set_remaining_gate(gp, dir);
            DOUT("... remaining at " << gp->qasm() << " cycles " << remaining[s]);
        }
        else
        {
            // remaining until SOURCE (i.e. the ASAP value)
            remaining[s] = 0;
            // *circp is by definition in a topological order of the dependence graph
            for ( ql::circuit::iterator gpit = circp->begin(); gpit != circp->end(); gpit++)
            {
                ql::gate*   gp2 = *gpit;
                set_remaining_gate(gp2, dir);
                DOUT("... remaining at " << gp2->qasm() << " cycles " << remaining[node[gp2]]);
            }
            gp = instruction[t];
            set_remaining_gate(gp, dir);
            DOUT("... remaining at " << gp->qasm() << " cycles " << remaining[t]);
        }
    }

    ql::gate* find_mostcritical(std::list<ql::gate*>& lg)
    {
        size_t      maxRemain = 0;
        ql::gate*   mostCriticalGate = NULL;
        for ( auto gp : lg)
        {
            size_t gr = remaining[node[gp]];
            if (gr > maxRemain)
            {
                mostCriticalGate = gp;
                maxRemain = gr;
            }
        }
        DOUT("... most critical gate: " << mostCriticalGate->qasm() << " with remaining=" << maxRemain);
        return mostCriticalGate;
    }

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
    void init_available(std::list<ListDigraph::Node>& avlist, ql::scheduling_direction_t dir, size_t& curr_cycle)
    {
        avlist.clear();
        if (ql::forward_scheduling == dir)
        {
            curr_cycle = 0;
            instruction[s]->cycle = curr_cycle;
            avlist.push_back(s);
        }
        else
        {
            curr_cycle = ALAP_SINK_CYCLE;
            instruction[t]->cycle = curr_cycle;
            avlist.push_back(t);
        }
    }

    // collect the list of directly depending nodes
    // (i.e. those necessarily scheduled after the given node) without duplicates;
    // dependences that are duplicates from the perspective of the scheduler
    // may be present in the dependence graph because the scheduler ignores dependence type and cause
    void get_depending_nodes(ListDigraph::Node n, ql::scheduling_direction_t dir, std::list<ListDigraph::Node> & ln)
    {
        if (ql::forward_scheduling == dir)
        {
            for (ListDigraph::OutArcIt succArc(graph,n); succArc != INVALID; ++succArc)
            {
                ListDigraph::Node succNode = graph.target(succArc);
                // DOUT("...... succ of " << instruction[n]->qasm() << " : " << instruction[succNode]->qasm());
                bool found = false;             // filter out duplicates
                for ( auto anySuccNode : ln )
                {
                    if (succNode == anySuccNode)
                    {
                        // DOUT("...... duplicate: " << instruction[succNode]->qasm());
                        found = true;           // duplicate found
                    }
                }
                if (found == false)             // found new one
                {
                    ln.push_back(succNode);     // new node to ln
                }
            }
            // ln contains depending nodes of n without duplicates
        }
        else
        {
            for (ListDigraph::InArcIt predArc(graph,n); predArc != INVALID; ++predArc)
            {
                ListDigraph::Node predNode = graph.source(predArc);
                // DOUT("...... pred of " << instruction[n]->qasm() << " : " << instruction[predNode]->qasm());
                bool found = false;             // filter out duplicates
                for ( auto anyPredNode : ln )
                {
                    if (predNode == anyPredNode)
                    {
                        // DOUT("...... duplicate: " << instruction[predNode]->qasm());
                        found = true;           // duplicate found
                    }
                }
                if (found == false)             // found new one
                {
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
    bool criticality_lessthan(ListDigraph::Node n1, ListDigraph::Node n2, ql::scheduling_direction_t dir)
    {
        if (n1 == n2) return false;             // because not <

        if (remaining[n1] < remaining[n2]) return true;
        if (remaining[n1] > remaining[n2]) return false;
        // so: remaining[n1] == remaining[n2]

        std::list<ListDigraph::Node>   ln1;
        std::list<ListDigraph::Node>   ln2;

        get_depending_nodes(n1, dir, ln1);
        get_depending_nodes(n2, dir, ln2);
        if (ln2.empty()) return false;          // strictly < only when ln1.empty and ln2.not_empty
        if (ln1.empty()) return true;           // so when both empty, it is equal, so not strictly <, so false
        // so: ln1.non_empty && ln2.non_empty

        ln1.sort([this](const ListDigraph::Node &d1, const ListDigraph::Node &d2) { return remaining[d1] < remaining[d2]; });
        ln2.sort([this](const ListDigraph::Node &d1, const ListDigraph::Node &d2) { return remaining[d1] < remaining[d2]; });

        size_t crit_dep_n1 = remaining[ln1.back()];    // the last of the list is the one with the largest remaining value
        size_t crit_dep_n2 = remaining[ln2.back()];

        if (crit_dep_n1 < crit_dep_n2) return true;
        if (crit_dep_n1 > crit_dep_n2) return false;
        // so: crit_dep_n1 == crit_dep_n2, call this crit_dep

        ln1.remove_if([this,crit_dep_n1](ListDigraph::Node n) { return remaining[n] < crit_dep_n1; });
        ln2.remove_if([this,crit_dep_n2](ListDigraph::Node n) { return remaining[n] < crit_dep_n2; });
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
    // update its cycle attribute to reflect these dependences;
    // avlist is initialized with s or t as first element by init_available
    // avlist is kept ordered on deep-criticality, non-increasing (i.e. highest deep-criticality first)
    void MakeAvailable(ListDigraph::Node n, std::list<ListDigraph::Node>& avlist, ql::scheduling_direction_t dir)
    {
        bool    already_in_avlist = false;  // check whether n is already in avlist
                                            // originates from having multiple arcs between pair of nodes
        std::list<ListDigraph::Node>::iterator first_lower_criticality_inp;     // for keeping avlist ordered
        bool    first_lower_criticality_found = false;                          // for keeping avlist ordered

        DOUT(".... making available node " << name[n] << " remaining: " << remaining[n]);
        for (std::list<ListDigraph::Node>::iterator inp = avlist.begin(); inp != avlist.end(); inp++)
        {
            if (*inp == n)
            {
                already_in_avlist = true;
                DOUT("...... duplicate when making available: " << name[n]);
            }
            else
            {
                // scanning avlist from front to back (avlist is ordered from high to low criticality)
                // when encountering first node *inp with less criticality,
                // that is where new node n should be inserted just before,
                // to keep avlist in desired order
                //
                // consequence is that
                // when a node has same criticality as n, new node n is put after it, as last one of set of same criticality,
                // so order of calling MakeAvailable (and probably original circuit, and running other scheduler first) matters,
                // also when all dependence sets (and so remaining values) are identical!
                if (criticality_lessthan(*inp, n, dir) && !first_lower_criticality_found)
                {
                    first_lower_criticality_inp = inp;
                    first_lower_criticality_found = true;
                }
            }
        }
        if (!already_in_avlist)
        {
            set_cycle_gate(instruction[n], dir);        // for the schedulers to inspect whether gate has completed
            if (first_lower_criticality_found)
            {
                // add n to avlist just before the first with lower criticality
                avlist.insert(first_lower_criticality_inp, n);
            }
            else
            {
                // add n to end of avlist, if none found with less criticality
                avlist.push_back(n);
            }
            DOUT("...... made available node(@" << instruction[n]->cycle << "): " << name[n] << " remaining: " << remaining[n]);
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
    void TakeAvailable(ListDigraph::Node n, std::list<ListDigraph::Node>& avlist, std::map<ql::gate*,bool> & scheduled, ql::scheduling_direction_t dir)
    {
        scheduled[instruction[n]] = true;
        avlist.remove(n);

        if (ql::forward_scheduling == dir)
        {
            for (ListDigraph::OutArcIt succArc(graph,n); succArc != INVALID; ++succArc)
            {
                ListDigraph::Node succNode = graph.target(succArc);
                bool schedulable = true;
                for (ListDigraph::InArcIt predArc(graph,succNode); predArc != INVALID; ++predArc)
                {
                    ListDigraph::Node predNode = graph.source(predArc);
                    if (!scheduled[instruction[predNode]])
                    {
                        schedulable = false;
                        break;
                    }
                }
                if (schedulable)
                {
                    MakeAvailable(succNode, avlist, dir);
                }
            }
        }
        else
        {
            for (ListDigraph::InArcIt predArc(graph,n); predArc != INVALID; ++predArc)
            {
                ListDigraph::Node predNode = graph.source(predArc);
                bool schedulable = true;
                for (ListDigraph::OutArcIt succArc(graph,predNode); succArc != INVALID; ++succArc)
                {
                    ListDigraph::Node succNode = graph.target(succArc);
                    if (!scheduled[instruction[succNode]])
                    {
                        schedulable = false;
                        break;
                    }
                }
                if (schedulable)
                {
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
    void AdvanceCurrCycle(ql::scheduling_direction_t dir, size_t& curr_cycle)
    {
        if (ql::forward_scheduling == dir)
        {
            curr_cycle++;
        }
        else
        {
            curr_cycle--;
        }
    }

    // a gate must wait until all its operand are available, i.e. the gates having computed them have completed,
    // and must wait until all resources required for the gate's execution are available;
    // return true when immediately schedulable
    // when returning false, isres indicates whether resource occupation was the reason or operand completion (for debugging)
    bool immediately_schedulable(ListDigraph::Node n, ql::scheduling_direction_t dir, const size_t curr_cycle,
                                const ql::quantum_platform& platform, ql::arch::resource_manager_t& rm, bool& isres)
    {
        ql::gate*   gp = instruction[n];
        isres = true;
        // have dependent gates completed at curr_cycle?
        if (    ( ql::forward_scheduling == dir && gp->cycle <= curr_cycle)
            ||  ( ql::backward_scheduling == dir && curr_cycle <= gp->cycle)
            )
        {
            // are resources available?
            if ( n == s || n == t
                || gp->type() == ql::gate_type_t::__dummy_gate__
                || gp->type() == ql::gate_type_t::__classical_gate__
                || gp->type() == ql::gate_type_t::__wait_gate__
               )
            {
                return true;
            }
            if (rm.available(curr_cycle, gp, platform))
            {
                return true;
            }
            isres = true;;
            return false;
        }
        else
        {
            isres = false;
            return false;
        }
    }

    // select a node from the avlist
    // the avlist is deep-ordered from high to low criticality (see criticality_lessthan above)
    ListDigraph::Node SelectAvailable(std::list<ListDigraph::Node>& avlist, ql::scheduling_direction_t dir, const size_t curr_cycle,
                                const ql::quantum_platform& platform, ql::arch::resource_manager_t& rm, bool & success)
    {
        success = false;                        // whether a node was found and returned

        DOUT("avlist(@" << curr_cycle << "):");
        for ( auto n : avlist)
        {
            DOUT("...... node(@" << instruction[n]->cycle << "): " << name[n] << " remaining: " << remaining[n]);
        }

        // select the first immediately schedulable, if any
        // since avlist is deep-criticality ordered, highest first, the first is the most deep-critical
        for ( auto n : avlist)
        {
            bool isres;
            if ( immediately_schedulable(n, dir, curr_cycle, platform, rm, isres) )
            {
                DOUT("... node (@" << instruction[n]->cycle << "): " << name[n] << " immediately schedulable, remaining=" << remaining[n] << ", selected");
                success = true;
                return n;
            }
            else
            {
                DOUT("... node (@" << instruction[n]->cycle << "): " << name[n] << " remaining=" << remaining[n] << ", waiting for " << (isres? "resource" : "dependent completion"));
            }
        }

        success = false;
        return s;   // fake return value
    }

    // ASAP/ALAP scheduler with RC
    //
    // schedule the circuit that is in the dependence graph
    // for the given direction, with the given platform and resource manager;
    // what is done, is:
    // - the cycle attribute of the gates will be set according to the scheduling method
    // - *circp (the original and result circuit) is sorted in the new cycle order
    // - bundles are collected from the circuit
    // - latency compensation and buffer-buffer delay insertion done
    // the bundles are returned, with private start/duration attributes
    ql::ir::bundles_t schedule(ql::circuit* circp, ql::scheduling_direction_t dir,
            const ql::quantum_platform& platform, ql::arch::resource_manager_t& rm, std::string& sched_dot)
    {
        DOUT("Scheduling " << (ql::forward_scheduling == dir?"ASAP":"ALAP") << " with RC ...");

        // scheduled[gp] :=: whether gate *gp has been scheduled, init all false
        std::map<ql::gate*,bool>    scheduled;
        // avlist :=: list of schedulable nodes, initially (see below) just s or t
        std::list<ListDigraph::Node>    avlist;

        // initializations for this scheduler
        // note that dependence graph is not modified by a scheduler, so it can be reused
        DOUT("... initialization");
        for (ListDigraph::NodeIt n(graph); n != INVALID; ++n)
        {
            scheduled[instruction[n]] = false;   // none were scheduled, including SOURCE/SINK
        }
        size_t  curr_cycle;         // current cycle for which instructions are sought
        init_available(avlist, dir, curr_cycle);     // first node (SOURCE/SINK) is made available and curr_cycle set
        set_remaining(dir);         // for each gate, number of cycles until end of schedule

        DOUT("... loop over avlist until it is empty");
        while (!avlist.empty())
        {
            bool success;
            ListDigraph::Node   selected_node;

            selected_node = SelectAvailable(avlist, dir, curr_cycle, platform, rm, success);
            if (!success)
            {
                // i.e. none from avlist was found suitable to schedule in this cycle
                AdvanceCurrCycle(dir, curr_cycle);
                // so try again; eventually instrs complete and machine is empty
                continue;
            }

            // commit selected_node to the schedule
            ql::gate* gp = instruction[selected_node];
            DOUT("... selected " << gp->qasm() << " in cycle " << curr_cycle);
            gp->cycle = curr_cycle;                     // scheduler result, including s and t
            if (selected_node != s
                && selected_node != t
                && gp->type() != ql::gate_type_t::__dummy_gate__
                && gp->type() != ql::gate_type_t::__classical_gate__
                && gp->type() != ql::gate_type_t::__wait_gate__
               )
            {
                rm.reserve(curr_cycle, gp, platform);
            }
            TakeAvailable(selected_node, avlist, scheduled, dir);   // update avlist/scheduled/cycle
            // more nodes that could be scheduled in this cycle, will be found in an other round of the loop
        }

        DOUT("... sorting on cycle value");
        sort_by_cycle();

        if (ql::backward_scheduling == dir)
        {
            // readjust cycle values of gates so that SOURCE is at 0
            size_t  SOURCECycle = instruction[s]->cycle;
            DOUT("... readjusting cycle values by -" << SOURCECycle);

            instruction[t]->cycle -= SOURCECycle;
            for ( auto & gp : *circp)
            {
                gp->cycle -= SOURCECycle;
            }
            instruction[s]->cycle -= SOURCECycle;   // i.e. becomes 0
        }

        if (ql::options::get("print_dot_graphs") == "yes")
        {
            stringstream ssdot;
            get_dot(false, true, ssdot);
            sched_dot = ssdot.str();
        }

        // end scheduling
        // from here split off

        latency_compensation(circp, platform);

        ql::ir::bundles_t   bundles;
        bundles = ql::ir::bundler(*circp, cycle_time);

        insert_buffer_delays(bundles, platform);

        DOUT("Scheduling " << (ql::forward_scheduling == dir?"ASAP":"ALAP") << " with RC [DONE]");
        return bundles;
    }

    ql::ir::bundles_t schedule_asap(ql::arch::resource_manager_t & rm, const ql::quantum_platform & platform, std::string& sched_dot)
    {
        ql::ir::bundles_t   bundles;
        DOUT("Scheduling ASAP");
        bundles = schedule(circp, ql::forward_scheduling, platform, rm, sched_dot);

        DOUT("Scheduling ASAP [DONE]");
        return bundles;
    }

    ql::ir::bundles_t schedule_alap(ql::arch::resource_manager_t & rm, const ql::quantum_platform & platform, std::string& sched_dot)
    {
        ql::ir::bundles_t   bundles;
        DOUT("Scheduling ALAP");
        bundles = schedule(circp, ql::backward_scheduling, platform, rm, sched_dot);

        DOUT("Scheduling ALAP [DONE]");
        return bundles;
    }

// =========== uniform
    ql::ir::bundles_t schedule_alap_uniform()
    {
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
        // and such that the circuit's depth is not enlarged and the dependences/latencies are obeyed.
        // Hence, the result resembles an ALAP schedule with excess bundle lengths solved by moving nodes down ("rolling pin").

        DOUT("Scheduling ALAP UNIFORM to get bundles ...");
        ql::ir::bundles_t bundles;

        // initialize gp->cycle as ASAP cycles as first approximation of result;
        // note that the circuit doesn't contain the SOURCE and SINK gates but the dependence graph does;
        // from SOURCE is a weight 1 dep to the first nodes using each qubit and classical register, and to the SINK gate
        // is a dep from each unused qubit/classical register result with as weight the duration of the last operation.
        // SOURCE (node s) is at cycle 0 and the first circuit's gates are at cycle 1.
        // SINK (node t) is at the earliest cycle that all gates/operations have completed.
        set_cycle(ql::forward_scheduling);
        size_t   cycle_count = instruction[t]->cycle - 1;
        // so SOURCE at cycle 0, then all circuit's gates at cycles 1 to cycle_count, and finally SINK at cycle cycle_count+1

        // compute remaining which is the opposite of the alap cycle value (remaining[node] :=: SINK->cycle - alapcycle[node])
        // remaining[node] indicates number of cycles remaining in schedule from node's execution start to SINK,
        // and indicates the latest cycle that the node can be scheduled so that the circuit's depth is not increased.
        set_remaining(ql::forward_scheduling);

        // DOUT("Creating gates_per_cycle");
        // create gates_per_cycle[cycle] = for each cycle the list of gates at cycle cycle
        // this is the basic map to be operated upon by the uniforming scheduler below;
        std::map<size_t,std::list<ql::gate*>> gates_per_cycle;
        for ( ql::circuit::iterator gpit = circp->begin(); gpit != circp->end(); gpit++)
        {
            ql::gate*           gp = *gpit;
            gates_per_cycle[gp->cycle].push_back(gp);
        }

        // DOUT("Displaying circuit and bundle statistics");
        // to compute how well the algorithm is doing, two measures are computed:
        // - the largest number of gates in a cycle in the circuit,
        // - and the average number of gates in non-empty cycles
        // this is done before and after uniform scheduling, and printed
        size_t max_gates_per_cycle = 0;
        size_t non_empty_bundle_count = 0;
        size_t gate_count = 0;
        for (size_t curr_cycle = 1; curr_cycle <= cycle_count; curr_cycle++)
        {
            max_gates_per_cycle = std::max(max_gates_per_cycle, gates_per_cycle[curr_cycle].size());
            if (int(gates_per_cycle[curr_cycle].size()) != 0) non_empty_bundle_count++;
            gate_count += gates_per_cycle[curr_cycle].size();
        }
        double avg_gates_per_cycle = double(gate_count)/cycle_count;
        double avg_gates_per_non_empty_cycle = double(gate_count)/non_empty_bundle_count;
        DOUT("... before uniform scheduling:"
            << " cycle_count=" << cycle_count
            << "; gate_count=" << gate_count
            << "; non_empty_bundle_count=" << non_empty_bundle_count
            );
        DOUT("... and max_gates_per_cycle=" << max_gates_per_cycle
            << "; avg_gates_per_cycle=" << avg_gates_per_cycle
            << "; avg_gates_per_non_empty_cycle=" << avg_gates_per_non_empty_cycle
            );

        // in a backward scan, make non-empty bundles max avg_gates_per_non_empty_cycle long;
        // an earlier version of the algorithm aimed at making bundles max avg_gates_per_cycle long
        // but that flawed because of frequent empty bundles causing this estimate for a uniform length being too low
        // DOUT("Backward scan uniform scheduling");
        for (size_t curr_cycle = cycle_count; curr_cycle >= 1; curr_cycle--)
        {
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
            // it is readjusted during the scan to cater for dips in bundle size caused by local dependence chains
            if (non_empty_bundle_count == 0) break;     // nothing to do
            avg_gates_per_cycle = double(gate_count)/curr_cycle;
            avg_gates_per_non_empty_cycle = double(gate_count)/non_empty_bundle_count;
            DOUT("Cycle=" << curr_cycle << " number of gates=" << gates_per_cycle[curr_cycle].size()
                << "; avg_gates_per_cycle=" << avg_gates_per_cycle
                << "; avg_gates_per_non_empty_cycle=" << avg_gates_per_non_empty_cycle);

            while ( double(gates_per_cycle[curr_cycle].size()) < avg_gates_per_non_empty_cycle && pred_cycle >= 1 )
            {
                DOUT("pred_cycle=" << pred_cycle);
                DOUT("gates_per_cycle[curr_cycle].size()=" << gates_per_cycle[curr_cycle].size());
                size_t      min_remaining_cycle = MAX_CYCLE;
                ql::gate*   best_predgp;
                bool        best_predgp_found = false;

                // scan bundle at pred_cycle to find suitable candidate to move forward to curr_cycle
                for ( auto predgp : gates_per_cycle[pred_cycle] )
                {
                    bool    forward_predgp = true;
                    size_t  predgp_completion_cycle;
                    ListDigraph::Node   pred_node = node[predgp];
                    DOUT("... considering: " << predgp->qasm() << " @cycle=" << predgp->cycle << " remaining=" << remaining[pred_node]);

                    // candidate's result, when moved, must be ready before end-of-circuit and before used
                    predgp_completion_cycle = curr_cycle + size_t(std::ceil(static_cast<float>(predgp->duration)/cycle_time));
                    // predgp_completion_cycle = curr_cycle + (predgp->duration+cycle_time-1)/cycle_time;
                    if (predgp_completion_cycle > cycle_count + 1)  // at SINK is ok, later not
                    {
                        forward_predgp = false;
                        DOUT("... ... rejected (after circuit): " << predgp->qasm() << " would complete @" << predgp_completion_cycle << " SINK @" << cycle_count+1);
                    }
                    else
                    {
                        for ( ListDigraph::OutArcIt arc(graph,pred_node); arc != INVALID; ++arc )
                        {
                            ql::gate*   target_gp = instruction[graph.target(arc)];
                            size_t target_cycle = target_gp->cycle;
                            if(predgp_completion_cycle > target_cycle)
                            {
                                forward_predgp = false;
                                DOUT("... ... rejected (after succ): " << predgp->qasm() << " would complete @" << predgp_completion_cycle << " target=" << target_gp->qasm() << " target_cycle=" << target_cycle);
                            }
                        }
                    }

                    // when multiple nodes in bundle qualify, take the one with lowest remaining
                    // because that is the most critical one and thus deserves a cycle as high as possible (ALAP)
                    if (forward_predgp && remaining[pred_node] < min_remaining_cycle)
                    {
                        min_remaining_cycle = remaining[pred_node];
                        best_predgp_found = true;
                        best_predgp = predgp;
                    }
                }

                // when candidate was found in this bundle, move it, and search for more in this bundle, if needed
                // otherwise, continue scanning backward
                if (best_predgp_found)
                {
                    // move predgp from pred_cycle to curr_cycle;
                    // adjust all bookkeeping that is affected by this
                    gates_per_cycle[pred_cycle].remove(best_predgp);
                    if (gates_per_cycle[pred_cycle].size() == 0)
                    {
                        // source bundle was non-empty, now it is empty
                        non_empty_bundle_count--;
                    }
                    if (gates_per_cycle[curr_cycle].size() == 0)
                    {
                        // target bundle was empty, now it will be non_empty
                        non_empty_bundle_count++;
                    }
                    best_predgp->cycle = curr_cycle;        // what it is all about
                    gates_per_cycle[curr_cycle].push_back(best_predgp);

                    // recompute targets
                    if (non_empty_bundle_count == 0) break;     // nothing to do
                    avg_gates_per_cycle = double(gate_count)/curr_cycle;
                    avg_gates_per_non_empty_cycle = double(gate_count)/non_empty_bundle_count;
                    DOUT("... moved " << best_predgp->qasm() << " with remaining=" << remaining[node[best_predgp]]
                        << " from cycle=" << pred_cycle << " to cycle=" << curr_cycle
                        << "; new avg_gates_per_cycle=" << avg_gates_per_cycle
                        << "; avg_gates_per_non_empty_cycle=" << avg_gates_per_non_empty_cycle
                        );
                }
                else
                {
                    pred_cycle --;
                }
            }   // end for finding a bundle to forward a node from to the current cycle

            // curr_cycle ready, recompute counts for remaining cycles
            // mask current cycle and its gates from the target counts:
            // - gate_count, non_empty_bundle_count, curr_cycle (as cycles still to go)
            gate_count -= gates_per_cycle[curr_cycle].size();
            if (gates_per_cycle[curr_cycle].size() != 0)
            {
                // bundle is non-empty
                non_empty_bundle_count--;
            }
        }   // end curr_cycle loop; curr_cycle is bundle which must be enlarged when too small

        // new cycle values computed; reflect this in circuit's gate order
        sort_by_cycle();

        // recompute and print statistics reporting on uniform scheduling performance
        max_gates_per_cycle = 0;
        non_empty_bundle_count = 0;
        gate_count = 0;
        // cycle_count was not changed
        for (size_t curr_cycle = 1; curr_cycle <= cycle_count; curr_cycle++)
        {
            max_gates_per_cycle = std::max(max_gates_per_cycle, gates_per_cycle[curr_cycle].size());
            if (int(gates_per_cycle[curr_cycle].size()) != 0) non_empty_bundle_count++;
            gate_count += gates_per_cycle[curr_cycle].size();
        }
        avg_gates_per_cycle = double(gate_count)/cycle_count;
        avg_gates_per_non_empty_cycle = double(gate_count)/non_empty_bundle_count;
        DOUT("... after uniform scheduling:"
            << " cycle_count=" << cycle_count
            << "; gate_count=" << gate_count
            << "; non_empty_bundle_count=" << non_empty_bundle_count
            );
        DOUT("... and max_gates_per_cycle=" << max_gates_per_cycle
            << "; avg_gates_per_cycle=" << avg_gates_per_cycle
            << "; ..._per_non_empty_cycle=" << avg_gates_per_non_empty_cycle
            );

        // prefer standard bundler over using the gates_per_cycle data structure
        bundles = ql::ir::bundler(*circp, cycle_time);

        DOUT("Scheduling ALAP UNIFORM to get bundles [DONE]");
        return bundles;
    }

// =========== printing dot of the dependence graph
    void get_dot(
                bool WithCritical,
                bool WithCycles,
                std::ostream& dotout
                )
    {
        DOUT("Get_dot");
        Path<ListDigraph> p;
        ListDigraph::ArcMap<bool> isInCritical(graph);
        if(WithCritical)
        {
            for (ListDigraph::ArcIt a(graph); a != INVALID; ++a)
            {
                isInCritical[a] = false;
                for ( Path<ListDigraph>::ArcIt ap(p); ap != INVALID; ++ap )
                {
                    if(a==ap)
                    {
                        isInCritical[a] = true;
                        break;
                    }
                }
            }
        }

        string NodeStyle(" fontcolor=black, style=filled, fontsize=16");
        string EdgeStyle1(" color=black");
        string EdgeStyle2(" color=red");
        string EdgeStyle = EdgeStyle1;

        dotout << "digraph {\ngraph [ rankdir=TD; ]; // or rankdir=LR"
            << "\nedge [fontsize=16, arrowhead=vee, arrowsize=0.5];"
            << endl;

        // first print the nodes
        for (ListDigraph::NodeIt n(graph); n != INVALID; ++n)
        {
            dotout  << "\"" << graph.id(n) << "\""
                    << " [label=\" " << name[n] <<" \""
                    << NodeStyle
                    << "];" << endl;
        }

        if( WithCycles)
        {
            // Print cycle numbers as timeline, as shown below
            size_t TotalCycles;
            if (circp->size() == 0)
            {
                TotalCycles = 1;    // +1 is SOURCE's duration in cycles
            }
            else
            {
                TotalCycles = circp->back()->cycle + (circp->back()->duration+cycle_time-1)/cycle_time
                                    - circp->front()->cycle + 1;    // +1 is SOURCE's duration in cycles
            }
            dotout << "{\nnode [shape=plaintext, fontsize=16, fontcolor=blue]; \n";
            for(size_t cn=0;cn<=TotalCycles;++cn)
            {
                if(cn>0)
                    dotout << " -> ";
                dotout << "Cycle" << cn;
            }
            dotout << ";\n}\n";

            // Now print ranks, as shown below
            dotout << "{ rank=same; Cycle" << instruction[s]->cycle <<"; " << graph.id(s) << "; }\n";
            for (auto gp : *circp)
            {
                dotout << "{ rank=same; Cycle" << gp->cycle <<"; " << graph.id(node[gp]) << "; }\n";
            }
            dotout << "{ rank=same; Cycle" << instruction[t]->cycle <<"; " << graph.id(t) << "; }\n";
        }

        // now print the edges
        for (ListDigraph::ArcIt arc(graph); arc != INVALID; ++arc)
        {
            ListDigraph::Node srcNode = graph.source(arc);
            ListDigraph::Node dstNode = graph.target(arc);
            int srcID = graph.id( srcNode );
            int dstID = graph.id( dstNode );

            if(WithCritical)
                EdgeStyle = ( isInCritical[arc]==true ) ? EdgeStyle2 : EdgeStyle1;

            dotout << dec
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
                << endl;
        }

        dotout << "}" << endl;
        DOUT("Get_dot[DONE]");
    }

    void get_dot(std::string & dot)
    {
        set_cycle(ql::forward_scheduling);
        sort_by_cycle();

        stringstream ssdot;
        get_dot(false, true, ssdot);
        dot = ssdot.str();
    }
};

#endif
