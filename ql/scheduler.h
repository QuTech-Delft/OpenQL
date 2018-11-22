/**
 * @file   scheduler.h
 * @date   01/2017
 * @author Imran Ashraf
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
#include "ql/arch/cc_light_resource_manager.h"

using namespace std;
using namespace lemon;

enum DepTypes{RAW, WAW, WAR, RAR};
const string DepTypesNames[] = {"RAW", "WAW", "WAR", "RAR"};

class Scheduler
{
private:
    ListDigraph graph;

    ListDigraph::NodeMap<ql::gate*> instruction;
    ListDigraph::NodeMap<std::string> name;
    ListDigraph::ArcMap<int> weight;
    //TODO it might be more readable to change 'cause' to string
    //   to accomodate/print both r0, q0 operands as cause
    ListDigraph::ArcMap<int> cause;
    ListDigraph::ArcMap<int> depType;

    ListDigraph::NodeMap<double> dist;
    Path<ListDigraph> p;

    ListDigraph::Node s, t;
    size_t cycle_time;
    std::map< std::pair<std::string,std::string>, size_t> buffer_cycles_map;

    size_t qubit_count;
    size_t creg_count;

public:
    Scheduler(): instruction(graph), name(graph), weight(graph),
        cause(graph), depType(graph), dist(graph) {}

    void Init(ql::circuit& ckt, ql::quantum_platform platform, size_t qcount, size_t ccount)
    {
        DOUT("Scheduler initialization ...");
        qubit_count = qcount;
        creg_count = ccount;
        size_t qubit_creg_count = qubit_count + creg_count;
        cycle_time = platform.cycle_time;
        DOUT("... num_qubits: " << qcount << " cycle_time: " << cycle_time);

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
            name[consNode] = ins->qasm();

            // Add edges
            auto operands = ins->operands;
            size_t op_count = operands.size();
            size_t operandNo=0;

            if(ins->name == "wait")
            {
                for( auto operand : operands )
                {
                    { // WAW dependencies
                        int prodID = LastWriter[operand];
                        ListDigraph::Node prodNode = graph.nodeFromId(prodID);
                        ListDigraph::Arc arc = graph.addArc(prodNode,consNode);
                        weight[arc] = std::ceil( static_cast<float>(instruction[prodNode]->duration) / cycle_time);
                        cause[arc] = operand;
                        depType[arc] = WAW;
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
                        }
                    }
                }

                // now update LastWriter
                for( auto operand : operands )
                {
                    LastWriter[operand] = consID;
                }
            }
            else if(ins->name == "measure")
            {
                for( auto operand : operands )
                {
                    { // WAW dependencies
                        int prodID = LastWriter[operand];
                        ListDigraph::Node prodNode = graph.nodeFromId(prodID);
                        ListDigraph::Arc arc = graph.addArc(prodNode,consNode);
                        weight[arc] = std::ceil( static_cast<float>(instruction[prodNode]->duration) / cycle_time);
                        cause[arc] = operand;
                        depType[arc] = WAW;
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
                        }
                    }
                }

                ql::measure * mins = (ql::measure*)ins;
                for( auto operand : mins->creg_operands )
                {
                    { // WAW dependencies
                        int prodID = LastWriter[qubit_count+operand];
                        ListDigraph::Node prodNode = graph.nodeFromId(prodID);
                        ListDigraph::Arc arc = graph.addArc(prodNode,consNode);
                        weight[arc] = std::ceil( static_cast<float>(instruction[prodNode]->duration) / cycle_time);
                        cause[arc] = operand;
                        depType[arc] = WAW;
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
                        }
                    }
                }

                // now update LastWriter
                for( auto operand : operands )
                {
                    LastWriter[operand] = consID;
                }
                for( auto operand : mins->creg_operands )
                {
                    LastWriter[operand] = consID;
                }
            }
            else if(ins->name == "swap" )
            {
                for( auto operand : operands )
                {
                    { // RAW dependencies
                        int prodID = LastWriter[operand];
                        ListDigraph::Node prodNode = graph.nodeFromId(prodID);
                        ListDigraph::Arc arc = graph.addArc(prodNode,consNode);
                        weight[arc] = std::ceil( static_cast<float>(instruction[prodNode]->duration) / cycle_time);
                        cause[arc] = operand;
                        depType[arc] = RAW;
                    }

                    { // RAR dependencies
                        ReadersListType readers = LastReaders[operand];
                        for(auto & readerID : readers)
                        {
                            ListDigraph::Node readerNode = graph.nodeFromId(readerID);
                            ListDigraph::Arc arc1 = graph.addArc(readerNode,consNode);
                            weight[arc1] = std::ceil( static_cast<float>(instruction[readerNode]->duration) / cycle_time);
                            cause[arc1] = operand;
                            depType[arc1] = RAR;
                        }
                    }

                    { // WAW dependencies
                        int prodID = LastWriter[operand];
                        ListDigraph::Node prodNode = graph.nodeFromId(prodID);
                        ListDigraph::Arc arc = graph.addArc(prodNode,consNode);
                        weight[arc] = std::ceil( static_cast<float>(instruction[prodNode]->duration) / cycle_time);
                        cause[arc] = operand;
                        depType[arc] = WAW;
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
                        }
                    }
                }

                for( auto operand : operands )
                {
                    // update LastWriter and LastReaders
                    LastWriter[operand] = consID;
                    // clear LastReaders for this operand
                    LastReaders[operand].clear();
                }
            }
            else if(ins->name == "display")
            {
                std::vector<size_t> qubits(qubit_creg_count);
                std::iota(qubits.begin(), qubits.end(), 0);
                for( auto operand : qubits )
                {
                    { // WAW dependencies
                        int prodID = LastWriter[operand];
                        ListDigraph::Node prodNode = graph.nodeFromId(prodID);
                        ListDigraph::Arc arc = graph.addArc(prodNode,consNode);
                        weight[arc] = std::ceil( static_cast<float>(instruction[prodNode]->duration) / cycle_time);
                        cause[arc] = operand;
                        depType[arc] = WAW;
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
                        }
                    }
                }

                // now update LastWriter
                for( auto operand : operands )
                {
                    LastWriter[operand] = consID;
                }
            }
            else if(ins->type() == ql::gate_type_t::__classical_gate__)
            {
                std::vector<size_t> all_operands(qubit_creg_count);
                std::iota(all_operands.begin(), all_operands.end(), 0);
                for( auto operand : all_operands )
                {
                    { // WAW dependencies
                        int prodID = LastWriter[operand];
                        ListDigraph::Node prodNode = graph.nodeFromId(prodID);
                        ListDigraph::Arc arc = graph.addArc(prodNode,consNode);
                        weight[arc] = std::ceil( static_cast<float>(instruction[prodNode]->duration) / cycle_time);
                        cause[arc] = operand;
                        depType[arc] = WAW;
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
                        }
                    }
                }

                // now update LastWriter
                for( auto operand : all_operands )
                {
                    LastWriter[operand] = consID;
                }
            }
            else
            {
                for( auto operand : operands )
                {
                    // DOUT("Operand: " << operand << " operandNo: " << operandNo << " op_count: " << op_count);
                    if( operandNo < op_count-1 )
                    {
                        // RAW dependencies
                        // DOUT("... starting RAW for operand:" << operand);
                        int prodID = LastWriter[operand];
                        ListDigraph::Node prodNode = graph.nodeFromId(prodID);
                        ListDigraph::Arc arc = graph.addArc(prodNode,consNode);
                        weight[arc] = std::ceil( static_cast<float>(instruction[prodNode]->duration) / cycle_time);
                        cause[arc] = operand;
                        depType[arc] = RAW;

                        // RAR dependencies
                        // DOUT("... starting RAR for operand:" << operand);
                        ReadersListType readers = LastReaders[operand];
                        for(auto & readerID : readers)
                        {
                            ListDigraph::Node readerNode = graph.nodeFromId(readerID);
                            ListDigraph::Arc arc1 = graph.addArc(readerNode,consNode);
                            weight[arc1] = std::ceil( static_cast<float>(instruction[readerNode]->duration) / cycle_time);
                            cause[arc1] = operand;
                            depType[arc1] = RAR;
                        }

                        // update LastReaders for this operand
                        LastReaders[operand].push_back(consID);
                    }
                    else
                    {
                        // WAW dependencies
                        // DOUT("... starting WAW for operand:" << operand);
                        int prodID = LastWriter[operand];
                        ListDigraph::Node prodNode = graph.nodeFromId(prodID);
                        ListDigraph::Arc arc = graph.addArc(prodNode,consNode);
                        weight[arc] = std::ceil( static_cast<float>(instruction[prodNode]->duration) / cycle_time);
                        cause[arc] = operand;
                        depType[arc] = WAW;

                        // WAR dependencies
                        // DOUT("... starting WAR for operand:" << operand);
                        ReadersListType readers = LastReaders[operand];
                        for(auto & readerID : readers)
                        {
                            ListDigraph::Node readerNode = graph.nodeFromId(readerID);
                            ListDigraph::Arc arc1 = graph.addArc(readerNode,consNode);
                            weight[arc1] = std::ceil( static_cast<float>(instruction[readerNode]->duration) / cycle_time);
                            cause[arc1] = operand;
                            depType[arc1] = WAR;
                        }

                        // clear LastReaders for this operand
                        LastReaders[operand].clear();

                        // update LastWriter for this operand
                        LastWriter[operand] = consID;
                    }
                    operandNo++;
                } // end of operand for
            } // end of if/else
        } // end of instruction for

        // add dummy target node
        ListDigraph::Node targetNode = graph.addNode();
        instruction[targetNode] = new ql::SINK();
        name[targetNode] = instruction[targetNode]->qasm();
        t=targetNode;

        // make links to the dummy target node
        OutDegMap<ListDigraph> outDeg(graph);
        for (ListDigraph::NodeIt n(graph); n != INVALID; ++n)
        {
            if( outDeg[n] == 0 && n!=targetNode )
            {
                ListDigraph::Arc arc = graph.addArc(n,targetNode);
                weight[arc] = 1; // TARGET is dummy
                cause[arc] = 0;
                depType[arc] = RAW;
            }
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

    void TopologicalSort(std::vector<ListDigraph::Node> & order)
    {
        // DOUT("Performing Topological sort.");
        ListDigraph::NodeMap<int> rorder(graph);
        if( !dag(graph) )
            EOUT("This digraph is not a DAG.");

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

// =========== asap

private:
    void schedule_asap_(ListDigraph::NodeMap<size_t> & cycle, std::vector<ListDigraph::Node> & order)
    {
        DOUT("Performing ASAP Scheduling");
        TopologicalSort(order);

        std::vector<ListDigraph::Node>::reverse_iterator currNode = order.rbegin();
        cycle[*currNode]=0; // src dummy in cycle 0
        instruction[*currNode]->cycle = 0;
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
            instruction[*currNode]->cycle = currCycle;
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
        instruction[*currNode]->cycle = currCycle;
        DOUT("Source instruction: " << instruction[*currNode]->qasm());
        ++currNode;
        while(currNode != order.rend() )
        {
            DOUT("");
            DOUT("Current instruction: " << instruction[*currNode]->qasm());
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
                    // DOUT("Trying to schedule: " << name[*currNode] << "  in cycle: " << op_start_cycle);
                    // DOUT("current operation_duration: " << operation_duration);
                    if( rm.available(op_start_cycle, curr_ins, operation_name, operation_type, instruction_type, operation_duration) )
                    {
                        // DOUT("Resources available at cycle " << op_start_cycle << ", Scheduled.");

                        rm.reserve(op_start_cycle, curr_ins, operation_name, operation_type, instruction_type, operation_duration);
                        cycle[*currNode]=op_start_cycle;
                        instruction[*currNode]->cycle = op_start_cycle;
                        break;
                    }
                    else
                    {
                        // DOUT("Resources not available at cycle " << op_start_cycle << ", trying again ...");
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
            instruction[*it]->cycle = cycle[*it];
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

public:
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


    ql::ir::bundles_t schedule_asap()
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

        DOUT("Scheduling ASAP to get bundles [DONE]");
        return bundles;
    }



    // the following with rc and buffer-buffer delays
    ql::ir::bundles_t schedule_asap(ql::arch::resource_manager_t & rm, const ql::quantum_platform & platform)
    {
        DOUT("RC Scheduling ASAP to get bundles ...");
        ql::ir::bundles_t bundles;
        ListDigraph::NodeMap<size_t> cycle(graph);
        std::vector<ListDigraph::Node> order;

        schedule_asap_(cycle, order, rm, platform);

        typedef std::vector<ql::gate*> insInOneCycle;
        std::map<size_t,insInOneCycle> insInAllCycles;

        std::vector<ListDigraph::Node>::iterator it;
        DOUT("After rc scheduling, collecting cycles for bundling");
        for ( it = order.begin(); it != order.end(); ++it)
        {
            if ( instruction[*it]->type() != ql::gate_type_t::__wait_gate__ &&
                 instruction[*it]->type() != ql::gate_type_t::__dummy_gate__
               )
            {
                // DOUT("Instruction: " << instruction[*it]->qasm());
                insInAllCycles[ cycle[*it] ].push_back( instruction[*it] );
            }
        }

        size_t TotalCycles = 0;
        if( ! order.empty() )
        {
            TotalCycles =  cycle[ *( order.begin() ) ];
        }

        for(size_t currCycle = 0; currCycle<=TotalCycles; ++currCycle)
        {
            auto it = insInAllCycles.find(currCycle);
            if( it != insInAllCycles.end() )
            {
                // DOUT("Bundling at cycle: " << currCycle);
                ql::ir::bundle_t abundle;
                size_t bduration = 0;
                auto nInsThisCycle = insInAllCycles[currCycle].size();
                // DOUT("... nInsThisCycle: " << nInsThisCycle);
                for(size_t i=0; i<nInsThisCycle; ++i )
                {
                    ql::ir::section_t aparsec;
                    auto & ins = insInAllCycles[currCycle][i];
                    aparsec.push_back(ins);
                    abundle.parallel_sections.push_back(aparsec);
                    // DOUT("... ins: " << ins->qasm() << " in private parallel section");
                    size_t iduration = ins->duration;
                    bduration = std::max(bduration, iduration);
                }
                abundle.start_cycle = currCycle;
                abundle.duration_in_cycles = std::ceil(static_cast<float>(bduration)/cycle_time);
                // DOUT("... bundel duration in cycles: " << abundle.duration_in_cycles);
                bundles.push_back(abundle);
                // DOUT("... ready with bundle");
            }
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

// =========== alap

private:
    void schedule_alap_(ListDigraph::NodeMap<size_t> & cycle, std::vector<ListDigraph::Node> & order)
    {
        DOUT("Performing ALAP Scheduling");
        TopologicalSort(order);

        std::vector<ListDigraph::Node>::iterator currNode = order.begin();
        cycle[*currNode]=MAX_CYCLE;
        instruction[*currNode]->cycle = MAX_CYCLE;
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
            instruction[*currNode]->cycle = currCycle;
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
            auto & curr_ins = instruction[*currNode];
            auto & id = curr_ins->name;

            size_t op_start_cycle=MAX_CYCLE;
            // DOUT("Scheduling " << name[*currNode]);
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
                    // DOUT("Trying to schedule: " << name[*currNode] << "  in cycle: " << op_start_cycle);
                    // DOUT("current operation_duration: " << operation_duration);
                    if( rm.available(op_start_cycle, curr_ins, operation_name, operation_type, instruction_type, operation_duration) )
                    {
                        // DOUT("Resources available at cycle " << op_start_cycle << ", Scheduled.");

                        rm.reserve(op_start_cycle, curr_ins, operation_name, operation_type, instruction_type, operation_duration);
                        cycle[*currNode]=op_start_cycle;
                        break;
                    }
                    else
                    {
                        // DOUT("Resources not available at cycle " << op_start_cycle << ", trying again ...");
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


public:
    // the following without rc and buffer-buffer delays
    ql::ir::bundles_t schedule_alap()
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
        // DOUT("Depth=" << TotalCycles);
        DOUT("Scheduling ALAP to get bundles [DONE]");
        return bundles;
    }

    // the following with rc and buffer-buffer delays
    ql::ir::bundles_t schedule_alap(ql::arch::resource_manager_t & rm, 
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

        DOUT("Depth=" << (TotalCycles+buffer_cycles_accum));
        DOUT("RC Scheduling ALAP to get bundles [DONE]");
        return bundles;
    }


// =========== uniform

private:
    void compute_alap_cycle(ListDigraph::NodeMap<size_t> & cycle, std::vector<ListDigraph::Node> & order, size_t max_cycle)
    {
        // DOUT("Computing alap_cycle");
        std::vector<ListDigraph::Node>::iterator currNode = order.begin();
        cycle[*currNode]=max_cycle;
        instruction[*currNode]->cycle = max_cycle;
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
            instruction[*currNode]->cycle = currCycle;
            ++currNode;
        }
    }

    void compute_asap_cycle(ListDigraph::NodeMap<size_t> & cycle, std::vector<ListDigraph::Node> & order)
    {
        // DOUT("Computing asap_cycle");
        std::vector<ListDigraph::Node>::reverse_iterator currNode = order.rbegin();
        cycle[*currNode]=0; // src dummy in cycle 0
        instruction[*currNode]->cycle = 0;
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
            instruction[*currNode]->cycle = currCycle;
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

public:
    ql::ir::bundles_t schedule_alap_uniform()
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

        DOUT("Depth=" << TotalCycles);
        DOUT("Scheduling ALAP UNIFORM to get bundles [DONE]");
        return bundles;
    }

};

#endif
