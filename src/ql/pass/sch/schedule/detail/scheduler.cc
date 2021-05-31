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
 * type (RAW, WAW, etc.), cause (the qubit, classical register or bit register used as
 * parameter), and a weight (the cycles the previous gate takes to complete its
 * execution, after which the current gate can start execution).
 *
 * In dependency graph creation, each qubit/classical register/bit register (creg,breg) use
 * in each gate is seen as an "event". The following events are distinguished:
 *
 *  - W for Cwrite/Bwrite:
 *    such a use must sequentialize with all previous and later
 *    uses of the same creg/breg. This is the default in classical code.
 *    Since all Writes sequentialize, one has only to create dependences with the previous and next one.
 *  - R for Cread/Bread:
 *    such uses can be arbitrarily reordered (as long as other dependences allow that),
 *    but sequentialize with the previous and following Write on the same register.
 *    It applies to all reads in classical code (that don't have side effects).
 *    uses of the same creg/breg. This is the default in classical code.
 *  - D for Default:
 *    such a use must sequentialize with all previous and later
 *    uses of the same qubit. This is the default for qubit operands of gates.
 *    Since all Defaults sequentialize, one has only to create dependences with the previous and next one.
 *  - X for Xrotate:
 *    such uses can be arbitrarily reordered (as long as other
 *    dependencies allow that) but are sequentialized with Write and Zrotate events on the same qubit.
 *    This event applies to the second operand of CNOT gates,
 *    and all X rotations: CNOT(a,d), CNOT(b,d), RX(d), all commute.
 *  - Z for Z-rotate:
 *    such uses can be arbitrarily reordered (as long as other
 *    dependencies allow that) but are sequentialized with Write and Xrotate events on the same qubit.
 *    This event applies to all operands of CZ, the first operand of CNOT gates,
 *    and all Z rotations (RZ,Z,Z90(SDAG),ZM90(S)).
 *    It also applies in general to the control operand of Control Unitaries.
 *    It represents commutativity between the gates with such use: CU(a,b),
 *    CZ(a,c), CZ(d,a), CNOT(a,e), RZ(a), S(a), all commute.
 *  With this, we effectively get the following tables of event transitions (from left-bottom to right-up):
 *  for creg and breg operands:
 *           W   R
 *        W  WAW RAW
 *        R  WAR RAR
 *
 *  for qubit operands:
 *          D   X   Z
 *        D DAD XAD ZAD
 *        X DAX XAX ZAX
 *        Z DAZ XAZ ZAZ
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

#include "ql/utils/vec.h"
#include "ql/utils/filesystem.h"

namespace ql {
namespace pass {
namespace sch {
namespace schedule {
namespace detail {

using namespace utils;
using ListDigraph = lemon::ListDigraph;
using ListDigraphPath = lemon::Path<ListDigraph>;

std::ostream &operator<<(std::ostream &os, DepType dt) {
    switch (dt) {
        case DepType::RAR: os << "RAR"; break;
        case DepType::RAW: os << "RAW"; break;
        case DepType::WAR: os << "WAR"; break;
        case DepType::WAW: os << "WAW"; break;
        case DepType::DAD: os << "DAD"; break;
        case DepType::DAX: os << "DAX"; break;
        case DepType::DAZ: os << "DAZ"; break;
        case DepType::XAD: os << "XAD"; break;
        case DepType::XAX: os << "XAX"; break;
        case DepType::XAZ: os << "XAZ"; break;
        case DepType::ZAD: os << "ZAD"; break;
        case DepType::ZAX: os << "ZAX"; break;
        case DepType::ZAZ: os << "ZAZ"; break;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, EventType et) {
    switch (et) {
        case EventType::DEFAULT: os << "Default"; break;
        case EventType::XROTATE: os << "Xrotate"; break;
        case EventType::ZROTATE: os << "Zrotate"; break;
        case EventType::CREAD:   os << "Cread"; break;
        case EventType::CWRITE:  os << "Cwrite"; break;
        case EventType::BREAD:   os << "Bread"; break;
        case EventType::BWRITE:  os << "Bwrite"; break;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, OperandType ot) {
    switch (ot) {
        case OperandType::QUBIT: os << "q"; break;
        case OperandType::CREG:  os << "c"; break;
        case OperandType::BREG:  os << "b"; break;
    }
    return os;
}

Scheduler::Scheduler() :
    instruction(graph),
    name(graph),
    weight(graph),
    op_type(graph),
    cause(graph),
    dep_type(graph)
{
}

// ins->name may contain parameters, so must be stripped first before checking it for gate's name
void Scheduler::strip_name(Str &name) {
    UInt p = name.find(' ');
    if (p != Str::npos) {
        name = name.substr(0,p);
    }
}

// Add a dependency between two nodes: from node fromID to node toID
// the dependence is annotated with the deptype, operandtype and operand for possible transformations and for tracing
void Scheduler::add_dep(
    Int from_id,
    Int to_id,
    DepType dt,
    OperandType ot,
    UInt operand
) {
    QL_DOUT(".. adddep ... from fromID " << from_id << " to toID " << to_id << "   opnd=" << ot << "[" << operand << "], dep=" << dt);
    auto from_node = graph.nodeFromId(from_id);
    auto to_node = graph.nodeFromId(to_id);
    auto arc = graph.addArc(from_node, to_node);
    weight[arc] = Int(ceil(static_cast<Real>(instruction[from_node]->duration) / cycle_time));
    op_type[arc] = ot;
    cause[arc] = operand;
    dep_type[arc] = dt;
    QL_DOUT("... dep " << name[from_node] << " -> " << name[to_node] << " opnd=" << op_type[arc] << "[" << cause[arc] << "], dep=" << dep_type[arc] << ", wght=" << weight[arc] << ")");
}

// Signal a new event to the depgraph constructor:
// the new event is of type currEvent
// and concerns the current gate encoded by currID
// and its given ooperand;
// commutes indicates whether this event is allowed to commute with the other events of its type.
//
// This event drives a state machine to do one step (one state transition).
// It accepts the following event sequence per Qubit operand: Default { Default | Xrotate+ | Zrotate+ }* Default
// and the following event sequence per Creg/Breg operand: Write { Write | Read+ }* Write,
// in which the first Write/Default is the SOURCE and the last Write/Default is the SINK.
// The state machines have as state vectors for the lastevent, and various last states; these are vectors indexed by the operand.
void Scheduler::new_event(
    int curr_id,
    OperandType operand_type,
    UInt operand,
    EventType curr_event,
    bool commutes
) {
    switch (curr_event) {
        case EventType::DEFAULT:
            QL_DOUT(".. " << curr_event << " on: " << operand_type << "[" << operand << "]" << " while in " << last_q_event[operand]);
            if (last_q_event[operand] == EventType::DEFAULT) {
                add_dep(last_default[operand], curr_id, DepType::DAD, OperandType::QUBIT, operand);
            }
            if (last_q_event[operand] == EventType::ZROTATE) {
                for (auto &ZgateID : last_z_rotates[operand]) {
                    add_dep(ZgateID, curr_id, DepType::DAZ, OperandType::QUBIT, operand);
                }
            }
            if (last_q_event[operand] == EventType::XROTATE) {
                for (auto &XgateID : last_x_rotates[operand]) {
                    add_dep(XgateID, curr_id, DepType::DAX, OperandType::QUBIT, operand);
                }
            }
            last_default[operand] = curr_id;
            last_q_event[operand] = curr_event;
            break;
    
        case EventType::ZROTATE:
            QL_DOUT(".. " << curr_event << " on: " << operand_type << "[" << operand << "]" << " while in " << last_q_event[operand]);
            add_dep(last_default[operand], curr_id, DepType::ZAD, OperandType::QUBIT, operand);
            if (last_q_event[operand] != EventType::ZROTATE) {
                last_z_rotates[operand].clear();
            }
            if (last_q_event[operand] == EventType::ZROTATE) {
                if (!commutes) {
                    for (auto &ZgateID : last_z_rotates[operand]) {
                        add_dep(ZgateID, curr_id, DepType::ZAZ, OperandType::QUBIT, operand);
                    }
                }
            }
            for (auto &XgateID : last_x_rotates[operand]) {
                add_dep(XgateID, curr_id, DepType::ZAX, OperandType::QUBIT, operand);
            }
            last_z_rotates[operand].push_back(curr_id);
            last_q_event[operand] = curr_event;
            break;
    
        case EventType::XROTATE:
            QL_DOUT(".. " << curr_event << " on: " << operand_type << "[" << operand << "]" << " while in " << last_q_event[operand]);
            add_dep(last_default[operand], curr_id, DepType::XAD, OperandType::QUBIT, operand);
            if (last_q_event[operand] != EventType::XROTATE) {
                last_x_rotates[operand].clear();
            }
            for (auto &ZgateID : last_z_rotates[operand]) {
                add_dep(ZgateID, curr_id, DepType::XAZ, OperandType::QUBIT, operand);
            }
            if (last_q_event[operand] == EventType::XROTATE) {
                if (!commutes) {
                    for (auto &XgateID : last_x_rotates[operand]) {
                        add_dep(XgateID, curr_id, DepType::XAX, OperandType::QUBIT, operand);
                    }
                }
            }
            last_x_rotates[operand].push_back(curr_id);
            last_q_event[operand] = curr_event;
            break;
    
        case EventType::CWRITE:
            QL_DOUT(".. " << curr_event << " on: " << operand_type << "[" << operand << "]" << " while in " << last_c_event[operand]);
            if (last_c_event[operand] == EventType::CWRITE) {
                add_dep(last_c_writer[operand], curr_id, DepType::WAW, OperandType::BREG, operand);
            }
            if (last_c_event[operand] == EventType::CREAD) {
                for (auto &RgateID : last_c_readers[operand]) {
                    add_dep(RgateID, curr_id, DepType::WAR, OperandType::BREG, operand);
                }
            }
            last_c_writer[operand] = curr_id;
            last_c_event[operand] = curr_event;
            break;
    
        case EventType::CREAD:
            QL_DOUT(".. " << curr_event << " on: " << operand_type << "[" << operand << "]" << " while in " << last_c_event[operand]);
            add_dep(last_c_writer[operand], curr_id, DepType::RAW, OperandType::BREG, operand);
            if (last_c_event[operand] != EventType::CREAD) {
                last_c_readers[operand].clear();
            }
    //      if (LastCEvent[operand] == EventType::CREAD) {
    //          if (!commutes) {
    //              for (auto &RgateID : LastCReaders[operand]) {
    //                  add_dep(RgateID, currID, DepType::RAR, OperandType::BREG, operand);
    //              }
    //          }
    //      }
            last_c_readers[operand].push_back(curr_id);
            last_c_event[operand] = curr_event;
            break;
    
        case EventType::BWRITE:
            QL_DOUT(".. " << curr_event << " on: " << operand_type << "[" << operand << "]" << " while in " << last_b_event[operand]);
            if (last_b_event[operand] == EventType::BWRITE) {
                add_dep(last_b_writer[operand], curr_id, DepType::WAW, OperandType::BREG, operand);
            }
            if (last_b_event[operand] == EventType::BREAD) {
                for (auto &RgateID : last_b_readers[operand]) {
                    add_dep(RgateID, curr_id, DepType::WAR, OperandType::BREG, operand);
                }
            }
            last_b_writer[operand] = curr_id;
            last_b_event[operand] = curr_event;
            break;
    
        case EventType::BREAD:
            QL_DOUT(".. " << curr_event << " on: " << operand_type << "[" << operand << "]" << " while in " << last_b_event[operand]);
            add_dep(last_b_writer[operand], curr_id, DepType::RAW, OperandType::BREG, operand);
            if (last_b_event[operand] != EventType::BREAD) {
                last_b_readers[operand].clear();
            }
    //      if (LastBEvent[operand] == EventType::BREAD) {
    //          if (!commutes) {
    //              for (auto &RgateID : LastBReaders[operand]) {
    //                  add_dep(RgateID, currID, DepType::RAR, OperandType::REG, operand);
    //              }
    //          }
    //      }
            last_b_readers[operand].push_back(curr_id);
            last_b_event[operand] = curr_event;
            break;

    }
}

// construct the dependency graph ('graph') with nodes from the circuit and adding arcs for their dependencies
void Scheduler::init(
    const ir::KernelRef &kernel,
    const utils::Str &output_prefix,
    utils::Bool commute_multi_qubit,
    utils::Bool commute_single_qubit,
    utils::Bool enable_criticality
) {
    QL_DOUT("dependency graph creation ... #qubits = " << kernel->platform->qubit_count);
    qubit_count = kernel->platform->qubit_count;
    creg_count = kernel->platform->creg_count;
    breg_count = kernel->platform->breg_count;
    UInt total_reg_count = qubit_count + creg_count + breg_count;
    QL_DOUT("Scheduler.init: qubit_count=" << qubit_count << ", creg_count=" << creg_count << ", breg_count=" << breg_count << ", total=" << total_reg_count);

    cycle_time = kernel->platform->cycle_time;
    this->kernel = kernel;
    this->output_prefix = output_prefix;
    this->commute_multi_qubit = commute_multi_qubit;
    this->commute_single_qubit = commute_single_qubit;
    this->enable_criticality = enable_criticality;

    // dependencies are created with a current gate as target
    // and with those previous gates as source that have an operand match with the current gate:
    // this dependence creation is done by a state matchine triggered to step on each operand of each gate encountered.
    // operands can be a qubit, a classical register or a bit register
    // the indices in the state vectors are operand indices within the operandType space

    // start filling the dependency graph by creating the s node, the top of the graph
    {
        // add dummy source node
        auto srcNode = graph.addNode();
        instruction[srcNode].emplace<ir::gate_types::Source>();    // so SOURCE is defined as instruction[s], not unique in itself
        node.set(instruction[srcNode]) = srcNode;
        name[srcNode] = instruction[srcNode]->qasm();
        s = srcNode;
    }
    Int src_id = graph.id(s);

    // start the state machines, one for each possible operand
    last_q_event.resize(qubit_count, EventType::DEFAULT);   // start as if SOURCE gate did Default on all qubit operands
    last_default.resize(qubit_count, src_id);
    last_x_rotates.resize(qubit_count);                     // start off as empty list, no Xrotate/Zrotate seen yet
    last_z_rotates.resize(qubit_count);

    last_c_event.resize(creg_count, EventType::CWRITE);     // start as if SOURCE gate did Cwrite on all creg operands
    last_c_writer.resize(creg_count, src_id);
    last_c_readers.resize(creg_count);                      // start off as empty list, no Creader seen yet

    last_b_event.resize(breg_count, EventType::BWRITE);     // start as if SOURCE gate did Bwrite on all breg operands
    last_b_writer.resize(breg_count, src_id);
    last_b_readers.resize(breg_count);                      // start off as empty list, no Breader seen yet

    // for each gate pointer ins in the circuit, add a node and add dependencies on previous gates to it
    for (const auto &ins : kernel->gates) {
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
        strip_name(iname);

        // Add node
        ListDigraph::Node currNode = graph.addNode();
        int curr_id = graph.id(currNode);
        instruction[currNode] = ins;
        node.set(ins) = currNode;
        name[currNode] = ins->qasm();   // and this includes any condition!

        // Add edges (arcs)
        // In quantum computing there are no real Reads and Writes on qubits because they cannot be cloned.
        // Every qubit use influences the qubit, updates it, so would be considered a Read+Write at the same time.
        // In dependency graph construction, this leads to WAW-dependency chains of all uses of the same qubit,
        // and hence in a scheduler using this graph to a sequentialization of those uses in the original program order.
        // For a scheduler, only the presence of a dependency counts, not its type (RAW/WAW/etc.).
        //
        // But as in classical computation Reads commute, in quantum computation e.g. Z rotations commute.
        // However, multiple classes of such uses can be readily distinguished, e.g. X rotations and Z rotations.
        // So all X rotations commute and all Zs commute, but an X followed by a Z or vice-versa must be sequentialized.
        // And since a Write for a qubit is not really correct, we call the default behaviour Default.
        // So in classical computing with 2 event types, there can be 4 kinds of dependences: RAR, RAW, WAR, and WAW;
        // of these an RAR dependence is only created when we explicitly want to sequentialize, i.e. ignore commutability.
        // Similarly with 3 event types in quantum, there can be 9 kinds of dependences:
        // DAD, DAX, DAZ, XAD, XAX, XAZ, ZAD, ZAX, and ZAZ. Again, XAX and ZAZ dependences
        // are only created when we explicitly want to sequentialize, i.e. ignore commutability.
        // Since dependency graphs also has other uses apart from the scheduler, and we might
        // reconstruct the sets of commuting events later, we annotate the dependence type (and operand) in the edge.
        //
        // In classical computing, Reads not only commute but can be done in parallel.
        // But two Xrotations on the same qubit (and also two Z rotations on the same qubit) cannot be done in parallel.
        // So the independence in the dependence graph should not be interpreted as a license for parallel execution.
        //
        // In a non-resource scheduler such independent gates are put in parallel
        // but it doesn't do harm because it is not a real machine.
        // In a resource-constrained scheduler the resource constraint that prohibits more than one use
        // of the same qubit being active at the same time, will prevent this parallelism.
        // So ignoring Xrotate After Xrotate (XAX) dependencies enables the scheduler to take advantage
        // of the commutation property of Xrotations (among which the target operands of CNOTs.
        // Likewise, ignoring Zrotate After Zrotate (ZAZ) dependencies enables the scheduler to take advantage
        // of the commutation property of Zrotations (among which the control operands of all controlled unitaries,
        // and the CZ target operands).
        
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
            new_event(curr_id, OperandType::BREG, boperand, EventType::BREAD, true);
        }

        // each type of gate has a different 'signature' of events; switch out to each one
        if (iname == "measure") {
            QL_DOUT(". considering " << name[currNode] << " as measure");
            // Default each qubit operand + Cwrite each classical operand + Bwrite each bit operand
            for (auto operand : ins->operands) {
                new_event(curr_id, OperandType::QUBIT, operand, EventType::DEFAULT, false);
            }
            for (auto coperand : ins->creg_operands) {
                new_event(curr_id, OperandType::CREG, coperand, EventType::CWRITE, false);
            }
            for (auto boperand : ins->breg_operands) {
                new_event(curr_id, OperandType::BREG, boperand, EventType::BWRITE, false);
            }
            QL_DOUT(". measure done");
        } else if (iname == "display") {
            QL_DOUT(". considering " << name[currNode] << " as display");
            // no operands, display all qubits, cregs and bregs
            // FIXME: operands should have been added when creating this gate; then this special case would not be needed
            // Default on each qubit operand
            // Cwrite on each classical operand
            // Bwrite on each bit operand
            Vec<UInt> qubits(qubit_count);
            std::iota(qubits.begin(), qubits.end(), 0);
            for (auto operand : qubits) {
                new_event(curr_id, OperandType::QUBIT, operand, EventType::DEFAULT, false);
            }
            Vec<UInt> cregs(creg_count);
            std::iota(cregs.begin(), cregs.end(), 0);
            for (auto coperand : cregs) {
                new_event(curr_id, OperandType::CREG, coperand, EventType::CWRITE, false);
            }
            Vec<UInt> bregs(breg_count);
            std::iota(bregs.begin(), bregs.end(), 0);
            for (auto boperand : bregs) {
                new_event(curr_id, OperandType::BREG, boperand, EventType::BWRITE, false);
            }
        } else if (ins->type() == ir::GateType::CLASSICAL) {
            QL_DOUT(". considering " << name[currNode] << " as classical gate");
            // Cwrite each classical operand
            for (auto coperand : ins->creg_operands) {
                new_event(curr_id, OperandType::CREG, coperand, EventType::CWRITE, false);
            }
        } else if (iname == "cnot") {
            QL_DOUT(". considering " << name[currNode] << " as cnot");
            // CNOTs first operand is control and a Zrotate, second operand is target and an Xrotate
            QL_ASSERT(ins->operands.size() == 2);
            new_event(curr_id, OperandType::QUBIT, ins->operands[0], EventType::ZROTATE, commute_multi_qubit);
            new_event(curr_id, OperandType::QUBIT, ins->operands[1], EventType::XROTATE, commute_multi_qubit);
        } else if (iname == "cz" || iname == "cphase") {
            QL_DOUT(". considering " << name[currNode] << " as cz");
            // CZs operands are both Zrotates
            QL_ASSERT(ins->operands.size() == 2);
            new_event(curr_id, OperandType::QUBIT, ins->operands[0], EventType::ZROTATE, commute_multi_qubit);
            new_event(curr_id, OperandType::QUBIT, ins->operands[1], EventType::ZROTATE, commute_multi_qubit);
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
            new_event(curr_id, OperandType::QUBIT, ins->operands[0], EventType::ZROTATE, commute_single_qubit);
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
            new_event(curr_id, OperandType::QUBIT, ins->operands[0], EventType::XROTATE, commute_single_qubit);
        } else {
            QL_DOUT(". considering " << name[currNode] << " as no special gate (catch-all, generic rules)");
            // Default on each qubit operand
            // Cwrite on each classical operand
            // Bwrite on each bit operand
            for (auto operand : ins->operands) {
                new_event(curr_id, OperandType::QUBIT, operand, EventType::DEFAULT, false);
            }
            for (auto coperand : ins->creg_operands) {
                new_event(curr_id, OperandType::CREG, coperand, EventType::CWRITE, false);
            }
            for (auto boperand : ins->breg_operands) {
                new_event(curr_id, OperandType::BREG, boperand, EventType::BWRITE, false);
            }
        } // end of if/else
        QL_DOUT(". instruction done: " << ins->qasm());
    } // end of instruction for

    QL_DOUT("adding deps to SINK");
    // finish filling the dependency graph by creating the t node, the bottom of the graph
    {
        // add dummy target node
        ListDigraph::Node curr_node = graph.addNode();
        int curr_id = graph.id(curr_node);
        instruction[curr_node].emplace<ir::gate_types::Sink>();    // so SINK is defined as instruction[t], not unique in itself
        node.set(instruction[curr_node]) = curr_node;
        name[curr_node] = instruction[curr_node]->qasm();
        t = curr_node;

        // add deps to the dummy target node to close the dependency chains
        // it behaves as a Default to every qubit, Cwrite/Bwrite to every creg and breg
        //
        // to guarantee that exactly at start of execution of dummy SINK,
        // all still executing nodes complete, give arc weight of those nodes;
        // this is relevant for ALAP (which starts backward from SINK for all these nodes);
        // also for accurately computing the circuit's depth (which includes full completion);
        // and also for implementing scheduling and mapping across control-flow (so that it is
        // guaranteed that on a jump and on start of target circuit, the source circuit completed).
        //
        // note that there always is a LastWriter: the dummy source node wrote to every qubit and class. reg
        Vec<UInt> qubits(qubit_count);
        std::iota(qubits.begin(), qubits.end(), 0);
        for (auto operand : qubits) {
            new_event(curr_id, OperandType::QUBIT, operand, EventType::DEFAULT, false);
        }
        Vec<UInt> cregs(creg_count);
        std::iota(cregs.begin(), cregs.end(), 0);
        for (auto coperand : cregs) {
            new_event(curr_id, OperandType::CREG, coperand, EventType::CWRITE, false);
        }
        Vec<UInt> bregs(breg_count);
        std::iota(bregs.begin(), bregs.end(), 0);
        for (auto boperand : bregs) {
            new_event(curr_id, OperandType::BREG, boperand, EventType::BWRITE, false);
        }
    }

    // when in doubt about dependence graph, enable next line to get a dump of it in debugging output
    dprint_depgraph("init");

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
void Scheduler::dprint_depgraph(const Str &s) const {
    if (logger::log_level >= logger::LogLevel::LOG_DEBUG) {
        std::cout << "Depgraph " << s << std::endl;
        for (ListDigraph::NodeIt n(graph); n != lemon::INVALID; ++n) {
            std::cout << "Node " << graph.id(n) << " \"" << name[n] << "\" :" << std::endl;
            std::cout << "    out:";
            for (ListDigraph::OutArcIt arc(graph,n); arc != lemon::INVALID; ++arc) {
                std::cout << " Arc(" << graph.id(arc) << "," << dep_type[arc] << "," << op_type[arc] << "[" << cause[arc] << "])->node(" << graph.id(graph.target(arc)) << ")";
            }
            std::cout << std::endl;
            std::cout << "    in:";
            for (ListDigraph::InArcIt arc(graph,n); arc != lemon::INVALID; ++arc) {
                std::cout << " Arc(" << graph.id(arc) << "," << dep_type[arc] << "," << op_type[arc] << "[" << cause[arc] << "])<-node(" << graph.id(graph.source(arc)) << ")";
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
        arcMap("optype", op_type).
        arcMap("cause", cause).
        arcMap("weight", weight).
        // arcMap("depType", depType).
        node("source", s).
        node("target", t).
        run();
}

void Scheduler::write_dependence_matrix() const {
    QL_COUT("Printing dependency Matrix ...");
    Str datfname(output_prefix + "dependenceMatrix.dat");
    OutFile fout(datfname);

    UInt total_instructions = countNodes(graph);
    Vec<Vec<Bool> > matrix(total_instructions, Vec<Bool>(total_instructions));

    // now print the edges
    for (ListDigraph::ArcIt arc(graph); arc != lemon::INVALID; ++arc) {
        auto srcNode = graph.source(arc);
        auto dstNode = graph.target(arc);
        UInt srcID = graph.id( srcNode );
        UInt dstID = graph.id( dstNode );
        matrix[srcID][dstID] = true;
    }

    for (UInt i = 1; i < total_instructions - 1; i++) {
        for (UInt j = 1; j < total_instructions - 1; j++) {
            fout << matrix[j][i] << "\t";
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
void Scheduler::set_cycle_gate(const ir::GateRef &gp, rmgr::Direction dir) {
    ListDigraph::Node curr_node = node.at(gp);
    UInt  curr_cycle;
    if (dir == rmgr::Direction::FORWARD) {
        curr_cycle = 0;
        for (ListDigraph::InArcIt arc(graph, curr_node); arc != lemon::INVALID; ++arc) {
            auto nextgp = instruction[graph.source(arc)];
            if (nextgp->cycle == ir::MAX_CYCLE) {
                set_cycle_gate(nextgp, dir);
            }
            curr_cycle = max<UInt>(curr_cycle, nextgp->cycle + weight[arc]);
        }
    } else {
        curr_cycle = ALAP_SINK_CYCLE;
        for (ListDigraph::OutArcIt arc(graph, curr_node); arc != lemon::INVALID; ++arc) {
            auto nextgp = instruction[graph.target(arc)];
            if (nextgp->cycle == ir::MAX_CYCLE) {
                set_cycle_gate(nextgp, dir);
            }
            curr_cycle = min<UInt>(curr_cycle, nextgp->cycle - weight[arc]);
        }
    }
    gp->cycle = curr_cycle;
    QL_DOUT("... set_cycle of " << gp->qasm() << " cycles " << gp->cycle);
}

void Scheduler::set_cycle(rmgr::Direction dir) {
    // note when iterating that graph contains SOURCE and SINK whereas the circuit doesn't
    for (ListDigraph::NodeIt n(graph); n != lemon::INVALID; ++n) {
        instruction[n]->cycle = ir::MAX_CYCLE;       // not yet visited successfully by set_cycle_gate
    }
    if (dir == rmgr::Direction::FORWARD) {
        set_cycle_gate(instruction[s], dir);
        for (auto gpit = kernel->gates.begin(); gpit != kernel->gates.end(); gpit++) {
            if ((*gpit)->cycle == ir::MAX_CYCLE) {
                set_cycle_gate(*gpit, dir);
            }
        }
        set_cycle_gate(instruction[t], dir);
    } else {
        set_cycle_gate(instruction[t], dir);
        for (auto gpit = kernel->gates.rbegin(); gpit != kernel->gates.rend(); gpit++) {
            if ((*gpit)->cycle == ir::MAX_CYCLE) {
                set_cycle_gate(*gpit, dir);
            }
        }
        set_cycle_gate(instruction[s], dir);

        // readjust cycle values of gates so that SOURCE is at 0
        UInt  SOURCECycle = instruction[s]->cycle;
        QL_DOUT("... readjusting cycle values by -" << SOURCECycle);

        instruction[t]->cycle -= SOURCECycle;
        for (auto &gp : kernel->gates) {
            gp->cycle -= SOURCECycle;
        }
        instruction[s]->cycle -= SOURCECycle;   // i.e. becomes 0
    }
}

static Bool cycle_lessthan(const ir::GateRef &gp1, const ir::GateRef &gp2) {
    return gp1->cycle < gp2->cycle;
}

// sort circuit by the gates' cycle attribute in non-decreasing order
void Scheduler::sort_by_cycle(ir::GateRefs &cp) {
    QL_DOUT("... before sorting on cycle value");
    // for ( circuit::iterator gpit = cp->begin(); gpit != cp->end(); gpit++)
    // {
    //     gate*           gp = *gpit;
    //     DOUT("...... (@" << gp->cycle << ") " << gp->qasm());
    // }

    // std::sort doesn't preserve the original order of elements that have equal values but std::stable_sort does
    std::stable_sort(cp.begin(), cp.end(), cycle_lessthan);

    QL_DOUT("... after sorting on cycle value");
    // for ( circuit::iterator gpit = cp->begin(); gpit != cp->end(); gpit++)
    // {
    //     gate*           gp = *gpit;
    //     DOUT("...... (@" << gp->cycle << ") " << gp->qasm());
    // }
}

// ASAP scheduler without RC, setting gate cycle values and sorting the resulting circuit
void Scheduler::schedule_asap() {
    QL_DOUT("Scheduling ASAP ...");
    set_cycle(rmgr::Direction::FORWARD);
    sort_by_cycle(kernel->gates);
    kernel->cycles_valid = true;
    QL_DOUT("Scheduling ASAP [DONE]");
}

// ALAP scheduler without RC, setting gate cycle values and sorting the resulting circuit
void Scheduler::schedule_alap() {
    QL_DOUT("Scheduling ALAP ...");
    set_cycle(rmgr::Direction::BACKWARD);
    sort_by_cycle(kernel->gates);
    kernel->cycles_valid = true;
    QL_DOUT("Scheduling ALAP [DONE]");
}

// remaining[node] == cycles until end of schedule; nodes with highest remaining are most critical
// it is without RC and depends on direction: forward:ASAP so cycles until SINK, backward:ALAP so cycles until SOURCE;
// remaining[node] is complementary to node's cycle value,
// so the implementation below is also a systematically modified copy of that of set_cycle_gate and set_cycle
void Scheduler::set_remaining_gate(const ir::GateRef &gp, rmgr::Direction dir) {
    ListDigraph::Node curr_node = node.at(gp);
    UInt curr_remain = 0;
    QL_DOUT("... set_remaining of node " << graph.id(curr_node) << ": " << gp->qasm() << " ...");
    if (dir == rmgr::Direction::FORWARD) {
        for (ListDigraph::OutArcIt arc(graph, curr_node); arc != lemon::INVALID; ++arc) {
            auto nextNode = graph.target(arc);
            QL_DOUT("...... target of arc " << graph.id(arc) << " to node " << graph.id(nextNode));
            if (remaining.at(nextNode) == ir::MAX_CYCLE) {
                set_remaining_gate(instruction[nextNode], dir);
            }
            curr_remain = max<UInt>(curr_remain, remaining.at(nextNode) + weight[arc]);
        }
    } else {
        for (ListDigraph::InArcIt arc(graph, curr_node); arc != lemon::INVALID; ++arc) {
            auto nextNode = graph.source(arc);
            QL_DOUT("...... source of arc " << graph.id(arc) << " from node " << graph.id(nextNode));
            if (remaining.at(nextNode) == ir::MAX_CYCLE) {
                set_remaining_gate(instruction[nextNode], dir);
            }
            curr_remain = max<UInt>(curr_remain, remaining.at(nextNode) + weight[arc]);
        }
    }
    remaining.set(curr_node) = curr_remain;
    QL_DOUT("... set_remaining of node " << graph.id(curr_node) << ": " << gp->qasm() << " remaining " << curr_remain);
}

void Scheduler::set_remaining(rmgr::Direction dir) {
    // note when iterating that graph contains SOURCE and SINK whereas the circuit doesn't;
    // regretfully, the order of visiting the nodes while iterating over the graph, is undefined
    // and in set_remaining (and set_cycle) the order matters (i.e. in circuit order or reversed circuit order)
    for (ListDigraph::NodeIt n(graph); n != lemon::INVALID; ++n) {
        remaining.set(n) = ir::MAX_CYCLE;               // not yet visited successfully by set_remaining_gate
    }
    if (dir == rmgr::Direction::FORWARD) {
        // remaining until SINK (i.e. the SINK.cycle-ALAP value)
        set_remaining_gate(instruction[t], dir);
        for (auto gpit = kernel->gates.rbegin(); gpit != kernel->gates.rend(); gpit++) {
            if (remaining.at(node.at(*gpit)) == ir::MAX_CYCLE) {
                set_remaining_gate(*gpit, dir);
            }
        }
        set_remaining_gate(instruction[s], dir);
    } else {
        // remaining until SOURCE (i.e. the ASAP value)
        set_remaining_gate(instruction[s], dir);
        for (auto gpit = kernel->gates.begin(); gpit != kernel->gates.end(); gpit++) {
            if (remaining.at(node.at(*gpit)) == ir::MAX_CYCLE) {
                set_remaining_gate(*gpit, dir);
            }
        }
        set_remaining_gate(instruction[t], dir);
    }
}

ir::GateRef Scheduler::find_mostcritical(const List<ir::GateRef> &lg) {
    UInt max_remain = 0;
    ir::GateRef most_critical_gate = {};
    for (const auto &gp : lg) {
        UInt gr = remaining.at(node.at(gp));
        if (gr > max_remain) {
            most_critical_gate = gp;
            max_remain = gr;
        }
    }
    QL_DOUT("... most critical gate: " << most_critical_gate->qasm() << " with remaining=" << max_remain);
    return most_critical_gate;
}

// Set the curr_cycle of the scheduling algorithm to start at the appropriate end as well;
// note that the cycle attributes will be shifted down to start at 1 after backward scheduling.
void Scheduler::init_available(
    List<ListDigraph::Node> &avlist,
    rmgr::Direction dir,
    UInt &curr_cycle
) {
    avlist.clear();
    if (dir == rmgr::Direction::FORWARD) {
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
    rmgr::Direction dir,
    List<ListDigraph::Node> &ln
) {
    if (dir == rmgr::Direction::FORWARD) {
        for (ListDigraph::OutArcIt succ_arc(graph, n); succ_arc != lemon::INVALID; ++succ_arc) {
            auto succ_node = graph.target(succ_arc);
            // DOUT("...... succ of " << instruction[n]->qasm() << " : " << instruction[succNode]->qasm());
            Bool found = false;             // filter out duplicates
            for (auto anySuccNode : ln) {
                if (succ_node == anySuccNode) {
                    // DOUT("...... duplicate: " << instruction[succNode]->qasm());
                    found = true;           // duplicate found
                }
            }
            if (!found) {                   // found new one
                ln.push_back(succ_node);     // new node to ln
            }
        }
        // ln contains depending nodes of n without duplicates
    } else {
        for (ListDigraph::InArcIt pred_arc(graph, n); pred_arc != lemon::INVALID; ++pred_arc) {
            ListDigraph::Node pred_node = graph.source(pred_arc);
            // DOUT("...... pred of " << instruction[n]->qasm() << " : " << instruction[predNode]->qasm());
            Bool found = false;             // filter out duplicates
            for (auto any_pred_node : ln) {
                if (pred_node == any_pred_node) {
                    // DOUT("...... duplicate: " << instruction[predNode]->qasm());
                    found = true;           // duplicate found
                }
            }
            if (!found) {                   // found new one
                ln.push_back(pred_node);     // new node to ln
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
    rmgr::Direction dir
) {
    if (n1 == n2) return false;             // because not <

    if (remaining.at(n1) < remaining.at(n2)) return true;
    if (!enable_criticality) return false;
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
void Scheduler::make_available(
    ListDigraph::Node n,
    utils::List<lemon::ListDigraph::Node> &avlist,
    rmgr::Direction dir
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
void Scheduler::take_available(
    ListDigraph::Node n,
    utils::List<lemon::ListDigraph::Node> &avlist,
    utils::Map<ir::GateRef, utils::Bool> &scheduled,
    rmgr::Direction dir
) {
    scheduled.set(instruction[n]) = true;
    avlist.remove(n);

    if (dir == rmgr::Direction::FORWARD) {
        for (ListDigraph::OutArcIt succ_arc(graph, n); succ_arc != lemon::INVALID; ++succ_arc) {
            auto succ_node = graph.target(succ_arc);
            Bool schedulable = true;
            for (ListDigraph::InArcIt pred_arc(graph, succ_node); pred_arc != lemon::INVALID; ++pred_arc) {
                ListDigraph::Node predNode = graph.source(pred_arc);
                if (!scheduled.at(instruction[predNode])) {
                    schedulable = false;
                    break;
                }
            }
            if (schedulable) {
                make_available(succ_node, avlist, dir);
            }
        }
    } else {
        for (ListDigraph::InArcIt pred_arc(graph, n); pred_arc != lemon::INVALID; ++pred_arc) {
            auto pred_node = graph.source(pred_arc);
            Bool schedulable = true;
            for (ListDigraph::OutArcIt succ_arc(graph, pred_node); succ_arc != lemon::INVALID; ++succ_arc) {
                auto succ_node = graph.target(succ_arc);
                if (!scheduled.at(instruction[succ_node])) {
                    schedulable = false;
                    break;
                }
            }
            if (schedulable) {
                make_available(pred_node, avlist, dir);
            }
        }
    }
}

// advance curr_cycle
// when no node was selected from the avlist, advance to the next cycle
// and try again; this makes nodes/instructions to complete execution for one more cycle,
// and makes resources finally available in case of resource constrained scheduling
// so it contributes to proceeding and to finally have an empty avlist
void Scheduler::advance_curr_cycle(rmgr::Direction dir, UInt &curr_cycle) {
    if (dir == rmgr::Direction::FORWARD) {
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
    rmgr::Direction dir,
    const UInt curr_cycle,
    rmgr::State &rs,
    Bool &isres
) {
    ir::GateRef gp = instruction[n];
    isres = true;
    // have dependent gates completed at curr_cycle?
    if (
        (dir == rmgr::Direction::FORWARD && gp->cycle <= curr_cycle)
        || (dir == rmgr::Direction::BACKWARD && curr_cycle <= gp->cycle)
        ) {
        // are resources available?
        if (
            n == s || n == t
            || gp->type() == ir::GateType::DUMMY
            || gp->type() == ir::GateType::CLASSICAL
            || gp->type() == ir::GateType::WAIT
            ) {
            return true;
        }
        if (rs.available(curr_cycle, gp)) {
            return true;
        }
        isres = true;
        return false;
    } else {
        isres = false;
        return false;
    }
}

// select a node from the avlist
// the avlist is deep-ordered from high to low criticality (see criticality_lessthan above)
ListDigraph::Node Scheduler::select_available(
    utils::List<lemon::ListDigraph::Node> &avlist,
    rmgr::Direction dir,
    const UInt curr_cycle,
    rmgr::State &rs,
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
        if (instruction[n]->duration == 0 && immediately_schedulable(n, dir, curr_cycle, rs, isres)) {
            QL_DOUT("... node (@" << instruction[n]->cycle << "): " << name[n] << " duration 0 and immediately schedulable, remaining=" << remaining.dbg(n) << ", selected");
            success = true;
            return n;
        }
    }
    // select the first (most critical) immediately schedulable, if any, otherwise
    // since avlist is deep-criticality ordered, highest first, the first is the most deep-critical
    for (auto n : avlist) {
        Bool isres;
        if (immediately_schedulable(n, dir, curr_cycle, rs, isres)) {
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
    rmgr::Direction dir,
    const rmgr::Manager &rm
) {
    QL_DOUT("Scheduling " << (dir == rmgr::Direction::FORWARD ? "ASAP" : "ALAP") << " with RC ...");

    // build a new resource state
    auto rs = rm.build(dir);

    // scheduled[gp] :=: whether gate *gp has been scheduled, init all false
    Map<ir::GateRef, Bool> scheduled;
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

        selected_node = select_available(avlist, dir, curr_cycle, rs, success);
        if (!success) {
            // i.e. none from avlist was found suitable to schedule in this cycle
            advance_curr_cycle(dir, curr_cycle);
            // so try again; eventually instrs complete and machine is empty
            continue;
        }

        // commit selected_node to the schedule
        ir::GateRef gp = instruction[selected_node];
        QL_DOUT("... selected " << gp->qasm() << " in cycle " << curr_cycle);
        gp->cycle = curr_cycle;                     // scheduler result, including s and t
        if (
            selected_node != s
            && selected_node != t
            && gp->type() != ir::GateType::DUMMY
            && gp->type() != ir::GateType::CLASSICAL
            && gp->type() != ir::GateType::WAIT
            ) {
            rs.reserve(curr_cycle, gp);
        }
        take_available(selected_node, avlist, scheduled, dir);   // update avlist/scheduled/cycle
        // more nodes that could be scheduled in this cycle, will be found in an other round of the loop
    }

    QL_DOUT("... sorting on cycle value");
    sort_by_cycle(kernel->gates);

    if (dir == rmgr::Direction::BACKWARD) {
        // readjust cycle values of gates so that SOURCE is at 0
        UInt SOURCECycle = instruction[s]->cycle;
        QL_DOUT("... readjusting cycle values by -" << SOURCECycle);

        instruction[t]->cycle -= SOURCECycle;
        for (auto &gp : kernel->gates) {
            gp->cycle -= SOURCECycle;
        }
        instruction[s]->cycle -= SOURCECycle;   // i.e. becomes 0
    }
    kernel->cycles_valid = true;

    // end scheduling

    QL_DOUT("Scheduling " << (dir == rmgr::Direction::FORWARD ? "ASAP" : "ALAP") << " with RC [DONE]");
}

void Scheduler::schedule_asap(
    const rmgr::Manager &rm
) {
    QL_DOUT("Scheduling ASAP");
    schedule(rmgr::Direction::FORWARD, rm);
    QL_DOUT("Scheduling ASAP [DONE]");
}

void Scheduler::schedule_alap(
    const rmgr::Manager &rm
) {
    QL_DOUT("Scheduling ALAP");
    schedule(rmgr::Direction::BACKWARD, rm);
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
    set_cycle(rmgr::Direction::FORWARD);
    UInt cycle_count = instruction[t]->cycle - 1;
    // so SOURCE at cycle 0, then all circuit's gates at cycles 1 to cycle_count, and finally SINK at cycle cycle_count+1

    // compute remaining which is the opposite of the alap cycle value (remaining[node] :=: SINK->cycle - alapcycle[node])
    // remaining[node] indicates number of cycles remaining in schedule from node's execution start to SINK,
    // and indicates the latest cycle that the node can be scheduled so that the circuit's depth is not increased.
    set_remaining(rmgr::Direction::FORWARD);

    // DOUT("Creating gates_per_cycle");
    // create gates_per_cycle[cycle] = for each cycle the list of gates at cycle cycle
    // this is the basic map to be operated upon by the uniforming scheduler below;
    Map<UInt, List<ir::GateRef>> gates_per_cycle;
    for (const auto &gp : kernel->gates) {
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

        Int pred_cycle = curr_cycle - 1;    // signed because can become negative

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
            UInt min_remaining_cycle = ir::MAX_CYCLE;
            List<ir::GateRef>::const_iterator best_predgp_it;
            ir::GateRef best_predgp = {};
            Bool best_predgp_found = false;

            // scan bundle at pred_cycle to find suitable candidate to move forward to curr_cycle
            for (auto predgp_it = gates_per_cycle.get(pred_cycle).begin(); predgp_it != gates_per_cycle.get(pred_cycle).end(); ++predgp_it) {
                auto predgp = *predgp_it;
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
                        ir::GateRef target_gp = instruction[graph.target(arc)];
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
                    best_predgp_it = predgp_it;
                }
            }

            // when candidate was found in this bundle, move it, and search for more in this bundle, if needed
            // otherwise, continue scanning backward
            if (best_predgp_found) {
                // move predgp from pred_cycle to curr_cycle;
                // adjust all bookkeeping that is affected by this
                gates_per_cycle.at(pred_cycle).erase(best_predgp_it);
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
    sort_by_cycle(kernel->gates);
    kernel->cycles_valid = true;

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
    Bool with_critical,
    Bool with_cycles,
    std::ostream &dotout
) {
    QL_DOUT("Get_dot");
    ListDigraphPath p;
    ListDigraph::ArcMap<Bool> is_in_critical{graph};
    if (with_critical) {
        for (ListDigraph::ArcIt a(graph); a != lemon::INVALID; ++a) {
            is_in_critical[a] = false;
            for (ListDigraphPath::ArcIt ap(p); ap != lemon::INVALID; ++ap) {
                if (a == ap) {
                    is_in_critical[a] = true;
                    break;
                }
            }
        }
    }

    Str node_style(" fontcolor=black, style=filled, fontsize=16");
    Str edge_style_1(" color=black");
    Str edge_style_2(" color=red");
    Str edge_style = edge_style_1;

    dotout << "digraph {\ngraph [ rankdir=TD; ]; // or rankdir=LR"
           << "\nedge [fontsize=16, arrowhead=vee, arrowsize=0.5];"
           << std::endl;

    // first print the nodes
    for (ListDigraph::NodeIt n(graph); n != lemon::INVALID; ++n) {
        dotout << "\"" << graph.id(n) << "\""
               << " [label=\" " << name[n] << " \""
               << node_style
                << "];" << std::endl;
    }

    if (with_cycles) {
        // Print cycle numbers as timeline, as shown below
        UInt total_cycles;
        if (kernel->gates.empty()) {
            total_cycles = 1;    // +1 is SOURCE's duration in cycles
        } else {
            total_cycles = kernel->gates.back()->cycle + (kernel->gates.back()->duration + cycle_time - 1) / cycle_time
                           - kernel->gates.front()->cycle + 1;    // +1 is SOURCE's duration in cycles
        }
        dotout << "{\nnode [shape=plaintext, fontsize=16, fontcolor=blue]; \n";
        for (UInt cn = 0; cn <= total_cycles; ++cn) {
            if (cn > 0) {
                dotout << " -> ";
            }
            dotout << "Cycle" << cn;
        }
        dotout << ";\n}\n";

        // Now print ranks, as shown below
        dotout << "{ rank=same; Cycle" << instruction[s]->cycle <<"; " << graph.id(s) << "; }\n";
        for (const auto &gp : kernel->gates) {
            dotout << "{ rank=same; Cycle" << gp->cycle <<"; " << graph.id(node.at(gp)) << "; }\n";
        }
        dotout << "{ rank=same; Cycle" << instruction[t]->cycle <<"; " << graph.id(t) << "; }\n";
    }

    // now print the edges
    for (ListDigraph::ArcIt arc(graph); arc != lemon::INVALID; ++arc) {
        auto src_node = graph.source(arc);
        auto dst_node = graph.target(arc);
        Int src_id = graph.id(src_node );
        Int dst_id = graph.id(dst_node );

        if (with_critical) {
            edge_style = (is_in_critical[arc] == true) ? edge_style_2 : edge_style_1;
        }

        dotout << std::dec
               << "\"" << src_id << "\""
               << "->"
               << "\"" << dst_id << "\""
               << "[ label=\""
               << op_type[arc] << "[" << cause[arc] << "]"
               << " , " << weight[arc]
               << " , " << dep_type[arc]
               << "\""
               << " " << edge_style << " "
               << "]"
               << std::endl;
    }

    dotout << "}" << std::endl;
    QL_DOUT("Get_dot[DONE]");
}

void Scheduler::get_dot(Str &dot) {
    set_cycle(rmgr::Direction::FORWARD);
    sort_by_cycle(kernel->gates);

    StrStrm ssdot;
    get_dot(false, true, ssdot);
    dot = ssdot.str();
}

} // namespace detail
} // namespace schedule
} // namespace sch
} // namespace pass
} // namespace ql
