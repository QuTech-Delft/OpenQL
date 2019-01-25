/**
 * @file   scheduler.h
 * @date   01/2017
 * @author Imran Ashraf
 * @author Hans van Someren
 * @brief  ASAP/ALAP and UNIFORM scheduling with and without resource constraint
 */

#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include <lemon/list_graph.h>
#include <lemon/lgf_reader.h>
#include <lemon/lgf_writer.h>
#include <lemon/dijkstra.h>
#include <lemon/connectivity.h>

#include "ql/utils.h"
#include "ql/gate.h"
#include "ql/circuit.h"
#include "ql/ir.h"
#include "ql/arch/cc_light/cc_light_resource_manager.h"     // FIXME(WJV): uses specific backend code (is it really specific to cc_light?)

using namespace std;
using namespace lemon;

enum DepTypes{RAW, WAW, WAR, RAR};
const string DepTypesNames[] = {"RAW", "WAW", "WAR", "RAR"};

class Scheduler
{
private:
    // dependence graph is constructed (see Init) once, and can be reused as often as needed
    ListDigraph graph;

    ListDigraph::NodeMap<ql::gate*> instruction;                    // instruction[n] == gate*
    ListDigraph::NodeMap<std::string> name;                         // name[n] == qasm string
    ListDigraph::ArcMap<int> weight;                                // number of cycles of dependence
    //TODO it might be more readable to change 'cause' to string
    //   to accomodate/print both r0, q0 operands as cause
    ListDigraph::ArcMap<int> cause;                                 // qubit/creg index of dependence
    ListDigraph::ArcMap<int> depType;                               // RAW, WAW, ...
    ListDigraph::Node s, t;                     // instruction[s]==SOURCE, instruction[t]==SINK

    // parameters of dependence graph construction
    size_t          cycle_time;                 // to convert durations to cycles as weight of dependence
    size_t          qubit_count;                // to check/represent qubit as cause of dependence
    size_t          creg_count;                 // to check/represent cbit as cause of dependence
    ql::circuit*    circp;                      // current and result circuit, should be parameter of scheduler

    std::map< std::pair<std::string,std::string>, size_t> buffer_cycles_map;
    std::map<ql::gate*,ListDigraph::Node>  node;    // node[gate*] == node_id
    std::map<ListDigraph::Node,size_t>  remaining;  // remaining[node] == cycles until end


public:
    Scheduler(): instruction(graph), name(graph), weight(graph),
        cause(graph), depType(graph) {}

    void Init(ql::circuit& ckt, ql::quantum_platform platform, size_t qcount, size_t ccount)
    {
        DOUT("Scheduler initialization ...");
        qubit_count = qcount;
        creg_count = ccount;
        size_t qubit_creg_count = qubit_count + creg_count;
        cycle_time = platform.cycle_time;
        circp = &ckt;

        // populate buffer map
        // 'none' type is a dummy type and 0 buffer cycles will be inserted for
        // instructions of type 'none'
        std::vector<std::string> buffer_names = {"none", "mw", "flux", "readout"};
        for(auto & buf1 : buffer_names)
        {
            for(auto & buf2 : buffer_names)
            {
                auto bpair = std::pair<std::string,std::string>(buf1,buf2);
                auto bname = buf1+ "_" + buf2 + "_buffer";
                if(platform.hardware_settings.count(bname) > 0)
                {
                    buffer_cycles_map[ bpair ] = std::ceil( 
                        static_cast<float>(platform.hardware_settings[bname]) / cycle_time);
                }
                // DOUT("Initializing " << bname << ": "<< buffer_cycles_map[bpair]);
            }
        }

        // add dummy source node
        ListDigraph::Node srcNode = graph.addNode();
        instruction[srcNode] = new ql::SOURCE();
        node[instruction[srcNode]] = srcNode;
        name[srcNode] = instruction[srcNode]->qasm();
        s=srcNode;

        typedef vector<int> ReadersListType;
        vector<ReadersListType> LastReaders;
        LastReaders.resize(qubit_creg_count);

        int srcID = graph.id(srcNode);
        vector<int> LastWriter(qubit_creg_count,srcID);

        for( auto ins : ckt )
        {
            DOUT("Current instruction : " << ins->qasm());

            // Add nodes
            ListDigraph::Node consNode = graph.addNode();
            int consID = graph.id(consNode);
            instruction[consNode] = ins;
            node[ins] = consNode;
            name[consNode] = ins->qasm();

            // Add edges
            // In quantum computing there are no real Reads and Writes on qubits because they cannot be cloned.
            // Every qubit use influences the qubit, updates it, so would be considered a Read+Write at the same time.
            // In dependence graph construction, this leads to RAW-dependence chains of all uses of the same qubit,
            // and hence in a scheduler using this graph to a sequentialization of those uses in the original program order.
            // For a scheduler, only the presence of a dependence counts, not its type (RAW/WAW/etc.).
            // A dependence graph also has other uses apart from the scheduler: e.g. to find chains of live qubits,
            // from their creation (Prep etc.) to their destruction (Measure, etc.) in allocation of virtual to real qubits.
            // For those uses it makes sense to make a difference with a gate doing a Read+Write, just a Write or just a Read:
            // a Prep creates a new 'value' (Write); wait, display, x, swap, cnot, all pass this value on (so Read+Write),
            // while a Measure 'destroys' the 'value' (Read+Write of the qubit, Write of the creg),
            // the destruction aspect of a Measure being implied by it being followed by a Prep (Write only) on the same qubit.
            // Furthermore Writes can model barriers on a qubit (see Wait, Display, etc.), because Writes sequentialize.
            // The dependence graph creation below models a graph suitable for all functions, including chains of live qubits.

            if (ql::options::get("scheduler_post179") == "yes")
            {
            // Control-operands of Controlled Unitaries commute, independent of the unitary,
            // i.e. these qubit uses need not be kept in order.
            // But, of course, those uses should be ordered after (/before) the last (/next) non-control use of the qubit.
            // In this way, those control-operand qubit uses would be like pure Reads in dependence graph construction.
            // A problem might be that the gates with the same control-operands might be scheduled in parallel then.
            // In a non-resource scheduler that will happen but it doesn't do harm because it is not a real machine.
            // In a resource-constrained scheduler the resource constraint that prohibits more than one use
            // of the same qubit being active at the same time, will prevent this parallelism.
            // So ignoring Read After Read (RAR) dependences enables the scheduler to take advantage
            // of the commutation property of Controlled Unitaries without disadvantages.
            }

            auto operands = ins->operands;
            size_t op_count = operands.size();
            size_t operandNo=0;

            if(ins->name == "measure")
            {
                DOUT(". considering " << name[consNode] << " as measure");
                // Read+Write each qubit operand + Write corresponding creg
                for( auto operand : operands )
                {
                    DOUT(".. Operand: " << operand);
                    { // RAW dependencies
                        int prodID = LastWriter[operand];
                        ListDigraph::Node prodNode = graph.nodeFromId(prodID);
                        ListDigraph::Arc arc = graph.addArc(prodNode,consNode);
                        weight[arc] = std::ceil( static_cast<float>(instruction[prodNode]->duration) / cycle_time);
                        cause[arc] = operand;
                        depType[arc] = RAW;
                        DOUT("... dep " << name[prodNode] << " -> " << name[consNode] << " (opnd=" << operand << ", dep=" << RAW << ")");
                    }

                    { // WAR dependencies
                        ReadersListType readers = LastReaders[operand];
                        for(auto & readerID : readers)
                        {
                            ListDigraph::Node readerNode = graph.nodeFromId(readerID);
                            ListDigraph::Arc arc1 = graph.addArc(readerNode,consNode);
                            weight[arc1] = std::ceil( static_cast<float>(instruction[readerNode]->duration) / cycle_time);
                            cause[arc1] = operand;
                            depType[arc1] = WAR;
                            DOUT("... dep " << name[readerNode] << " -> " << name[consNode] << " (opnd=" << operand << ", dep=" << WAR << ")");
                        }
                    }
                }

                ql::measure * mins = (ql::measure*)ins;
                for( auto operand : mins->creg_operands )
                {
                    DOUT(".. Operand: " << operand);
                    { // WAW dependencies
                        int prodID = LastWriter[qubit_count+operand];
                        ListDigraph::Node prodNode = graph.nodeFromId(prodID);
                        ListDigraph::Arc arc = graph.addArc(prodNode,consNode);
                        weight[arc] = std::ceil( static_cast<float>(instruction[prodNode]->duration) / cycle_time);
                        cause[arc] = operand;
                        depType[arc] = WAW;
                        DOUT("... dep " << name[prodNode] << " -> " << name[consNode] << " (opnd=" << operand << ", dep=" << WAW << ")");
                    }

                    { // WAR dependencies
                        ReadersListType readers = LastReaders[qubit_count+operand];
                        for(auto & readerID : readers)
                        {
                            ListDigraph::Node readerNode = graph.nodeFromId(readerID);
                            ListDigraph::Arc arc1 = graph.addArc(readerNode,consNode);
                            weight[arc1] = std::ceil( static_cast<float>(instruction[readerNode]->duration) / cycle_time);
                            cause[arc1] = operand;
                            depType[arc1] = WAR;
                            DOUT("... dep " << name[readerNode] << " -> " << name[consNode] << " (opnd=" << operand << ", dep=" << WAR << ")");
                        }
                    }
                }

                // update LastWriter and so clear LastReaders
                for( auto operand : operands )
                {
                    LastWriter[operand] = consID;
                    if (ql::options::get("scheduler_post179") == "yes")
                    {
                        LastReaders[operand].clear();
                    }
                }
                for( auto operand : mins->creg_operands )
                {
                    LastWriter[operand] = consID;
                    if (ql::options::get("scheduler_post179") == "yes")
                    {
                        LastReaders[operand].clear();
                    }
                }
            }
            else if(ins->name == "display")
            {
                DOUT(". considering " << name[consNode] << " as display");
                // no operands, display all qubits and cregs
                // Read+Write each operand
                std::vector<size_t> qubits(qubit_creg_count);
                std::iota(qubits.begin(), qubits.end(), 0);
                for( auto operand : qubits )
                {
                    DOUT(".. Operand: " << operand);
                    { // RAW dependencies
                        int prodID = LastWriter[operand];
                        ListDigraph::Node prodNode = graph.nodeFromId(prodID);
                        ListDigraph::Arc arc = graph.addArc(prodNode,consNode);
                        weight[arc] = std::ceil( static_cast<float>(instruction[prodNode]->duration) / cycle_time);
                        cause[arc] = operand;
                        depType[arc] = RAW;
                        DOUT("... dep " << name[prodNode] << " -> " << name[consNode] << " (opnd=" << operand << ", dep=" << RAW << ")");
                    }

                    { // WAR dependencies
                        ReadersListType readers = LastReaders[operand];
                        for(auto & readerID : readers)
                        {
                            ListDigraph::Node readerNode = graph.nodeFromId(readerID);
                            ListDigraph::Arc arc1 = graph.addArc(readerNode,consNode);
                            weight[arc1] = std::ceil( static_cast<float>(instruction[readerNode]->duration) / cycle_time);
                            cause[arc1] = operand;
                            depType[arc1] = WAR;
                            DOUT("... dep " << name[readerNode] << " -> " << name[consNode] << " (opnd=" << operand << ", dep=" << WAR << ")");
                        }
                    }
                }

                // now update LastWriter and so clear LastReaders
                for( auto operand : operands )
                {
                    LastWriter[operand] = consID;
                    if (ql::options::get("scheduler_post179") == "yes")
                    {
                        LastReaders[operand].clear();
                    }
                }
            }
            else if(ins->type() == ql::gate_type_t::__classical_gate__)
            {
                DOUT(". considering " << name[consNode] << " as classical gate");
                std::vector<size_t> all_operands(qubit_creg_count);
                std::iota(all_operands.begin(), all_operands.end(), 0);
                for( auto operand : all_operands )
                {
                    DOUT(".. Operand: " << operand);
                    { // WAW dependencies
                        int prodID = LastWriter[operand];
                        ListDigraph::Node prodNode = graph.nodeFromId(prodID);
                        ListDigraph::Arc arc = graph.addArc(prodNode,consNode);
                        weight[arc] = std::ceil( static_cast<float>(instruction[prodNode]->duration) / cycle_time);
                        cause[arc] = operand;
                        depType[arc] = WAW;
                        DOUT("... dep " << name[prodNode] << " -> " << name[consNode] << " (opnd=" << operand << ", dep=" << WAW << ")");
                    }

                    { // WAR dependencies
                        ReadersListType readers = LastReaders[operand];
                        for(auto & readerID : readers)
                        {
                            ListDigraph::Node readerNode = graph.nodeFromId(readerID);
                            ListDigraph::Arc arc1 = graph.addArc(readerNode,consNode);
                            weight[arc1] = std::ceil( static_cast<float>(instruction[readerNode]->duration) / cycle_time);
                            cause[arc1] = operand;
                            depType[arc1] = WAR;
                            DOUT("... dep " << name[readerNode] << " -> " << name[consNode] << " (opnd=" << operand << ", dep=" << WAR << ")");
                        }
                    }
                }

                // now update LastWriter and so clear LastReaders
                for( auto operand : all_operands )
                {
                    LastWriter[operand] = consID;
                    if (ql::options::get("scheduler_post179") == "yes")
                    {
                        LastReaders[operand].clear();
                    }
                }
            }
            else if (  ins->name == "cnot"
                    || ins->name == "cz"
                    || ins->name == "cphase"
                    // or is a control-unitary in general
                    )
            {
                DOUT(". considering " << name[consNode] << " as control-unitary");
                // Control Unitaries Read all operands, and Write the last operand
                for( auto operand : operands )
                {
                    DOUT(".. Operand: " << operand);
                    { // RAW dependencies
                        int prodID = LastWriter[operand];
                        ListDigraph::Node prodNode = graph.nodeFromId(prodID);
                        ListDigraph::Arc arc = graph.addArc(prodNode,consNode);
                        weight[arc] = std::ceil( static_cast<float>(instruction[prodNode]->duration) / cycle_time);
                        cause[arc] = operand;
                        depType[arc] = RAW;
                        DOUT("... dep " << name[prodNode] << " -> " << name[consNode] << " (opnd=" << operand << ", dep=" << RAW << ")");
                    }

                    if (ql::options::get("scheduler_post179") == "no")
                    {
	                    // RAR dependencies
	                    ReadersListType readers = LastReaders[operand];
	                    for(auto & readerID : readers)
	                    {
	                        ListDigraph::Node readerNode = graph.nodeFromId(readerID);
	                        ListDigraph::Arc arc1 = graph.addArc(readerNode,consNode);
	                        weight[arc1] = std::ceil( static_cast<float>(instruction[readerNode]->duration) / cycle_time);
	                        cause[arc1] = operand;
	                        depType[arc1] = RAR;
	                        DOUT("... dep " << name[readerNode] << " -> " << name[consNode] << " (opnd=" << operand << ", dep=" << RAR << ")");
	                    }
                    }

                    if( operandNo < op_count-1 )
                    {
                        // update LastReaders for this operand
                        LastReaders[operand].push_back(consID);
                    }
                    else
                    {
	                    {   // WAW dependencies
	                        int prodID = LastWriter[operand];
	                        ListDigraph::Node prodNode = graph.nodeFromId(prodID);
	                        ListDigraph::Arc arc = graph.addArc(prodNode,consNode);
	                        weight[arc] = std::ceil( static_cast<float>(instruction[prodNode]->duration) / cycle_time);
	                        cause[arc] = operand;
	                        depType[arc] = WAW;
	                        DOUT("... dep " << name[prodNode] << " -> " << name[consNode] << " (opnd=" << operand << ", dep=" << WAW << ")");
	                    }
	
	                    {   // WAR dependencies
	                        ReadersListType readers = LastReaders[operand];
	                        for(auto & readerID : readers)
	                        {
	                            ListDigraph::Node readerNode = graph.nodeFromId(readerID);
	                            ListDigraph::Arc arc1 = graph.addArc(readerNode,consNode);
	                            weight[arc1] = std::ceil( static_cast<float>(instruction[readerNode]->duration) / cycle_time);
	                            cause[arc1] = operand;
	                            depType[arc1] = WAR;
	                            DOUT("... dep " << name[readerNode] << " -> " << name[consNode] << " (opnd=" << operand << ", dep=" << WAR << ")");
	                        }
	                    }

                        // update LastWriter and so also clear LastReaders for this operand
                        LastWriter[operand] = consID;
                        LastReaders[operand].clear();
                    }
                    operandNo++;
                } // end of operand for
            }
            else
            {
                DOUT(". considering " << name[consNode] << " as general quantum gate");
                // general quantum gate, Read+Write on each operand
                for( auto operand : operands )
                {
                    DOUT(".. Operand: " << operand);
                    { // RAW dependencies
                        int prodID = LastWriter[operand];
                        ListDigraph::Node prodNode = graph.nodeFromId(prodID);
                        ListDigraph::Arc arc = graph.addArc(prodNode,consNode);
                        weight[arc] = std::ceil( static_cast<float>(instruction[prodNode]->duration) / cycle_time);
                        cause[arc] = operand;
                        depType[arc] = RAW;
                        DOUT("... dep " << name[prodNode] << " -> " << name[consNode] << " (opnd=" << operand << ", dep=" << RAW << ")");
                    }

                    { // WAR dependencies
                        ReadersListType readers = LastReaders[operand];
                        for(auto & readerID : readers)
                        {
                            ListDigraph::Node readerNode = graph.nodeFromId(readerID);
                            ListDigraph::Arc arc1 = graph.addArc(readerNode,consNode);
                            weight[arc1] = std::ceil( static_cast<float>(instruction[readerNode]->duration) / cycle_time);
                            cause[arc1] = operand;
                            depType[arc1] = WAR;
                            DOUT("... dep " << name[readerNode] << " -> " << name[consNode] << " (opnd=" << operand << ", dep=" << WAR << ")");
                        }
                    }

                    // update LastWriter and so also clear LastReader for this operand
                    LastWriter[operand] = consID;
                    LastReaders[operand].clear();
                    
                    operandNo++;
                } // end of operand for
            } // end of if/else
        } // end of instruction for

        // add dummy target node
        DOUT("adding deps to SINK");
        ListDigraph::Node targetNode = graph.addNode();
        instruction[targetNode] = new ql::SINK();
        node[instruction[targetNode]] = targetNode;
        name[targetNode] = instruction[targetNode]->qasm();
        t=targetNode;

        // make links to the dummy target node
        OutDegMap<ListDigraph> outDeg(graph);
        for (ListDigraph::NodeIt n(graph); n != INVALID; ++n)
        {
            DOUT(". considering node " << name[n] << " and its outDeg " << outDeg[n]);
            if( outDeg[n] == 0 && n!=targetNode )
            {
                ListDigraph::Arc arc = graph.addArc(n,targetNode);
                // weight[arc] = 1; // TARGET is dummy
                // to guarantee that exactly at start of execution of dummy SINK,
                // all still executing nodes complete, give arc weight of those nodes;
                // this is relevant for ALAP (which starts backward from SINK for all these nodes),
                // for accurately computing the circuit's depth (which includes full completion),
                // and for implementing scheduling and mapping across control-flow (so that it is
                // guaranteed that on a jump and on start of target circuit, the source circuit completed).
                weight[arc] = std::ceil( static_cast<float>(instruction[n]->duration) / cycle_time);
                cause[arc] = 0;
                depType[arc] = WAW;
                DOUT("... dep " << name[n] << " -> " << name[targetNode] << " (opnd=" << 0 << ", dep=" << WAW << ")");
            }
        }

        if( !dag(graph) )
        {
            DOUT("The dependence graph is not a DAG.");
            EOUT("The dependence graph is not a DAG.");
        }
        DOUT("Scheduler initialization Done.");
    }

    void Print()
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

    void PrintMatrix()
    {
        COUT("Printing Dependence Graph as Matrix");
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

    // void PrintDot1_(
    //             bool WithCritical,
    //             bool WithCycles,
    //             ListDigraph::NodeMap<size_t> & cycle,
    //             std::vector<ListDigraph::Node> & order,
    //             std::ostream& dotout
    //             )
    // {
    //     ListDigraph::ArcMap<bool> isInCritical(graph);
    //     if(WithCritical)
    //     {
    //         for (ListDigraph::ArcIt a(graph); a != INVALID; ++a)
    //         {
    //             isInCritical[a] = false;
    //             for ( Path<ListDigraph>::ArcIt ap(p); ap != INVALID; ++ap )
    //             {
    //                 if(a==ap)
    //                 {
    //                     isInCritical[a] = true;
    //                     break;
    //                 }
    //             }
    //         }
    //     }

    //     string NodeStyle(" fontcolor=black, style=filled, fontsize=16");
    //     string EdgeStyle1(" color=black");
    //     string EdgeStyle2(" color=red");
    //     string EdgeStyle = EdgeStyle1;

    //     dotout << "digraph {\ngraph [ rankdir=TD; ]; // or rankdir=LR"
    //         << "\nedge [fontsize=16, arrowhead=vee, arrowsize=0.5];"
    //         << endl;

    //     // first print the nodes
    //     for (ListDigraph::NodeIt n(graph); n != INVALID; ++n)
    //     {
    //         int nid = graph.id(n);
    //         string nodeName = name[n];
    //         dotout  << "\"" << nid << "\""
    //                 << " [label=\" " << nodeName <<" \""
    //                 << NodeStyle
    //                 << "];" << endl;
    //     }

    //     if( WithCycles)
    //     {
    //         // Print cycle numbers as timeline, as shown below
    //         size_t cn=0,TotalCycles=0;
    //         dotout << "{\nnode [shape=plaintext, fontsize=16, fontcolor=blue]; \n";
    //         ListDigraph::NodeMap<size_t>::MapIt it(cycle);
    //         if(it != INVALID)
    //             TotalCycles=cycle[it];
    //         for(cn=0;cn<=TotalCycles;++cn)
    //         {
    //             if(cn>0)
    //                 dotout << " -> ";
    //             dotout << "Cycle" << cn;
    //         }
    //         dotout << ";\n}\n";

    //         // Now print ranks, as shown below
    //         std::vector<ListDigraph::Node>::reverse_iterator rit;
    //         for ( rit = order.rbegin(); rit != order.rend(); ++rit)
    //         {
    //             int nid = graph.id(*rit);
    //             dotout << "{ rank=same; Cycle" << cycle[*rit] <<"; " <<nid<<"; }\n";
    //         }
    //     }

    //     // now print the edges
    //     for (ListDigraph::ArcIt arc(graph); arc != INVALID; ++arc)
    //     {
    //         ListDigraph::Node srcNode = graph.source(arc);
    //         ListDigraph::Node dstNode = graph.target(arc);
    //         int srcID = graph.id( srcNode );
    //         int dstID = graph.id( dstNode );

    //         if(WithCritical)
    //             EdgeStyle = ( isInCritical[arc]==true ) ? EdgeStyle2 : EdgeStyle1;

    //         dotout << dec
    //             << "\"" << srcID << "\""
    //             << "->"
    //             << "\"" << dstID << "\""
    //             << "[ label=\""
    //             << "q" << cause[arc]
    //             << " , " << weight[arc]
    //             << " , " << DepTypesNames[ depType[arc] ]
    //             <<"\""
    //             << " " << EdgeStyle << " "
    //             << "]"
    //             << endl;
    //     }

    //     dotout << "}" << endl;
    // }

    // void PrintDot()
    // {
    //     IOUT("Printing Dependence Graph in DOT");
    //     ofstream dotout;
    //     string dotfname(ql::options::get("output_dir") + "/dependenceGraph.dot");
    //     dotout.open(dotfname, ios::binary);
    //     if ( dotout.fail() )
    //     {
    //         EOUT("opening file " << dotfname << std::endl
    //                  << "Make sure the output directory ("<< ql::options::get("output_dir") << ") exists");
    //         return;
    //     }

    //     ListDigraph::NodeMap<size_t> cycle(graph);
    //     std::vector<ListDigraph::Node> order;
    //     PrintDot1_(false, false, cycle, order, dotout);
    //     dotout.close();
    // }

private:

// =========== pre179 schedulers

    void TopologicalSort(std::vector<ListDigraph::Node> & order)
    {
        // DOUT("Performing Topological sort.");
        ListDigraph::NodeMap<int> rorder(graph);
        if( !dag(graph) )
        {
            EOUT("This digraph is not a DAG.");
        }

        // result is in reverse topological order!
        topologicalSort(graph, rorder);

#ifdef DEBUG
        for (ListDigraph::ArcIt a(graph); a != INVALID; ++a)
        {
            if( rorder[graph.source(a)] > rorder[graph.target(a)] )
                EOUT("Wrong topologicalSort()");
        }
#endif

        for (ListDigraph::NodeMap<int>::MapIt it(rorder); it != INVALID; ++it)
        {
            order.push_back(it);
        }

        // DOUT("Nodes in Topological order:");
        // for ( std::vector<ListDigraph::Node>::reverse_iterator it = order.rbegin(); it != order.rend(); ++it)
        // {
        //     std::cout << name[*it] << std::endl;
        // }
    }

    void PrintTopologicalOrder()
    {
        std::vector<ListDigraph::Node> order;
        TopologicalSort(order);

        COUT("Printing nodes in Topological order");
        for ( std::vector<ListDigraph::Node>::reverse_iterator it = order.rbegin(); it != order.rend(); ++it)
        {
            std::cout << name[*it] << std::endl;
        }
    }

// =========== pre179 asap

    void schedule_asap_(ListDigraph::NodeMap<size_t> & cycle, std::vector<ListDigraph::Node> & order)
    {
        DOUT("Performing ASAP Scheduling");
        TopologicalSort(order);

        std::vector<ListDigraph::Node>::reverse_iterator currNode = order.rbegin();
        cycle[*currNode]=0; // src dummy in cycle 0
        ++currNode;
        while(currNode != order.rend() )
        {
            size_t currCycle=0;
            // DOUT("Scheduling " << name[*currNode]);
            for( ListDigraph::InArcIt arc(graph,*currNode); arc != INVALID; ++arc )
            {
                ListDigraph::Node srcNode  = graph.source(arc);
                size_t srcCycle = cycle[srcNode];
                if(currCycle < (srcCycle + weight[arc]))
                {
                    currCycle = srcCycle + weight[arc];
                }
            }
            cycle[*currNode]=currCycle;
            ++currNode;
        }

        DOUT("Performing ASAP Scheduling [Done].");
    }

    // with rc and latency compensation
    void schedule_asap_( ListDigraph::NodeMap<size_t> & cycle, std::vector<ListDigraph::Node> & order,
                       ql::arch::resource_manager_t & rm, const ql::quantum_platform & platform)
    {
        DOUT("Performing RC ASAP Scheduling");
        TopologicalSort(order);

        std::vector<ListDigraph::Node>::reverse_iterator currNode = order.rbegin();
        size_t currCycle=0;
        cycle[*currNode]=currCycle; // source node
        ++currNode;
        while(currNode != order.rend() )
        {
            DOUT("");
            auto & curr_ins = instruction[*currNode];
            auto & id = curr_ins->name;
            COUT("id: " << id);

            size_t op_start_cycle=0;
            DOUT("Scheduling " << name[*currNode]);
            for( ListDigraph::InArcIt arc(graph,*currNode); arc != INVALID; ++arc )
            {
                ListDigraph::Node srcNode  = graph.source(arc);
                size_t srcCycle = cycle[srcNode];
                if(op_start_cycle < (srcCycle + weight[arc]))
                {
                    op_start_cycle = srcCycle + weight[arc];
                }
            }

            if(curr_ins->type() == ql::gate_type_t::__dummy_gate__ ||
               curr_ins->type() == ql::gate_type_t::__classical_gate__)
            {
                cycle[*currNode]=op_start_cycle;
            }
            else
            {
                std::string operation_name(id);
                std::string operation_type; // MW/FLUX/READOUT etc
                std::string instruction_type; // single / two qubit
                size_t operation_duration = std::ceil( static_cast<float>(curr_ins->duration) / cycle_time);

                if(platform.instruction_settings.count(id) > 0)
                {
                    COUT("New count logic, Found " << id);
                    if(platform.instruction_settings[id].count("cc_light_instr") > 0)
                    {
                        operation_name = platform.instruction_settings[id]["cc_light_instr"];
                    }
                    if(platform.instruction_settings[id].count("type") > 0)
                    {
                        operation_type = platform.instruction_settings[id]["type"];
                    }
                    if(platform.instruction_settings[id].count("cc_light_instr_type") > 0)
                    {
                        instruction_type = platform.instruction_settings[id]["cc_light_instr_type"];
                    }
                }

                while(op_start_cycle < MAX_CYCLE)
                {
                    DOUT("Trying to schedule: " << name[*currNode] << "  in cycle: " << op_start_cycle);
                    DOUT("current operation_duration: " << operation_duration);
                    if( rm.available(op_start_cycle, curr_ins, operation_name, operation_type, instruction_type, operation_duration) )
                    {
                        DOUT("Resources available at cycle " << op_start_cycle << ", Scheduled.");

                        rm.reserve(op_start_cycle, curr_ins, operation_name, operation_type, instruction_type, operation_duration);
                        cycle[*currNode]=op_start_cycle;
                        break;
                    }
                    else
                    {
                        DOUT("Resources not available at cycle " << op_start_cycle << ", trying again ...");
                        ++op_start_cycle;
                    }
                }

                if(op_start_cycle >= MAX_CYCLE)
                {
                    EOUT("Error: could not find schedule");
                    throw ql::exception("[x] Error : could not find schedule !",false);
                }
            }
            ++currNode;
        }

        // DOUT("Printing ASAP Schedule before latency compensation");
        // DOUT("Cycle    Instruction");
        // for ( auto it = order.rbegin(); it != order.rend(); ++it)
        // {
        //     DOUT( cycle[*it] << "  <- " << name[*it] );
        // }

        // latency compensation
        DOUT("Latency compensation ...");
        for ( auto it = order.begin(); it != order.end(); ++it)
        {
            auto & curr_ins = instruction[*it];
            auto & id = curr_ins->name;
            // DOUT("Latency compensating instruction: " << id);
            long latency_cycles=0;

            if(platform.instruction_settings.count(id) > 0)
            {
                if(platform.instruction_settings[id].count("latency") > 0)
                {
                    float latency_ns = platform.instruction_settings[id]["latency"];
                    latency_cycles = (std::ceil( static_cast<float>(std::abs(latency_ns)) / cycle_time)) *
                                            ql::utils::sign_of(latency_ns);
                }
            }
            cycle[*it] = cycle[*it] + latency_cycles;
            // DOUT( cycle[*it] << " <- " << name[*it] << latency_cycles );
        }

        COUT("Re-ordering ...");
        // re-order
        std::sort
            (
                order.begin(),
                order.end(),
                [&](ListDigraph::Node & n1, ListDigraph::Node & n2) { return cycle[n1] > cycle[n2]; }
            );

        // DOUT("Printing ASAP Schedule after latency compensation");
        // DOUT("Cycle    Instruction");
        // for ( auto it = order.rbegin(); it != order.rend(); ++it)
        // {
        //     DOUT( cycle[*it] << "        " << name[*it] );
        // }

        DOUT("Performing RC ASAP Scheduling [Done].");
    }

    // void PrintScheduleASAP()
    // {
    //     ListDigraph::NodeMap<size_t> cycle(graph);
    //     std::vector<ListDigraph::Node> order;
    //     schedule_asap_(cycle,order);

    //     COUT("\nPrinting ASAP Schedule");
    //     std::cout << "Cycle <- Instruction " << std::endl;
    //     std::vector<ListDigraph::Node>::reverse_iterator it;
    //     for ( it = order.rbegin(); it != order.rend(); ++it)
    //     {
    //         std::cout << cycle[*it] << "     <- " <<  name[*it] << std::endl;
    //     }
    // }

    // std::string GetDotScheduleASAP()
    // {
    //     stringstream dotout;
    //     ListDigraph::NodeMap<size_t> cycle(graph);
    //     std::vector<ListDigraph::Node> order;
    //     schedule_asap_(cycle,order);
    //     PrintDot1_(false,true,cycle,order,dotout);
    //     return dotout.str();
    // }

    // void PrintDotScheduleASAP()
    // {
    //     ofstream dotout;
    //     string dotfname( ql::options::get("output_dir") + "/scheduledASAP.dot");
    //     dotout.open( dotfname, ios::binary);
    //     if ( dotout.fail() )
    //     {
    //         EOUT("opening file " << dotfname << std::endl
    //                  << "Make sure the output directory ("<< ql::options::get("output_dir") << ") exists");
    //         return;
    //     }

    //     IOUT("Printing Scheduled Graph in " << dotfname);
    //     dotout << GetDotScheduleASAP();
    //     dotout.close();
    // }


    ql::ir::bundles_t schedule_asap_pre179()
    {
        DOUT("Scheduling ASAP to get bundles ...");
        ql::ir::bundles_t bundles;
        ListDigraph::NodeMap<size_t> cycle(graph);
        std::vector<ListDigraph::Node> order;
        schedule_asap_(cycle, order);

        typedef std::vector<ql::gate*> insInOneCycle;
        std::map<size_t,insInOneCycle> insInAllCycles;

        std::vector<ListDigraph::Node>::reverse_iterator rit;
        for ( rit = order.rbegin(); rit != order.rend(); ++rit)
        {
            if( instruction[*rit]->type() != ql::gate_type_t::__wait_gate__ )
                insInAllCycles[ cycle[*rit] ].push_back( instruction[*rit] );
        }

        size_t TotalCycles = 0;
        if( ! order.empty() )
        {
            // order.rbegin==SOURCE, order.begin==SINK
            TotalCycles =  cycle[ *( order.begin() ) ];
        }

        for(size_t currCycle=1; currCycle<TotalCycles; ++currCycle)
        {
            auto it = insInAllCycles.find(currCycle);
            ql::ir::bundle_t abundle;
            abundle.start_cycle = currCycle;
            size_t bduration = 0;
            if( it != insInAllCycles.end() )
            {
                auto nInsThisCycle = insInAllCycles[currCycle].size();
                for(size_t i=0; i<nInsThisCycle; ++i )
                {
                    ql::ir::section_t asec;
                    auto & ins = insInAllCycles[currCycle][i];
                    asec.push_back(ins);
                    abundle.parallel_sections.push_back(asec);
                    size_t iduration = ins->duration;
                    bduration = std::max(bduration, iduration);
                }
                abundle.duration_in_cycles = std::ceil(static_cast<float>(bduration)/cycle_time);
                bundles.push_back(abundle);
            }
        }
        if( ! order.empty() )
        {
            // Totalcycles == cycle[t] (i.e. of SINK), and includes SOURCE with its duration
            DOUT("Depth: " << TotalCycles-bundles.front().start_cycle);
        }
        else
        {
            DOUT("Depth: " << 0);
        }

        DOUT("Scheduling ASAP to get bundles [DONE]");
        return bundles;
    }



    // the following with rc and buffer-buffer delays
    ql::ir::bundles_t schedule_asap_pre179(ql::arch::resource_manager_t & rm, const ql::quantum_platform & platform)
    {
        DOUT("RC Scheduling ASAP to get bundles ...");
        ql::ir::bundles_t bundles;
        ListDigraph::NodeMap<size_t> cycle(graph);
        std::vector<ListDigraph::Node> order;
        schedule_asap_(cycle, order, rm, platform);

        typedef std::vector<ql::gate*> insInOneCycle;
        std::map<size_t,insInOneCycle> insInAllCycles;

        std::vector<ListDigraph::Node>::iterator it;
        for ( it = order.begin(); it != order.end(); ++it)
        {
            if ( instruction[*it]->type() != ql::gate_type_t::__wait_gate__ &&
                 instruction[*it]->type() != ql::gate_type_t::__dummy_gate__
               )
            {
                insInAllCycles[ cycle[*it] ].push_back( instruction[*it] );
            }
        }

        size_t TotalCycles = 0;
        if( ! order.empty() )
        {
            // order.rbegin==SOURCE, order.begin==SINK
            TotalCycles =  cycle[ *( order.begin() ) ];
        }

        for(size_t currCycle = 0; currCycle<=TotalCycles; ++currCycle)
        {
            auto it = insInAllCycles.find(currCycle);
            if( it != insInAllCycles.end() )
            {
                ql::ir::bundle_t abundle;
                size_t bduration = 0;
                auto nInsThisCycle = insInAllCycles[currCycle].size();
                for(size_t i=0; i<nInsThisCycle; ++i )
                {
                    ql::ir::section_t aparsec;
                    auto & ins = insInAllCycles[currCycle][i];
                    aparsec.push_back(ins);
                    abundle.parallel_sections.push_back(aparsec);
                    size_t iduration = ins->duration;
                    bduration = std::max(bduration, iduration);
                }
                abundle.start_cycle = currCycle;
                abundle.duration_in_cycles = std::ceil(static_cast<float>(bduration)/cycle_time);
                bundles.push_back(abundle);
            }
        }
        if( ! order.empty() )
        {
            // Totalcycles == cycle[t] (i.e. of SINK), and includes SOURCE with its duration
            DOUT("Depth: " << TotalCycles-bundles.front().start_cycle);
        }
        else
        {
            DOUT("Depth: " << 0);
        }

        // insert buffer - buffer delays
        DOUT("buffer-buffer delay insertion ... ");
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
                            op_type = platform.instruction_settings[id]["type"];
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
                    DOUT("Considering buffer_" << op_prev << "_" << op_curr << ": " << temp_buf_cycles);
                    buffer_cycles = std::max(temp_buf_cycles, buffer_cycles);
                }
            }
            DOUT( "Inserting buffer : " << buffer_cycles);
            buffer_cycles_accum += buffer_cycles;
            abundle.start_cycle = abundle.start_cycle + buffer_cycles_accum;
            operations_prev_bundle = operations_curr_bundle;
        }

        DOUT("RC Scheduling ASAP to get bundles [DONE]");
        return bundles;
    }

// =========== pre179 alap

    void schedule_alap_(ListDigraph::NodeMap<size_t> & cycle, std::vector<ListDigraph::Node> & order)
    {
        DOUT("Performing ALAP Scheduling");
        TopologicalSort(order);

        std::vector<ListDigraph::Node>::iterator currNode = order.begin();
        cycle[*currNode]=MAX_CYCLE;
        ++currNode;
        while( currNode != order.end() )
        {
            // DOUT("Scheduling " << name[*currNode]);
            size_t currCycle=MAX_CYCLE;
            for( ListDigraph::OutArcIt arc(graph,*currNode); arc != INVALID; ++arc )
            {
                ListDigraph::Node targetNode  = graph.target(arc);
                size_t targetCycle = cycle[targetNode];
                if(currCycle > (targetCycle-weight[arc]) )
                {
                    currCycle = targetCycle - weight[arc];
                }
            }
            cycle[*currNode]=currCycle;
            ++currNode;
        }
        // DOUT("Printing ALAP Schedule");
        // DOUT("Cycle   Cycle-simplified    Instruction");
        // for ( auto it = order.begin(); it != order.end(); ++it)
        // {
        //     DOUT( cycle[*it] << " :: " << MAX_CYCLE-cycle[*it] << "  <- " << name[*it] );
        // }
        DOUT("Performing ALAP Scheduling [Done].");
    }

    // with rc and latency compensation
    void schedule_alap_( ListDigraph::NodeMap<size_t> & cycle, std::vector<ListDigraph::Node> & order,
                       ql::arch::resource_manager_t & rm, const ql::quantum_platform & platform)
    {
        DOUT("Performing RC ALAP Scheduling");

        TopologicalSort(order);

        std::vector<ListDigraph::Node>::iterator currNode = order.begin();
        cycle[*currNode]=MAX_CYCLE;          // sink node
        ++currNode;
        while(currNode != order.end() )
        {
            DOUT("");
            auto & curr_ins = instruction[*currNode];
            auto & id = curr_ins->name;

            size_t op_start_cycle=MAX_CYCLE;
            DOUT("Scheduling " << name[*currNode]);
            for( ListDigraph::OutArcIt arc(graph,*currNode); arc != INVALID; ++arc )
            {
                ListDigraph::Node targetNode  = graph.target(arc);
                size_t targetCycle = cycle[targetNode];
                if(op_start_cycle > (targetCycle - weight[arc]))
                {
                    op_start_cycle = targetCycle - weight[arc];
                }
            }

            if(curr_ins->type() == ql::gate_type_t::__dummy_gate__ ||
               curr_ins->type() == ql::gate_type_t::__classical_gate__)
            {
                cycle[*currNode]=op_start_cycle;
            }
            else
            {
                std::string operation_name(id);
                std::string operation_type; // MW/FLUX/READOUT etc
                std::string instruction_type; // single / two qubit
                size_t operation_duration = std::ceil( static_cast<float>(curr_ins->duration) / cycle_time);

                if(platform.instruction_settings.count(id) > 0)
                {
                    if(platform.instruction_settings[id].count("cc_light_instr") > 0)
                    {
                        operation_name = platform.instruction_settings[id]["cc_light_instr"];
                    }
                    if(platform.instruction_settings[id].count("type") > 0)
                    {
                        operation_type = platform.instruction_settings[id]["type"];
                    }
                    if(platform.instruction_settings[id].count("cc_light_instr_type") > 0)
                    {
                        instruction_type = platform.instruction_settings[id]["cc_light_instr_type"];
                    }
                }

                while(op_start_cycle > 0)
                {
                    DOUT("Trying to schedule: " << name[*currNode] << "  in cycle: " << op_start_cycle);
                    DOUT("current operation_duration: " << operation_duration);
                    if( rm.available(op_start_cycle, curr_ins, operation_name, operation_type, instruction_type, operation_duration) )
                    {
                        DOUT("Resources available at cycle " << op_start_cycle << ", Scheduled.");

                        rm.reserve(op_start_cycle, curr_ins, operation_name, operation_type, instruction_type, operation_duration);
                        cycle[*currNode]=op_start_cycle;
                        break;
                    }
                    else
                    {
                        DOUT("Resources not available at cycle " << op_start_cycle << ", trying again ...");
                        --op_start_cycle;
                    }
                }
                if(op_start_cycle <= 0)
                {
                    EOUT("Error: could not find schedule");
                    throw ql::exception("[x] Error : could not find schedule !",false);
                }
            }
            ++currNode;
        }

        // DOUT("Printing ALAP Schedule before latency compensation");
        // DOUT("Cycle   Cycle-simplified    Instruction");
        // for ( auto it = order.begin(); it != order.end(); ++it)
        // {
        //     DOUT( cycle[*it] << " :: " << MAX_CYCLE-cycle[*it] << "  <- " << name[*it] );
        // }

        // latency compensation
        for ( auto it = order.begin(); it != order.end(); ++it)
        {
            auto & curr_ins = instruction[*it];
            auto & id = curr_ins->name;
            long latency_cycles=0;
            if(platform.instruction_settings.count(id) > 0)
            {
                if(platform.instruction_settings[id].count("latency") > 0)
                {
                    float latency_ns = platform.instruction_settings[id]["latency"];
                    latency_cycles = (std::ceil( static_cast<float>(std::abs(latency_ns)) / cycle_time)) *
                                            ql::utils::sign_of(latency_ns);
                }
            }
            cycle[*it] = cycle[*it] + latency_cycles;
            // DOUT( cycle[*it] << " <- " << name[*it] << latency_cycles );
        }

        // re-order
        std::sort
            (
                order.begin(),
                order.end(),
                [&](ListDigraph::Node & n1, ListDigraph::Node & n2) { return cycle[n1] > cycle[n2]; }
            );

        // DOUT("Printing ALAP Schedule after latency compensation");
        // DOUT("Cycle    Instruction");
        // for ( auto it = order.begin(); it != order.end(); ++it)
        // {
        //     DOUT( cycle[*it] << "        " << name[*it] );
        // }

        DOUT("Performing RC ALAP Scheduling [Done].");
    }

    // void PrintScheduleALAP()
    // {
    //     ListDigraph::NodeMap<size_t> cycle(graph);
    //     std::vector<ListDigraph::Node> order;
    //     schedule_alap_(cycle,order);

    //     COUT("\nPrinting ALAP Schedule");
    //     std::cout << "Cycle <- Instruction " << std::endl;
    //     std::vector<ListDigraph::Node>::reverse_iterator it;
    //     for ( it = order.rbegin(); it != order.rend(); ++it)
    //     {
    //         std::cout << MAX_CYCLE-cycle[*it] << "     <- " <<  name[*it] << std::endl;
    //     }
    // }

    // void PrintDot2_(
    //             bool WithCritical,
    //             bool WithCycles,
    //             ListDigraph::NodeMap<size_t> & cycle,
    //             std::vector<ListDigraph::Node> & order,
    //             std::ostream& dotout
    //             )
    // {
    //     ListDigraph::ArcMap<bool> isInCritical(graph);
    //     if(WithCritical)
    //     {
    //         for (ListDigraph::ArcIt a(graph); a != INVALID; ++a)
    //         {
    //             isInCritical[a] = false;
    //             for ( Path<ListDigraph>::ArcIt ap(p); ap != INVALID; ++ap )
    //             {
    //                 if(a==ap)
    //                 {
    //                     isInCritical[a] = true;
    //                     break;
    //                 }
    //             }
    //         }
    //     }

    //     string NodeStyle(" fontcolor=black, style=filled, fontsize=16");
    //     string EdgeStyle1(" color=black");
    //     string EdgeStyle2(" color=red");
    //     string EdgeStyle = EdgeStyle1;

    //     dotout << "digraph {\ngraph [ rankdir=TD; ]; // or rankdir=LR"
    //         << "\nedge [fontsize=16, arrowhead=vee, arrowsize=0.5];"
    //         << endl;

    //     // first print the nodes
    //     for (ListDigraph::NodeIt n(graph); n != INVALID; ++n)
    //     {
    //         int nid = graph.id(n);
    //         string nodeName = name[n];
    //         dotout  << "\"" << nid << "\""
    //                 << " [label=\" " << nodeName <<" \""
    //                 << NodeStyle
    //                 << "];" << endl;
    //     }

    //     if( WithCycles)
    //     {
    //         // Print cycle numbers as timeline, as shown below
    //         size_t cn=0,TotalCycles = MAX_CYCLE - cycle[ *( order.rbegin() ) ];
    //         dotout << "{\nnode [shape=plaintext, fontsize=16, fontcolor=blue]; \n";

    //         for(cn=0;cn<=TotalCycles;++cn)
    //         {
    //             if(cn>0)
    //                 dotout << " -> ";
    //             dotout << "Cycle" << cn;
    //         }
    //         dotout << ";\n}\n";

    //         // Now print ranks, as shown below
    //         std::vector<ListDigraph::Node>::reverse_iterator rit;
    //         for ( rit = order.rbegin(); rit != order.rend(); ++rit)
    //         {
    //             int nid = graph.id(*rit);
    //             dotout << "{ rank=same; Cycle" << TotalCycles - (MAX_CYCLE - cycle[*rit]) <<"; " <<nid<<"; }\n";
    //         }
    //     }

    //     // now print the edges
    //     for (ListDigraph::ArcIt arc(graph); arc != INVALID; ++arc)
    //     {
    //         ListDigraph::Node srcNode = graph.source(arc);
    //         ListDigraph::Node dstNode = graph.target(arc);
    //         int srcID = graph.id( srcNode );
    //         int dstID = graph.id( dstNode );

    //         if(WithCritical)
    //             EdgeStyle = ( isInCritical[arc]==true ) ? EdgeStyle2 : EdgeStyle1;

    //         dotout << dec
    //             << "\"" << srcID << "\""
    //             << "->"
    //             << "\"" << dstID << "\""
    //             << "[ label=\""
    //             << "q" << cause[arc]
    //             << " , " << weight[arc]
    //             << " , " << DepTypesNames[ depType[arc] ]
    //             <<"\""
    //             << " " << EdgeStyle << " "
    //             << "]"
    //             << endl;
    //     }

    //     dotout << "}" << endl;
    // }

    // void PrintDotScheduleALAP()
    // {
    //     ofstream dotout;
    //     string dotfname(ql::options::get("output_dir") + "/scheduledALAP.dot");
    //     dotout.open( dotfname, ios::binary);
    //     if ( dotout.fail() )
    //     {
    //         EOUT("Error opening file " << dotfname << std::endl
    //                  << "Make sure the output directory ("<< ql::options::get("output_dir") << ") exists");
    //         return;
    //     }

    //     IOUT("Printing Scheduled Graph in " << dotfname);
    //     ListDigraph::NodeMap<size_t> cycle(graph);
    //     std::vector<ListDigraph::Node> order;
    //     schedule_alap_(cycle,order);
    //     PrintDot2_(false,true,cycle,order,dotout);

    //     dotout.close();
    // }

    // std::string GetDotScheduleALAP()
    // {
    //     stringstream dotout;
    //     ListDigraph::NodeMap<size_t> cycle(graph);
    //     std::vector<ListDigraph::Node> order;
    //     schedule_alap_(cycle,order);
    //     PrintDot2_(false,true,cycle,order,dotout);
    //     return dotout.str();
    // }


    // the following without rc and buffer-buffer delays
    ql::ir::bundles_t schedule_alap_pre179()
    {
        DOUT("Scheduling ALAP to get bundles ...");
        ql::ir::bundles_t bundles;
        ListDigraph::NodeMap<size_t> cycle(graph);
        std::vector<ListDigraph::Node> order;
        schedule_alap_(cycle,order);

        typedef std::vector<ql::gate*> insInOneCycle;
        std::map<size_t,insInOneCycle> insInAllCycles;

        std::vector<ListDigraph::Node>::iterator it;
        for ( it = order.begin(); it != order.end(); ++it)
        {
            if( instruction[*it]->type() != ql::gate_type_t::__wait_gate__ )
                insInAllCycles[ MAX_CYCLE - cycle[*it] ].push_back( instruction[*it] );
        }

        size_t TotalCycles = 0;
        if( ! order.empty() )
        {
            // order.rbegin==SOURCE, order.begin==SINK
            TotalCycles =  MAX_CYCLE - cycle[ *( order.rbegin() ) ];
        }

        for(size_t currCycle = TotalCycles-1; currCycle>0; --currCycle)
        {
            auto it = insInAllCycles.find(currCycle);
            ql::ir::bundle_t abundle;
            abundle.start_cycle = TotalCycles - currCycle;
            size_t bduration = 0;
            if( it != insInAllCycles.end() )
            {
                auto nInsThisCycle = insInAllCycles[currCycle].size();
                for(size_t i=0; i<nInsThisCycle; ++i )
                {
                    ql::ir::section_t asec;
                    auto & ins = insInAllCycles[currCycle][i];
                    asec.push_back(ins);
                    abundle.parallel_sections.push_back(asec);
                    size_t iduration = ins->duration;
                    bduration = std::max(bduration, iduration);
                }
                abundle.duration_in_cycles = std::ceil(static_cast<float>(bduration)/cycle_time);
                bundles.push_back(abundle);
            }
        }
        if( ! order.empty() )
        {
            // Totalcycles == MAX_CYCLE-(MAX_CYCLE-cycle[s]) (i.e. of SOURCE) and includes SOURCE with duration 1
            DOUT("Depth: " << TotalCycles-bundles.front().start_cycle);
        }
        else
        {
            DOUT("Depth: " << 0);
        }

        DOUT("Scheduling ALAP to get bundles [DONE]");
        return bundles;
    }

    // the following with rc and buffer-buffer delays
    ql::ir::bundles_t schedule_alap_pre179(ql::arch::resource_manager_t & rm, 
        const ql::quantum_platform & platform)
    {
        DOUT("RC Scheduling ALAP to get bundles ...");
        ql::ir::bundles_t bundles;
        ListDigraph::NodeMap<size_t> cycle(graph);
        std::vector<ListDigraph::Node> order;
        schedule_alap_(cycle, order, rm, platform);

        typedef std::vector<ql::gate*> insInOneCycle;
        std::map<size_t,insInOneCycle> insInAllCycles;

        std::vector<ListDigraph::Node>::iterator it;
        for ( it = order.begin(); it != order.end(); ++it)
        {
            if ( instruction[*it]->type() != ql::gate_type_t::__wait_gate__ &&
                 instruction[*it]->type() != ql::gate_type_t::__dummy_gate__
               )
            {
                insInAllCycles[ MAX_CYCLE - cycle[*it] ].push_back( instruction[*it] );
            }
        }

        size_t TotalCycles = 0;
        if( ! order.empty() )
        {
            TotalCycles =  MAX_CYCLE - cycle[ *( order.rbegin() ) ];
        }

        for(size_t currCycle = TotalCycles-1; currCycle>0; --currCycle)
        {
            auto it = insInAllCycles.find(currCycle);
            ql::ir::bundle_t abundle;
            abundle.start_cycle = TotalCycles - currCycle;
            size_t bduration = 0;
            if( it != insInAllCycles.end() )
            {
                auto nInsThisCycle = insInAllCycles[currCycle].size();
                for(size_t i=0; i<nInsThisCycle; ++i )
                {
                    ql::ir::section_t asec;
                    auto & ins = insInAllCycles[currCycle][i];
                    asec.push_back(ins);
                    abundle.parallel_sections.push_back(asec);
                    size_t iduration = ins->duration;
                    bduration = std::max(bduration, iduration);
                }
                abundle.duration_in_cycles = std::ceil(static_cast<float>(bduration)/cycle_time);
                bundles.push_back(abundle);
            }
        }
        if( ! order.empty() )
        {
            // Totalcycles == MAX_CYCLE-(MAX_CYCLE-cycle[s]) (i.e. of SOURCE) and includes SOURCE with duration 1
            DOUT("Depth: " << TotalCycles-bundles.front().start_cycle);
        }
        else
        {
            DOUT("Depth: " << 0);
        }

        // insert buffer - buffer delays
        DOUT("buffer-buffer delay insertion ... ");
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
                            op_type = platform.instruction_settings[id]["type"];
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
                    DOUT("Considering buffer_" << op_prev << "_" << op_curr << ": " << temp_buf_cycles);
                    buffer_cycles = std::max(temp_buf_cycles, buffer_cycles);
                }
            }
            DOUT( "Inserting buffer : " << buffer_cycles);
            buffer_cycles_accum += buffer_cycles;
            abundle.start_cycle = abundle.start_cycle + buffer_cycles_accum;
            operations_prev_bundle = operations_curr_bundle;
        }

        DOUT("RC Scheduling ALAP to get bundles [DONE]");
        return bundles;
    }


// =========== pre179 uniform
    void compute_alap_cycle(ListDigraph::NodeMap<size_t> & cycle, std::vector<ListDigraph::Node> & order, size_t max_cycle)
    {
        // DOUT("Computing alap_cycle");
        std::vector<ListDigraph::Node>::iterator currNode = order.begin();
        cycle[*currNode]=max_cycle;
        ++currNode;
        while( currNode != order.end() )
        {
            // DOUT("Scheduling " << name[*currNode]);
            size_t currCycle=max_cycle;
            for( ListDigraph::OutArcIt arc(graph,*currNode); arc != INVALID; ++arc )
            {
                ListDigraph::Node targetNode  = graph.target(arc);
                size_t targetCycle = cycle[targetNode];
                if(currCycle > (targetCycle-weight[arc]) )
                {
                    currCycle = targetCycle - weight[arc];
                }
            }
            cycle[*currNode]=currCycle;
            ++currNode;
        }
    }

    void compute_asap_cycle(ListDigraph::NodeMap<size_t> & cycle, std::vector<ListDigraph::Node> & order)
    {
        // DOUT("Computing asap_cycle");
        std::vector<ListDigraph::Node>::reverse_iterator currNode = order.rbegin();
        cycle[*currNode]=0; // src dummy in cycle 0
        ++currNode;
        while(currNode != order.rend() )
        {
            size_t currCycle=0;
            // DOUT("Scheduling " << name[*currNode]);
            for( ListDigraph::InArcIt arc(graph,*currNode); arc != INVALID; ++arc )
            {
                ListDigraph::Node srcNode  = graph.source(arc);
                size_t srcCycle = cycle[srcNode];
                if(currCycle < (srcCycle + weight[arc]))
                {
                    currCycle = srcCycle + weight[arc];
                }
            }
            cycle[*currNode]=currCycle;
            ++currNode;
        }
    }


    void schedule_alap_uniform_(ListDigraph::NodeMap<size_t> & cycle, std::vector<ListDigraph::Node> & order)
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
        // After this, it moves nodes up at most to their ALAP cycle to fill small bundles to the targeted uniform length.
        // It does this in a backward scan (as ALAP scheduling would do), so bundles at the highest cycles are filled first.
        // Hence, the result resembles an ALAP schedule with excess bundle lengths solved by moving nodes down ("dough rolling").

        DOUT("Performing ALAP UNIFORM Scheduling");
        // order becomes a reversed topological order of the nodes
        // don't know why it is done, since the nodes already are in topological order
        // that they are is a consequence of dep graph computation which is based on this original order
        TopologicalSort(order);

        // compute cycle itself as asap as first approximation of result
        // when schedule is already uniform, nothing changes
        // this actually is schedule_asap_(cycle, order) but without topological sort
        compute_asap_cycle(cycle, order);
        size_t   cycle_count = cycle[*( order.begin() )];// order is reversed asap, so starts at cycle_count

        // compute alap_cycle
        // when making asap bundles uniform in size in backward scan
        // fill them by moving instructions from earlier bundles (lower cycle values)
        // prefer to move those with highest alap (because that maximizes freedom)
        ListDigraph::NodeMap<size_t> alap_cycle(graph);
        compute_alap_cycle(alap_cycle, order, cycle_count);

        // DOUT("Creating nodes_per_cycle");
        // create nodes_per_cycle[cycle] = for each cycle the list of nodes at cycle cycle
        // this is the basic map to be operated upon by the uniforming scheduler below;
        // gate_count is computed to compute the target bundle size later
        std::map<size_t,std::list<ListDigraph::Node>> nodes_per_cycle;
        std::vector<ListDigraph::Node>::iterator it;
        for ( it = order.begin(); it != order.end(); ++it)
        {
            nodes_per_cycle[ cycle[*it] ].push_back( *it );
        }

        // DOUT("Displaying circuit and bundle statistics");
        // to compute how well the algorithm is doing, two measures are computed:
        // - the largest number of gates in a cycle in the circuit,
        // - and the average number of gates in non-empty cycles
        // this is done before and after uniform scheduling, and printed
        size_t max_gates_per_cycle = 0;
        size_t non_empty_bundle_count = 0;
        size_t gate_count = 0;
        for (size_t curr_cycle = 0; curr_cycle != cycle_count; curr_cycle++)
        {
            max_gates_per_cycle = std::max(max_gates_per_cycle, nodes_per_cycle[curr_cycle].size());
            if (int(nodes_per_cycle[curr_cycle].size()) != 0) non_empty_bundle_count++;
            gate_count += nodes_per_cycle[curr_cycle].size();
        }
        double avg_gates_per_cycle = double(gate_count)/cycle_count;
        double avg_gates_per_non_empty_cycle = double(gate_count)/non_empty_bundle_count;
        IOUT("... before uniform scheduling:"
            << " cycle_count=" << cycle_count
            << "; gate_count=" << gate_count
            << "; non_empty_bundle_count=" << non_empty_bundle_count
            );
        IOUT("... and max_gates_per_cycle=" << max_gates_per_cycle
            << "; avg_gates_per_cycle=" << avg_gates_per_cycle
            << "; ..._per_non_empty_cycle=" << avg_gates_per_non_empty_cycle
            );

        // backward make bundles max avg_gates_per_cycle long
        // DOUT("Backward scan uniform scheduling ILP");
        for (size_t curr_cycle = cycle_count-1; curr_cycle != 0; curr_cycle--)    // QUESTION: gate at cycle 0?
        {
            // Backward with pred_cycle from curr_cycle-1, look for node(s) to extend current too small bundle.
            // This assumes that current bundle is never too long, excess having been moved away earlier.
            // When such a node cannot be found, this loop scans the whole circuit for each original node to extend
            // and this creates a O(n^2) time complexity.
            //
            // A test to break this prematurely based on the current data structure, wasn't devised yet.
            // A sulution is to use the dep graph instead to find a node to extend the current node with,
            // i.e. maintain a so-called "heap" of nodes free to schedule, as in conventional scheduling algorithms,
            // which is not hard at all but which is not according to the published algorithm.
            // When the complexity becomes a problem, it is proposed to rewrite the algorithm accordingly.

            long pred_cycle = curr_cycle - 1;    // signed because can become negative

            // target size of each bundle is number of gates to go divided by number of non-empty cycles to go
            // it averages over non-empty bundles instead of all bundles because the latter would be very strict
            // it is readjusted to cater for dips in bundle size caused by local dependence chains
            if (non_empty_bundle_count == 0) break;
            avg_gates_per_cycle = double(gate_count)/curr_cycle;
            avg_gates_per_non_empty_cycle = double(gate_count)/non_empty_bundle_count;
            DOUT("Cycle=" << curr_cycle << " number of gates=" << nodes_per_cycle[curr_cycle].size()
                << "; avg_gates_per_cycle=" << avg_gates_per_cycle
                << "; ..._per_non_empty_cycle=" << avg_gates_per_non_empty_cycle);

            while ( double(nodes_per_cycle[curr_cycle].size()) < avg_gates_per_non_empty_cycle && pred_cycle >= 0 )
            {
                size_t          max_alap_cycle = 0;
                ListDigraph::Node best_n;
                bool          best_n_found = false;

                // scan bundle at pred_cycle to find suitable candidate to move forward to curr_cycle
                for ( auto n : nodes_per_cycle[pred_cycle] )
                {
                    bool          forward_n = true;
                    size_t          n_completion_cycle;

                    // candidate's result, when moved, must be ready before end-of-circuit and before used
                    n_completion_cycle = curr_cycle + std::ceil(static_cast<float>(instruction[n]->duration)/cycle_time);
                    if (n_completion_cycle > cycle_count)
                    {
                        forward_n = false;
                    }
                    for ( ListDigraph::OutArcIt arc(graph,n); arc != INVALID; ++arc )
                    {
                        ListDigraph::Node targetNode  = graph.target(arc);
                        size_t targetCycle = cycle[targetNode];
                        if(n_completion_cycle > targetCycle)
                        {
                            forward_n = false;
                        }
                    }

                    // when multiple nodes in bundle qualify, take the one with highest alap cycle
                    if (forward_n && alap_cycle[n] > max_alap_cycle)
                    {
                        max_alap_cycle = alap_cycle[n];
                        best_n_found = true;
                        best_n = n;
                    }
                }

                // when candidate was found in this bundle, move it, and search for more in this bundle, if needed
                // otherwise, continue scanning backward
                if (best_n_found)
                {
                    nodes_per_cycle[pred_cycle].remove(best_n);
                    if (nodes_per_cycle[pred_cycle].size() == 0)
                    {
                        // bundle was non-empty, now it is empty
                        non_empty_bundle_count--;
                    }
                    if (nodes_per_cycle[curr_cycle].size() == 0)
                    {
                        // bundle was empty, now it will be non_empty
                        non_empty_bundle_count++;
                    }
                    cycle[best_n] = curr_cycle;
                    nodes_per_cycle[curr_cycle].push_back(best_n);
                    if (non_empty_bundle_count == 0) break;
                    avg_gates_per_cycle = double(gate_count)/curr_cycle;
                    avg_gates_per_non_empty_cycle = double(gate_count)/non_empty_bundle_count;
                    DOUT("... moved " << name[best_n] << " with alap=" << alap_cycle[best_n]
                        << " from cycle=" << pred_cycle << " to cycle=" << curr_cycle
                        << "; new avg_gates_per_cycle=" << avg_gates_per_cycle
                        << "; ..._per_non_empty_cycle=" << avg_gates_per_non_empty_cycle
                        );
                }
                else
                {
                    pred_cycle --;
                }
            }   // end for finding a bundle to forward a node from to the current cycle

            // curr_cycle ready, mask it from the counts and recompute counts for remaining cycles
            gate_count -= nodes_per_cycle[curr_cycle].size();
            if (nodes_per_cycle[curr_cycle].size() != 0)
            {
                // bundle is non-empty
                non_empty_bundle_count--;
            }
        }   // end curr_cycle loop; curr_cycle is bundle which must be enlarged when too small

        // Recompute and print statistics reporting on uniform scheduling performance
        max_gates_per_cycle = 0;
        non_empty_bundle_count = 0;
        gate_count = 0;
        // cycle_count was not changed
        for (size_t curr_cycle = 0; curr_cycle != cycle_count; curr_cycle++)
        {
            max_gates_per_cycle = std::max(max_gates_per_cycle, nodes_per_cycle[curr_cycle].size());
            if (int(nodes_per_cycle[curr_cycle].size()) != 0) non_empty_bundle_count++;
            gate_count += nodes_per_cycle[curr_cycle].size();
        }
        avg_gates_per_cycle = double(gate_count)/cycle_count;
        avg_gates_per_non_empty_cycle = double(gate_count)/non_empty_bundle_count;
        IOUT("... after uniform scheduling:"
            << " cycle_count=" << cycle_count
            << "; gate_count=" << gate_count
            << "; non_empty_bundle_count=" << non_empty_bundle_count
            );
        IOUT("... and max_gates_per_cycle=" << max_gates_per_cycle
            << "; avg_gates_per_cycle=" << avg_gates_per_cycle
            << "; ..._per_non_empty_cycle=" << avg_gates_per_non_empty_cycle
            );

        DOUT("Performing ALAP UNIFORM Scheduling [DONE]");
    }

    ql::ir::bundles_t schedule_alap_uniform_pre179()
    {
        DOUT("Scheduling ALAP UNIFORM to get bundles ...");
        ql::ir::bundles_t bundles;
        ListDigraph::NodeMap<size_t> cycle(graph);
        std::vector<ListDigraph::Node> order;
        schedule_alap_uniform_(cycle, order);

        typedef std::vector<ql::gate*> insInOneCycle;
        std::map<size_t,insInOneCycle> insInAllCycles;

        std::vector<ListDigraph::Node>::reverse_iterator rit;
        for ( rit = order.rbegin(); rit != order.rend(); ++rit)
        {
            if( instruction[*rit]->type() != ql::gate_type_t::__wait_gate__ )
                insInAllCycles[ cycle[*rit] ].push_back( instruction[*rit] );
        }

        size_t TotalCycles = 0;
        if( ! order.empty() )
        {
            TotalCycles =  cycle[ *( order.begin() ) ];
        }

        for(size_t currCycle=1; currCycle<TotalCycles; ++currCycle)
        {
            auto it = insInAllCycles.find(currCycle);
            ql::ir::bundle_t abundle;
            abundle.start_cycle = currCycle;
            size_t bduration = 0;
            if( it != insInAllCycles.end() )
            {
                auto nInsThisCycle = insInAllCycles[currCycle].size();
                for(size_t i=0; i<nInsThisCycle; ++i )
                {
                    ql::ir::section_t asec;
                    auto & ins = insInAllCycles[currCycle][i];
                    asec.push_back(ins);
                    abundle.parallel_sections.push_back(asec);
                    size_t iduration = ins->duration;
                    bduration = std::max(bduration, iduration);
                }
                abundle.duration_in_cycles = std::ceil(static_cast<float>(bduration)/cycle_time);
                bundles.push_back(abundle);
            }
        }
        if( ! order.empty() )
        {
            DOUT("Depth: " << TotalCycles-bundles.front().start_cycle);
        }
        else
        {
            DOUT("Depth: " << 0);
        }

        DOUT("Scheduling ALAP UNIFORM to get bundles [DONE]");
        return bundles;
    }

// =========== post179 schedulers, plain, just ASAP and ALAP, no resources, etc.

// use MAX_CYCLE for absolute upperbound on cycle value
// use ALAP_SINK_CYCLE for initial cycle given to SINK in ALAP;
// the latter allows for some growing room when doing latency compensation/buffer-delay insertion
#define ALAP_SINK_CYCLE    (MAX_CYCLE/2)

    // cycle assignment without RC depending on direction: forward:ASAP, backward:ALAP;
    // without RC, this is all there is to schedule, apart from forming the bundles in bundler()
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
            // *circp is by definition in a topological order of the dependence graph
            for ( ql::circuit::iterator gpit = circp->begin(); gpit != circp->end(); gpit++)
            {
                set_cycle_gate(*gpit, dir);
            }
            set_cycle_gate(instruction[t], dir);
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
            for ( auto & gp : *circp)
            {
                gp->cycle -= SOURCECycle;
            }
            instruction[s]->cycle -= SOURCECycle;   // i.e. becomes 0
        }
    }

    // sort circuit by the gates' cycle attribute in non-decreasing order
    void sort_by_cycle()
    {
        DOUT("... before sorting on cycle value");
        for ( ql::circuit::iterator gpit = circp->begin(); gpit != circp->end(); gpit++)
        {
            ql::gate*           gp = *gpit;
            DOUT("...... (@" << gp->cycle << ") " << gp->qasm());
        }

#ifdef  USE_STANDARD_SORT
        std::sort(circp->begin(), circp->end(), [&](ql::gate* & gp1, ql::gate* & gp2) { return gp1->cycle < gp2->cycle; });

#else
        // std::sort doesn't preserve the original order when it is already ok
        // this is confusing while debugging but should not influence results
        //
        // the code below assumes that the original and the sorted sequence are not much different
        // and the sort is done by scanning the original and sorting it
        // in the new sequence by scanning the new one from the back to find the new place;
        // when the original is in order, this reduces to just appending each to the new sequence
        //
        // it is complicated by the fact that a circuit is a vector and not a list
        // so it doesn't support well insertion of a gate at an arbitrary place
        std::list<ql::gate*>   lg;  // target list
        for ( ql::circuit::iterator gpit = circp->begin(); gpit != circp->end(); gpit++)
        {
            ql::gate*  gp = *gpit;      // take gate by gate from original circuit circp
	        std::list<ql::gate*>::reverse_iterator rigp = lg.rbegin();
	        for (; rigp != lg.rend(); rigp++)
	        {
                // sort gate into target list from target list's end
	            if ((*rigp)->cycle <= gp->cycle)
	            {
	                // rigp.base() because insert doesn't work with reverse iteration
	                // rigp.base points after the element that rigp is pointing at
	                // which is lucky because insert only inserts before the given element
	                // the end effect is inserting after rigp
	                lg.insert(rigp.base(), gp);
	                break;
	            }
	        }
	        // when list was empty or no element was found, just put it in front
	        if (rigp == lg.rend())
	        {
	            lg.push_front(gp);
	        }
        }
        // and write the list back to the circuit
        circp->clear();
        for (auto & gp : lg)
        {
            circp->push_back(gp);
        }
#endif

        DOUT("... after sorting on cycle value");
        for ( ql::circuit::iterator gpit = circp->begin(); gpit != circp->end(); gpit++)
        {
            ql::gate*           gp = *gpit;
            DOUT("...... (@" << gp->cycle << ") " << gp->qasm());
        }
    }

    // return bundles for the given circuit;
    // assumes gatep->cycle attribute reflects the cycle assignment;
    // assumes circuit being a vector of gate pointers is ordered by this cycle value;
    // create bundles in a single scan over the circuit, using currBundle and currCycle as state
    ql::ir::bundles_t bundler(ql::circuit& circ)
    {
        ql::ir::bundles_t bundles;          // result bundles
    
        ql::ir::bundle_t    currBundle;     // current bundle at currCycle that is being filled
        size_t              currCycle = 0;  // cycle at which bundle is to be scheduled

        currBundle.start_cycle = currCycle; // starts off as empty bundle starting at currCycle
        currBundle.duration_in_cycles = 0;

        DOUT("bundler ...");

        for (auto & gp: circ)
        {
            DOUT(". adding gate(@" << gp->cycle << ")  " << gp->qasm());
            if ( gp->type() == ql::gate_type_t::__wait_gate__ ||
                 gp->type() == ql::gate_type_t::__dummy_gate__
               )
            {
                DOUT("... ignoring: " << gp->qasm());
                continue;
            }
            size_t newCycle = gp->cycle;        // taking cycle values from circuit, so excludes SOURCE and SINK!
            if (newCycle < currCycle)
            {
                EOUT("Error: circuit not ordered by cycle value");
                throw ql::exception("[x] Error: circuit not ordered by cycle value",false);
            }
            if (newCycle > currCycle)
            {
                if (!currBundle.parallel_sections.empty())
                {
                    // finish currBundle at currCycle
                    DOUT(".. bundle duration in cycles: " << currBundle.duration_in_cycles);
                    bundles.push_back(currBundle);
                    DOUT(".. ready with bundle");
                    currBundle.parallel_sections.clear();
                }

                // new empty currBundle at newCycle
                currCycle = newCycle;
                DOUT(".. bundling at cycle: " << currCycle);
                currBundle.start_cycle = currCycle;
                currBundle.duration_in_cycles = 0;
            }

            // add gp to currBundle
            ql::ir::section_t asec;
            asec.push_back(gp);
            currBundle.parallel_sections.push_back(asec);
            DOUT("... gate: " << gp->qasm() << " in private parallel section");
            currBundle.duration_in_cycles = std::max(currBundle.duration_in_cycles, (gp->duration+cycle_time-1)/cycle_time); 
        }
        if (!currBundle.parallel_sections.empty())
        {
            // finish currBundle (which is last bundle) at currCycle
            DOUT("... bundle duration in cycles: " << currBundle.duration_in_cycles);
            bundles.push_back(currBundle);
            DOUT("... ready with bundle");
        }

        // currCycle == cycle of last gate of circuit scheduled but we know that the first one got cycle 1
        DOUT("Depth: " << currCycle+currBundle.duration_in_cycles-bundles.front().start_cycle);
        DOUT("bundler [DONE]");
        return bundles;
    }

    // ASAP scheduler without RC, updating circuit and returning bundles
    ql::ir::bundles_t schedule_asap_post179()
    {
        DOUT("Scheduling ASAP post179 ...");
        set_cycle(ql::forward_scheduling);
        
        sort_by_cycle();

        DOUT("Scheduling ASAP [DONE]");
        return bundler(*circp);
    }

    // ALAP scheduler without RC, updating circuit and returning bundles
    ql::ir::bundles_t schedule_alap_post179()
    {
        DOUT("Scheduling ALAP post179 ...");
        set_cycle(ql::backward_scheduling);

        sort_by_cycle();

        DOUT("Scheduling ALAP [DONE]");
        return bundler(*circp);
    }


// =========== post179 schedulers with RC, latency compensation and buffer-buffer delay insertion
    // most code from here on deals with scheduling with Resource Constraints
    // then the cycles as assigned from the depgraph shift, because of resource conflicts
    // and then at each point all available nodes should be considered for scheduling
    // to avoid largely suboptimal results

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
                    latency_cycles = (std::ceil( static_cast<float>(std::abs(latency_ns)) / cycle_time)) *
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
                            op_type = platform.instruction_settings[id]["type"];
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

    // in critical-path scheduling, usually more critical instructions are preferred;
    // an instruction is more critical when its ASAP and ALAP values differ less;
    // when scheduling with resource constraints, the ideal ASAP/ALAP cycle values cannot
    // be attained because of resource conflicts being in the way, they will 'slip',
    // so actual cycle values cannot be compared anymore to ideal ASAP/ALAP values to compute criticality;
    // but the ideal ASAP/ALAP values can be compared mutually between gates as approximation:
    // when forward (backward) scheduling, a lower ALAP (higher ASAP) indicates more criticality;
    // those ALAP/ASAP are a measure for number of cycles still to fill with gates in the schedule,
    // and are coined 'remaining' cycles here;
    // remaining[node] indicates number of cycles remaining in schedule after start execution of node;
    // please note that for forward (backward) scheduling we use an
    // adaptation of the ALAP (ASAP) cycle computation to compute the remaining values; with this
    // definition both in forward and backward scheduling, a higher remaining indicates more criticality
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

    // ASAP/ALAP scheduling support code with RC
    // uses an "available list" (avlist) as interface between dependence graph and scheduler
    // the avlist contains all nodes that wrt their dependences can be scheduled:
    //  all its predecessors were scheduled (forward scheduling) or
    //  all its successors were scheduled (backward scheduling)
    // the scheduler fills cycles one by one, with nodes/instructions from the avlist
    // checking before selection whether the nodes/instructions have completed execution
    // and whether the resource constraints are fulfilled

    // avlist support

    // initialize avlist to single starting node
    // forward scheduling:
    //  node s (with SOURCE instruction) is the top of the dependence graph; all instructions depend on it
    // backward scheduling:
    //  node t (with SINK instruction) is the bottom of the dependence graph; it depends on all instructions
    // set the curr_cycle of the scheduling algorithm to start at the appropriate end as well;
    // note that the cycle attributes will be shifted down to start at 1 after backward scheduling
    void InitAvailable(std::list<ListDigraph::Node>& avlist, ql::scheduling_direction_t dir, size_t& curr_cycle)
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

    // make node n available
    // add it to the avlist because the condition for that is fulfilled:
    //  all its predecessors were scheduled (forward scheduling) or
    //  all its successors were scheduled (backward scheduling)
    // update its cycle attribute to reflect these dependences;
    // n cannot be s or t; s and t are added to the avlist by InitAvailable
    void MakeAvailable(ListDigraph::Node n, std::list<ListDigraph::Node>& avlist, ql::scheduling_direction_t dir)
    {
        set_cycle_gate(instruction[n], dir);
        avlist.push_back(n);
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
    void TakeFromAvailable(ListDigraph::Node n, std::list<ListDigraph::Node>& avlist, ListDigraph::NodeMap<bool> & scheduled, ql::scheduling_direction_t dir)
    {
        scheduled[n] = true;
        avlist.remove(n);
        if (ql::forward_scheduling == dir)
        {
            // the order of the arcs (i.e. dependences) in the depgraph are not in gate input order
            // they are in a reverse order; without additional action, in forward scheduling
            // the succNodes are made available in reverse gate input order
            // resulting in a later gate with otherwise equal attributes being preferred
            // over an earlier one while the original scheduler code had the non-reversed gate order;
            // since there is no reverse iterator over the arcs,
            // kludge here to undo the reversal explicitly;
            // for backward scheduler this reversed order is ok
            std::list<ListDigraph::Node>   sln;
            for (ListDigraph::OutArcIt succArc(graph,n); succArc != INVALID; ++succArc)
            {
                ListDigraph::Node succNode = graph.target(succArc);
                bool schedulable = true;
                for (ListDigraph::InArcIt predArc(graph,succNode); predArc != INVALID; ++predArc)
                {
                    ListDigraph::Node predNode = graph.source(predArc);
                    if (!scheduled[predNode])
                    {
                        schedulable = false;
                        break;
                    }
                }
                if (schedulable)
                {
                    sln.push_front(succNode);   // build list from front, i.e. reverse
                }
            }
            for (auto sn : sln)
            {
                MakeAvailable(sn, avlist, dir);
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
                    if (!scheduled[succNode])
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
    // and try again; this makes nodes/instructions to complete execution,
    // and makes resources available in case of resource constrained scheduling
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


    // reading platform dependent gate attributes for rc scheduling
    //
    // get the gate parameters that need to be passed to the resource manager;
    // it would have been nicer if they would have been made available by the platform
    // directly to the resource manager since this function makes the mapper dependent on cc_light
    void GetGateParameters(std::string id, const ql::quantum_platform& platform, std::string& operation_name, std::string& operation_type, std::string& instruction_type)
    {
        if (platform.instruction_settings.count(id) > 0)
        {
	        if ( !platform.instruction_settings[id]["cc_light_instr"].is_null() )
	        {
	            operation_name = platform.instruction_settings[id]["cc_light_instr"];
	        }
	        if ( !platform.instruction_settings[id]["type"].is_null() )
	        {
	            operation_type = platform.instruction_settings[id]["type"];
	        }
	        if ( !platform.instruction_settings[id]["cc_light_instr_type"].is_null() )
	        {
	            instruction_type = platform.instruction_settings[id]["cc_light_instr_type"];
	        }
        }
        else
        {
            EOUT("Error: platform doesn't support gate '" << id << "'");
            throw ql::exception("[x] Error : platform doesn't support gate!",false);
        }
    }

    // find the avg of remaining[] of the depending nodes of n;
    // rationale for this heuristic:
    // when two nodes are equally critical then prefer the node for scheduling
    // of which the subsequent nodes ('depending nodes') are most critical;
    // because there may not be just one depending node but multiple of them, and not
    // an equal number for each of the competing most critical nodes,
    // we take the average of the criticality (== remaining) of those depending nodes;
    // we first filter out the duplicates since these would otherwise dominate the average;
    // the one with the highest average wins and will be scheduled first
    double avg_remaining_deps(ListDigraph::Node n, ql::scheduling_direction_t dir)
    {
        std::list<ListDigraph::Node>   ln;
        if (ql::forward_scheduling == dir)
        {
            for (ListDigraph::OutArcIt succArc(graph,n); succArc != INVALID; ++succArc)
            {
                ListDigraph::Node succNode = graph.target(succArc);
                DOUT("...... succ of " << instruction[n]->qasm() << " : " << instruction[succNode]->qasm());
                bool found = false;             // filter out duplicates
                for ( auto anySuccNode : ln )
                {
                    if (succNode == anySuccNode)
                    {
                        DOUT("...... duplicate: " << instruction[succNode]->qasm());
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
                DOUT("...... pred of " << instruction[n]->qasm() << " : " << instruction[predNode]->qasm());
                bool found = false;             // filter out duplicates
                for ( auto anyPredNode : ln )
                {
                    if (predNode == anyPredNode)
                    {
                        DOUT("...... duplicate: " << instruction[predNode]->qasm());
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
        double  depremainingsum = 0.0;
        for ( auto sn : ln)
        {
            DOUT("...... adding remaining of " << instruction[sn]->qasm() << " : " << remaining[sn]);
            depremainingsum += remaining[sn];
        }
        DOUT("...... number of depending nodes: " << ln.size());
        double depremainingavg = depremainingsum / ln.size();
        DOUT("...... depremainingavg of " << instruction[n]->qasm() << " : " << depremainingavg);
        return depremainingavg;
    }

    // select a node from the avlist
    ListDigraph::Node select(std::list<ListDigraph::Node>& avlist, ql::scheduling_direction_t dir, const size_t curr_cycle,
                                const ql::quantum_platform& platform, ql::arch::resource_manager_t& rm, bool & success)
    {
        success = false;                        // whether a node was found and returned
        ListDigraph::Node   selected_node = s;  // node to be returned; fake value to hush gcc
        
        // avlist is distributed over following lists
        std::list<ListDigraph::Node>   nodes_critical_waiting;  // nodes critical but waiting for operand or conflict
        std::list<ListDigraph::Node>   nodes_waiting;           // other nodes waiting for operand or conflict
        std::list<ListDigraph::Node>   nodes_critical;          // nodes critical  and schedulable;   
        std::list<ListDigraph::Node>   nodes_other;             // nodes not critical but schedulable
        size_t highest_remaining_waiting = 0;
        size_t highest_remaining = 0;

        DOUT("avlist(@" << curr_cycle << "):");
        for ( auto n : avlist)
        {
            DOUT("...... node(@" << instruction[n]->cycle << "): " << name[n] << " remaining: " << remaining[n]);
        }

        for ( auto n : avlist)
        {
            // four variables to interface with resource manager (see GetGateParameters)
            std::string operation_name;
            std::string operation_type;
            std::string instruction_type;
            size_t      operation_duration = 0;

            ql::gate*   gp = instruction[n];
            if ( (     (ql::forward_scheduling == dir && gp->cycle <= curr_cycle)
                    || (ql::backward_scheduling == dir && curr_cycle <= gp->cycle)
                 )
                 &&
                 (     n == s
                    || n == t
                    || gp->type() == ql::gate_type_t::__dummy_gate__ 
                    || gp->type() == ql::gate_type_t::__classical_gate__ 
                    || (
                        GetGateParameters(gp->name, platform, operation_name, operation_type, instruction_type),
                        operation_duration = std::ceil( static_cast<float>(gp->duration) / cycle_time),
                        rm.available(curr_cycle, gp, operation_name, operation_type, instruction_type, operation_duration)
                       )
                 )
              )
            {
                if (remaining[n] >= highest_remaining)
                {
                    if (remaining[n] > highest_remaining)
                    {
                        nodes_other.splice(nodes_other.end(), nodes_critical); // move all of nodes_critical to nodes_other
                        highest_remaining = remaining[n];
                    }
                    nodes_critical.push_back(n);
                }
                else
                {
                    nodes_other.push_back(n);
                }
            }
            else
            {
                if (remaining[n] >= highest_remaining_waiting)
                {
                    if (remaining[n] > highest_remaining_waiting)
                    {
                        nodes_waiting.splice(nodes_waiting.end(), nodes_critical_waiting); // move all of nodes_critical to nodes_waiting
                        highest_remaining_waiting = remaining[n];
                    }
                    nodes_critical_waiting.push_back(n);
                }
                else
                {
                    nodes_waiting.push_back(n);
                }
            }
        }
        if (highest_remaining_waiting > highest_remaining)
        {
            // nodes waiting are more critical than those not waiting
            nodes_other.splice(nodes_other.end(), nodes_critical); // move all of nodes_critical to nodes_other
        }
        for (auto n : nodes_critical_waiting)
        { 
            DOUT("... node(@" << instruction[n]->cycle << "): " << name[n] << " critical waiting, remaining=" << remaining[n]);
        } 
        for (auto n : nodes_waiting)
        { 
            DOUT("... node(@" << instruction[n]->cycle << "): " << name[n] << " non-critical waiting, remaining=" << remaining[n]);
        } 
        for (auto n : nodes_critical)
        { 
            DOUT("... node(@" << instruction[n]->cycle << "): " << name[n] << " critical, remaining=" << remaining[n]);
        } 
        for (auto n : nodes_other)
        { 
            DOUT("... node(@" << instruction[n]->cycle << "): " << name[n] << " non-critical, remaining=" << remaining[n]);
        } 

        if (nodes_critical.size() == 1)
        {
            // only one node critical, select it
            selected_node = nodes_critical.front();
            DOUT("... node(@" << instruction[selected_node]->cycle << "): " << name[selected_node] << " single critical");
            success = true;
            return selected_node;
        }
        if (nodes_critical.size() > 1)
        {
            // more than one critical node, select one after comparing it for its dependent nodes
            DOUT("... number of critical nodes is " << nodes_critical.size());
            double highest_depremainingavg = -1.0;      // less than 0.0 to cmp less than 0.0 below
            for ( auto n : nodes_critical)
            {
                DOUT("... most critical: remaining of " << instruction[n]->qasm() << " : " << remaining[n]);
                double depremainingavg = avg_remaining_deps(n, dir);
                DOUT("...... depremainingavg of " << instruction[n]->qasm() << " : " << depremainingavg);
                if (depremainingavg > highest_depremainingavg)
                {
                    highest_depremainingavg = depremainingavg;
                    selected_node = n;      // selected_node is guaranteed to be assigned here
                }
            }
            // when more than one with highest avg most critical depending nodes
            // the above has selected the first of those;
            // here the original order of nodes (as in the circuit) becomes visible
            DOUT("... node(@" << instruction[selected_node]->cycle << "): " << name[selected_node] << " highest avg of depending remaining: " << highest_depremainingavg);
            success = true;
            return selected_node;
        }

        if (nodes_other.size() >= 1)
        {
            selected_node = nodes_other.front();    // just select it
            DOUT("... node(@" << instruction[selected_node]->cycle << "): " << name[selected_node] << " non-critical");
            success = true;
            return selected_node;
        }

        success = false;
        DOUT("... non selected for curr_cycle " << curr_cycle);
        return selected_node;   // fake return value
    }

    // ASAP/ALAP scheduler with RC
    //
    // schedule the circuit that is in the dependence graph
    // for the given direction, with the given platform and resource manager;
    // - the cycle attribute of the gates will be set
    // - *circp is sorted in the new cycle order
    // - bundles are collected from the circuit
    // - latency compensation and buffer-buffer delay insertion done
    // the bundles are returned, with private start/duration attributes
    ql::ir::bundles_t schedule_post179(ql::circuit* circp, ql::scheduling_direction_t dir,
            const ql::quantum_platform& platform, ql::arch::resource_manager_t& rm)
    {
        DOUT("Scheduling " << (ql::forward_scheduling == dir?"ASAP":"ALAP") << " with RC ...");

        // scheduled[n] :=: whether node n has been scheduled, init all false
        ListDigraph::NodeMap<bool>      scheduled(graph);
        // avlist :=: list of schedulable nodes, initially (see below) just s or t
        std::list<ListDigraph::Node>    avlist;

        // initializations for this scheduler
        // note that dependence graph is not modified by a scheduler, so it can be reused
        DOUT("... initialization");
        for (ListDigraph::NodeIt n(graph); n != INVALID; ++n)
        {
            scheduled[n] = false;   // none were scheduled
        }
        size_t  curr_cycle;         // current cycle for which instructions are sought
        InitAvailable(avlist, dir, curr_cycle);     // first node (SOURCE/SINK) is made available and curr_cycle set
        set_remaining(dir);         // for each gate, number of cycles until end of schedule

        DOUT("... loop over avlist until it is empty");
        while (!avlist.empty())
        {
            bool success;
            ListDigraph::Node   selected_node;
            
            selected_node = select(avlist, dir, curr_cycle, platform, rm, success);
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
               )
            {
                std::string operation_name;
                std::string operation_type;
                std::string instruction_type;
                size_t      operation_duration = 0;

                GetGateParameters(gp->name, platform, operation_name, operation_type, instruction_type),
                operation_duration = std::ceil( static_cast<float>(gp->duration) / cycle_time),
                rm.reserve(curr_cycle, gp, operation_name, operation_type, instruction_type, operation_duration);
            }
            TakeFromAvailable(selected_node, avlist, scheduled, dir);   // update avlist/scheduled/cycle
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

        latency_compensation(circp, platform);

        ql::ir::bundles_t   bundles;
        bundles = bundler(*circp);

        insert_buffer_delays(bundles, platform);

        DOUT("Scheduling " << (ql::forward_scheduling == dir?"ASAP":"ALAP") << " with RC [DONE]");
        return bundles;
    }

    ql::ir::bundles_t schedule_asap_post179(ql::arch::resource_manager_t & rm, const ql::quantum_platform & platform)
    {
        ql::ir::bundles_t   bundles;
        bundles = schedule_post179(circp, ql::forward_scheduling, platform, rm);

        DOUT("Scheduling ASAP [DONE]");
        return bundles;
    }

    ql::ir::bundles_t schedule_alap_post179(ql::arch::resource_manager_t & rm, const ql::quantum_platform & platform)
    {
        ql::ir::bundles_t   bundles;
        bundles = schedule_post179(circp, ql::backward_scheduling, platform, rm);

        DOUT("Scheduling ALAP [DONE]");
        return bundles;
    }

// =========== post179 uniform
    ql::ir::bundles_t schedule_alap_uniform_post179()
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
                    predgp_completion_cycle = curr_cycle + std::ceil(static_cast<float>(predgp->duration)/cycle_time);
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
                    // move predgp from pred_cycle to curr_cycle
                    gates_per_cycle[pred_cycle].remove(best_predgp);
                    if (gates_per_cycle[pred_cycle].size() == 0)
                    {
                        // bundle was non-empty, now it is empty
                        non_empty_bundle_count--;
                    }
                    if (gates_per_cycle[curr_cycle].size() == 0)
                    {
                        // bundle was empty, now it will be non_empty
                        non_empty_bundle_count++;
                    }
                    best_predgp->cycle = curr_cycle;
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
        bundles = bundler(*circp);

        DOUT("Scheduling ALAP UNIFORM to get bundles [DONE]");
        return bundles;
    }

public:

// =========== scheduling entry points switching out to pre179 or post179

    ql::ir::bundles_t schedule_asap()
    {
        if (ql::options::get("scheduler_post179") == "no")
        {
            return schedule_asap_pre179();
        }
        else
        {
            return schedule_asap_post179();
        }
    }

    ql::ir::bundles_t schedule_asap(ql::arch::resource_manager_t & rm, const ql::quantum_platform & platform)
    {
        if (ql::options::get("scheduler_post179") == "no")
        {
            return schedule_asap_pre179(rm, platform);
        }
        else
        {
            return schedule_asap_post179(rm, platform);
        }
    }

    ql::ir::bundles_t schedule_alap()
    {
        if (ql::options::get("scheduler_post179") == "no")
        {
            return schedule_alap_pre179();
        }
        else
        {
            return schedule_alap_post179();
        }
    }

    ql::ir::bundles_t schedule_alap(ql::arch::resource_manager_t & rm, const ql::quantum_platform & platform)
    {
        if (ql::options::get("scheduler_post179") == "no")
        {
            return schedule_alap_pre179(rm, platform);
        }
        else
        {
            return schedule_alap_post179(rm, platform);
        }
    }

    ql::ir::bundles_t schedule_alap_uniform()
    {
        if (ql::options::get("scheduler_post179") == "no")
        {
            return schedule_alap_uniform_pre179();
        }
        else
        {
            return schedule_alap_uniform_post179();
        }
    }
};

#endif
