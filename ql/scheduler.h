/**
 * @file   scheduler.h
 * @date   01/2017
 * @author Imran Ashraf
 * @brief  ASAP/AlAP scheduling
 */

#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include <lemon/list_graph.h>
#include <lemon/lgf_reader.h>
#include <lemon/lgf_writer.h>
#include <lemon/dijkstra.h>
#include <lemon/connectivity.h>

#include "utils.h"
#include "gate.h"
#include "circuit.h"
#include <ql/arch/cc_light_resource_manager.h>

using namespace std;
using namespace lemon;

enum DepTypes{RAW, WAW, WAR, RAR};
const string DepTypesNames[] = {"RAW", "WAW", "WAR", "RAR"};

typedef std::list<ql::gate *>ParallelSection;
class Bundle
{
public:
    size_t start_cycle;
    size_t duration_in_cycles;
    std::list<ParallelSection>ParallelSections;
};
typedef std::list<Bundle>Bundles;

class Scheduler
{
private:
    ListDigraph graph;

    ListDigraph::NodeMap<ql::gate*> instruction;
    ListDigraph::NodeMap<std::string> name;
    ListDigraph::ArcMap<int> weight;
    ListDigraph::ArcMap<int> cause;
    ListDigraph::ArcMap<int> depType;

    ListDigraph::NodeMap<double> dist;
    Path<ListDigraph> p;

    ListDigraph::Node s, t;
    size_t cycle_time;
    std::map< std::pair<std::string,std::string>, size_t> buffer_cycles_map;

    size_t num_qubits;

public:
    Scheduler(): instruction(graph), name(graph), weight(graph),
        cause(graph), depType(graph), dist(graph) {}

    void Init( size_t nQubits, ql::circuit& ckt, ql::quantum_platform platform, bool verbose=false)
    {
        num_qubits = nQubits;
        cycle_time = platform.cycle_time;

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
                if( platform.hardware_settings[bname].is_null() )
                {
                    buffer_cycles_map[ bpair ] = 0;
                }
                else
                {
                    buffer_cycles_map[ bpair ] = std::ceil( static_cast<float>(platform.hardware_settings[bname]) / cycle_time);
                }
                DOUT("Initializing " << bname << ": "<< buffer_cycles_map[bpair]);
            }
        }


        // add dummy source node
        ListDigraph::Node srcNode = graph.addNode();
        instruction[srcNode] = new ql::SOURCE();
        name[srcNode] = instruction[srcNode]->qasm();
        s=srcNode;

        typedef vector<int> ReadersListType;
        vector<ReadersListType> LastReaders;
        LastReaders.resize(nQubits);

        int srcID = graph.id(srcNode);
        vector<int> LastWriter(nQubits,srcID);

        for( auto ins : ckt )
        {
            // DOUT("\nCurrent instruction : " << ins->qasm());

            // Add nodes
            ListDigraph::Node consNode = graph.addNode();
            int consID = graph.id(consNode);
            instruction[consNode] = ins;
            name[consNode] = ins->qasm();

            // Add edges
            auto operands = ins->operands;
            size_t operandCount = operands.size();
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
            else
            {
                for( auto operand : operands )
                {
                    // DOUT("Operand: " << operand);
                    if( operandNo < operandCount-1 )
                    {
                        // RAW dependencies
                        int prodID = LastWriter[operand];
                        ListDigraph::Node prodNode = graph.nodeFromId(prodID);
                        ListDigraph::Arc arc = graph.addArc(prodNode,consNode);
                        weight[arc] = std::ceil( static_cast<float>(instruction[prodNode]->duration) / cycle_time);
                        cause[arc] = operand;
                        depType[arc] = RAW;

                        // RAR dependencies
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
                        int prodID = LastWriter[operand];
                        ListDigraph::Node prodNode = graph.nodeFromId(prodID);
                        ListDigraph::Arc arc = graph.addArc(prodNode,consNode);
                        weight[arc] = std::ceil( static_cast<float>(instruction[prodNode]->duration) / cycle_time);
                        cause[arc] = operand;
                        depType[arc] = WAW;

                        // WAR dependencies
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
    }

    void Print(bool verbose=false)
    {
        if(verbose) COUT("Printing Dependence Graph ");
        digraphWriter(graph).
        nodeMap("name", name).
        arcMap("cause", cause).
        arcMap("weight", weight).
        // arcMap("depType", depType).
        node("source", s).
        node("target", t).
        run();
    }

    void PrintMatrix(bool verbose=false)
    {
        if(verbose) COUT("Printing Dependence Graph as Matrix");
        ofstream fout;
        string datfname( ql::utils::get_output_dir() + "/dependenceMatrix.dat");
        fout.open( datfname, ios::binary);
        if ( fout.fail() )
        {
            COUT("Error opening file " << datfname << std::endl
                     << "Make sure the output directory ("<< ql::utils::get_output_dir() << ") exists");
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

    void PrintDot1_(
                bool WithCritical,
                bool WithCycles,
                ListDigraph::NodeMap<size_t> & cycle,
                std::vector<ListDigraph::Node> & order,
                std::ostream& dotout
                )
    {
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
            int nid = graph.id(n);
            string nodeName = name[n];
            dotout  << "\"" << nid << "\""
                    << " [label=\" " << nodeName <<" \""
                    << NodeStyle
                    << "];" << endl;
        }

        if( WithCycles)
        {
            // Print cycle numbers as timeline, as shown below
            size_t cn=0,TotalCycles=0;
            dotout << "{\nnode [shape=plaintext, fontsize=16, fontcolor=blue]; \n";
            ListDigraph::NodeMap<size_t>::MapIt it(cycle);
            if(it != INVALID)
                TotalCycles=cycle[it];
            for(cn=0;cn<=TotalCycles;++cn)
            {
                if(cn>0)
                    dotout << " -> ";
                dotout << "Cycle" << cn;
            }
            dotout << ";\n}\n";

            // Now print ranks, as shown below
            std::vector<ListDigraph::Node>::reverse_iterator rit;
            for ( rit = order.rbegin(); rit != order.rend(); ++rit)
            {
                int nid = graph.id(*rit);
                dotout << "{ rank=same; Cycle" << cycle[*rit] <<"; " <<nid<<"; }\n";
            }
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
    }

    void PrintDot(bool verbose=false)
    {
        if(verbose) COUT("Printing Dependence Graph in DOT");
        ofstream dotout;
        string dotfname(ql::utils::get_output_dir() + "/dependenceGraph.dot");
        dotout.open(dotfname, ios::binary);
        if ( dotout.fail() )
        {
            COUT("Error opening file " << dotfname << std::endl
                     << "Make sure the output directory ("<< ql::utils::get_output_dir() << ") exists");
            return;
        }

        ListDigraph::NodeMap<size_t> cycle(graph);
        std::vector<ListDigraph::Node> order;
        PrintDot1_(false, false, cycle, order, dotout);
        dotout.close();
    }

    void TopologicalSort(std::vector<ListDigraph::Node> & order)
    {
        // COUT("Performing Topological sort.");
        ListDigraph::NodeMap<int> rorder(graph);
        if( !dag(graph) )
            COUT("This digraph is not a DAG.");

        topologicalSort(graph, rorder);

#ifdef DEBUG
        for (ListDigraph::ArcIt a(graph); a != INVALID; ++a)
        {
            if( rorder[graph.source(a)] > rorder[graph.target(a)] )
                COUT("Wrong topologicalSort()");
        }
#endif

        for ( ListDigraph::NodeMap<int>::MapIt it(rorder); it != INVALID; ++it)
        {
            order.push_back(it);
        }
    }

    void PrintTopologicalOrder(bool verbose=false)
    {
        std::vector<ListDigraph::Node> order;
        TopologicalSort(order);

        if(verbose) COUT("Printing nodes in Topological order");
        for ( std::vector<ListDigraph::Node>::reverse_iterator it = order.rbegin(); it != order.rend(); ++it)
        {
            std::cout << name[*it] << std::endl;
        }
    }

    void ScheduleASAP(ListDigraph::NodeMap<size_t> & cycle, std::vector<ListDigraph::Node> & order, bool verbose=false)
    {
        if(verbose) COUT("Performing ASAP Scheduling");
        TopologicalSort(order);

        std::vector<ListDigraph::Node>::reverse_iterator currNode = order.rbegin();
        cycle[*currNode]=0; // src dummy in cycle 0
        ++currNode;
        while(currNode != order.rend() )
        {
            size_t currCycle=0;
            // COUT("Scheduling " << name[*currNode]);
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

    void PrintScheduleASAP(bool verbose=false)
    {
        ListDigraph::NodeMap<size_t> cycle(graph);
        std::vector<ListDigraph::Node> order;
        ScheduleASAP(cycle,order);

        COUT("\nPrinting ASAP Schedule");
        std::cout << "Cycle <- Instruction " << std::endl;
        std::vector<ListDigraph::Node>::reverse_iterator it;
        for ( it = order.rbegin(); it != order.rend(); ++it)
        {
            std::cout << cycle[*it] << "     <- " <<  name[*it] << std::endl;
        }
    }

    std::string GetDotScheduleASAP(bool verbose=false)
    {
        stringstream dotout;
        ListDigraph::NodeMap<size_t> cycle(graph);
        std::vector<ListDigraph::Node> order;
        ScheduleASAP(cycle,order);
        PrintDot1_(false,true,cycle,order,dotout);
        return dotout.str();
    }

    void PrintDotScheduleASAP(bool verbose=false)
    {
        ofstream dotout;
        string dotfname( ql::utils::get_output_dir() + "/scheduledASAP.dot");
        dotout.open( dotfname, ios::binary);
        if ( dotout.fail() )
        {
            COUT("Error opening file " << dotfname << std::endl
                     << "Make sure the output directory ("<< ql::utils::get_output_dir() << ") exists");
            return;
        }

        if(verbose) COUT("Printing Scheduled Graph in " << dotfname);
        dotout << GetDotScheduleASAP(verbose);
        dotout.close();
    }

    std::string GetQASMScheduledASAP(bool verbose=false)
    {
        std::stringstream ss;

        ListDigraph::NodeMap<size_t> cycle(graph);
        std::vector<ListDigraph::Node> order;
        ScheduleASAP(cycle, order, verbose);

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

        size_t empty_cycles=0;
        ss << '\n';
        for(size_t currCycle = 1; currCycle<TotalCycles; ++currCycle)
        {
            auto it = insInAllCycles.find(currCycle);
            if( it != insInAllCycles.end() )
            {
                if( empty_cycles > 0 )
                {
                    ss << "    qwait " << empty_cycles << '\n';
                    empty_cycles=0;
                }

                auto nInsThisCycle = insInAllCycles[currCycle].size();
                if(nInsThisCycle>0)
                    ss << "    ";
                if (nInsThisCycle > 1) ss << "{ "; 
                for(size_t i=0; i<nInsThisCycle; ++i )
                {
                    auto & ins = insInAllCycles[currCycle][i];
                    std::string ins_name = ins->qasm();
                    ss << ins_name;
                    if( i != nInsThisCycle - 1 ) // last instruction
                        ss << " | ";
                }
                if (nInsThisCycle > 1) ss << " }"; 
                ss << '\n';
            }
            else
            {
                ++empty_cycles;
            }
        }

        // printing of last qwait
        size_t bduration = 0;
        size_t currCycle = TotalCycles-1;
        auto it = insInAllCycles.find(currCycle);
        if( it != insInAllCycles.end() )
        {
            auto nInsThisCycle = insInAllCycles[currCycle].size();
            for(size_t i=0; i<nInsThisCycle; ++i )
            {
                size_t iduration = insInAllCycles[currCycle][i]->duration;
                bduration = std::max(bduration, iduration);
            }
        }

        int bduration_in_cycles = std::ceil(static_cast<float>(bduration)/cycle_time);
        if( bduration_in_cycles > 1 )
            ss << "    qwait " << bduration_in_cycles -1 << '\n';

        return ss.str();
    }

    void PrintQASMScheduledASAP(bool verbose=false)
    {
        ofstream fout;
        string qcfname(ql::utils::get_output_dir() + "/scheduledASAP.qasm");
        fout.open( qcfname, ios::binary);
        if ( fout.fail() )
        {
            COUT("Error opening file " << qcfname << std::endl
                     << "Make sure the output directory ("<< ql::utils::get_output_dir() << ") exists");
            return;
        }

        fout << GetQASMScheduledASAP(verbose);
        fout.close();
    }

    // without rc and latency compensation
    void ScheduleALAP(ListDigraph::NodeMap<size_t> & cycle, std::vector<ListDigraph::Node> & order, bool verbose=false)
    {
        if(verbose) COUT("Performing ALAP Scheduling");
        TopologicalSort(order);

        std::vector<ListDigraph::Node>::iterator currNode = order.begin();
        cycle[*currNode]=MAX_CYCLE;
        ++currNode;
        while(currNode != order.end() )
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
        if(verbose) COUT("Performing ALAP Scheduling [Done].");
    }


    // with rc and latency compensation
    void ScheduleASAP( ListDigraph::NodeMap<size_t> & cycle, std::vector<ListDigraph::Node> & order,
                       ql::arch::resource_manager_t & rm, ql::quantum_platform & platform, bool verbose=false )
    {
        if(verbose) COUT("Performing RC ASAP Scheduling");
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

            std::string operation_name(id);
            std::string operation_type; // MW/FLUX/READOUT etc
            std::string instruction_type; // single / two qubit
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

            size_t operation_duration = std::ceil( static_cast<float>(curr_ins->duration) / cycle_time);
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

            while(op_start_cycle < MAX_CYCLE)
            {
                DOUT("Trying to schedule: " << name[*currNode] << "  in cycle: " << op_start_cycle);
                DOUT("current operation_duration: " << operation_duration);
                if( rm.available(op_start_cycle, curr_ins, operation_name, operation_type, instruction_type, operation_duration) )
                {
                    DOUT("Resources available, Scheduled.");

                    rm.reserve(op_start_cycle, curr_ins, operation_name, operation_type, instruction_type, operation_duration);
                    cycle[*currNode]=op_start_cycle;
                    break;
                }
                else
                {
                    DOUT("Resources not available, trying again ...");
                    ++op_start_cycle;
                }
            }
            if(op_start_cycle >= MAX_CYCLE)
            {
                EOUT("Error: could not find schedule");
                throw ql::exception("[x] Error : could not find schedule !",false);
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
        for ( auto it = order.begin(); it != order.end(); ++it)
        {
            auto & curr_ins = instruction[*it];
            auto & id = curr_ins->name;
            long latency_cycles=0;
            if ( !platform.instruction_settings[id]["latency"].is_null() )
            {
                float latency_ns = platform.instruction_settings[id]["latency"];
                latency_cycles = (std::ceil( static_cast<float>(std::abs(latency_ns)) / cycle_time)) * 
                                        ql::utils::sign_of(latency_ns);
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

        // DOUT("Printing ASAP Schedule after latency compensation");
        // DOUT("Cycle    Instruction");
        // for ( auto it = order.rbegin(); it != order.rend(); ++it)
        // {
        //     DOUT( cycle[*it] << "        " << name[*it] );
        // }

        if(verbose) COUT("Performing RC ASAP Scheduling [Done].");
    }


    void PrintScheduleALAP(bool verbose=false)
    {
        ListDigraph::NodeMap<size_t> cycle(graph);
        std::vector<ListDigraph::Node> order;
        ScheduleALAP(cycle,order);

        COUT("\nPrinting ALAP Schedule");
        std::cout << "Cycle <- Instruction " << std::endl;
        std::vector<ListDigraph::Node>::reverse_iterator it;
        for ( it = order.rbegin(); it != order.rend(); ++it)
        {
            std::cout << MAX_CYCLE-cycle[*it] << "     <- " <<  name[*it] << std::endl;
        }
    }

    void PrintDot2_(
                bool WithCritical,
                bool WithCycles,
                ListDigraph::NodeMap<size_t> & cycle,
                std::vector<ListDigraph::Node> & order,
                std::ostream& dotout
                )
    {
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
            int nid = graph.id(n);
            string nodeName = name[n];
            dotout  << "\"" << nid << "\""
                    << " [label=\" " << nodeName <<" \""
                    << NodeStyle
                    << "];" << endl;
        }

        if( WithCycles)
        {
            // Print cycle numbers as timeline, as shown below
            size_t cn=0,TotalCycles = MAX_CYCLE - cycle[ *( order.rbegin() ) ];
            dotout << "{\nnode [shape=plaintext, fontsize=16, fontcolor=blue]; \n";

            for(cn=0;cn<=TotalCycles;++cn)
            {
                if(cn>0)
                    dotout << " -> ";
                dotout << "Cycle" << cn;
            }
            dotout << ";\n}\n";

            // Now print ranks, as shown below
            std::vector<ListDigraph::Node>::reverse_iterator rit;
            for ( rit = order.rbegin(); rit != order.rend(); ++rit)
            {
                int nid = graph.id(*rit);
                dotout << "{ rank=same; Cycle" << TotalCycles - (MAX_CYCLE - cycle[*rit]) <<"; " <<nid<<"; }\n";
            }
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
    }

    void PrintDotScheduleALAP(bool verbose=false)
    {
        ofstream dotout;
        string dotfname(ql::utils::get_output_dir() + "/scheduledALAP.dot");
        dotout.open( dotfname, ios::binary);
        if ( dotout.fail() )
        {
            EOUT("Error opening file " << dotfname << std::endl
                     << "Make sure the output directory ("<< ql::utils::get_output_dir() << ") exists");
            return;
        }

        if(verbose) COUT("Printing Scheduled Graph in " << dotfname);
        ListDigraph::NodeMap<size_t> cycle(graph);
        std::vector<ListDigraph::Node> order;
        ScheduleALAP(cycle,order);
        PrintDot2_(false,true,cycle,order,dotout);

        dotout.close();
    }

    std::string GetDotScheduleALAP(bool verbose=false)
    {
        stringstream dotout;
        ListDigraph::NodeMap<size_t> cycle(graph);
        std::vector<ListDigraph::Node> order;
        ScheduleALAP(cycle,order);
        PrintDot2_(false,true,cycle,order,dotout);
        return dotout.str();
    }


    std::string GetQASMScheduledALAP(bool verbose=false)
    {
        Bundles bundles = GetBundlesScheduleALAP(verbose);

        std::stringstream ssbundles;
        size_t curr_cycle=1;

        for (Bundle & abundle : bundles)
        {
            auto bcycle = abundle.start_cycle;
            auto delta = bcycle - curr_cycle;
            if(delta>1)
                ssbundles << "\n    qwait " << delta-1 << "\n";
            else
                ssbundles << "\n";

            ssbundles << "    ";
            if (abundle.ParallelSections.size() > 1) ssbundles << "{ "; 
            for( auto secIt = abundle.ParallelSections.begin(); secIt != abundle.ParallelSections.end(); ++secIt )
            {
                auto firstInsIt = secIt->begin();
                auto insqasm = (*(firstInsIt))->qasm();
                ssbundles << insqasm << " ";

                if( std::next(secIt) != abundle.ParallelSections.end() )
                {
                    ssbundles << " | ";
                }
            }
            if (abundle.ParallelSections.size() > 1) ssbundles << " }"; 
            curr_cycle+=delta;
        }

        auto & lastBundle = bundles.back();
        int lbduration = lastBundle.duration_in_cycles;
        if( lbduration > 1 )
            ssbundles << "\n    qwait " << lbduration -1 << '\n';

        return ssbundles.str();
    }

    void PrintQASMScheduledALAP(bool verbose=false)
    {
        ofstream fout;
        string qcfname(ql::utils::get_output_dir() + "/scheduledALAP.qasm");
        fout.open( qcfname, ios::binary);
        if ( fout.fail() )
        {
            EOUT("Error opening file " << qcfname << std::endl
                     << "Make sure the output directory ("<< ql::utils::get_output_dir() << ") exists");
            return;
        }

        fout << GetQASMScheduledALAP(verbose);
        fout.close();
    }


    // the following without rc and buffer-buffer delays
    Bundles GetBundlesScheduleALAP(bool verbose=false)
    {
        if(verbose) COUT("Scheduling ALAP to get bundles ...");
        Bundles bundles;
        ListDigraph::NodeMap<size_t> cycle(graph);
        std::vector<ListDigraph::Node> order;
        ScheduleALAP(cycle,order);

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
            Bundle abundle;
            abundle.start_cycle = TotalCycles - currCycle;
            size_t bduration = 0;
            if( it != insInAllCycles.end() )
            {
                auto nInsThisCycle = insInAllCycles[currCycle].size();
                for(size_t i=0; i<nInsThisCycle; ++i )
                {
                    ParallelSection aparsec;
                    auto & ins = insInAllCycles[currCycle][i];
                    aparsec.push_back(ins);
                    abundle.ParallelSections.push_back(aparsec);
                    size_t iduration = ins->duration;
                    bduration = std::max(bduration, iduration);
                }
                abundle.duration_in_cycles = std::ceil(static_cast<float>(bduration)/cycle_time);
                bundles.push_back(abundle);
            }
        }
        if(verbose) COUT("Scheduling ALAP to get bundles [DONE]");
        return bundles;
    }


    // the following with rc and buffer-buffer delays
    Bundles GetBundlesScheduleASAP( ql::arch::resource_manager_t & rm, ql::quantum_platform & platform, bool verbose=false )
    {
        if(verbose) COUT("RC Scheduling ASAP to get bundles ...");
        Bundles bundles;
        ListDigraph::NodeMap<size_t> cycle(graph);
        std::vector<ListDigraph::Node> order;
        ScheduleASAP(cycle, order, rm, platform, verbose);

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
            TotalCycles =  cycle[ *( order.begin() ) ];
        }

        for(size_t currCycle = 0; currCycle<=TotalCycles; ++currCycle)
        {
            auto it = insInAllCycles.find(currCycle);
            if( it != insInAllCycles.end() )
            {
                Bundle abundle;
                size_t bduration = 0;
                auto nInsThisCycle = insInAllCycles[currCycle].size();
                for(size_t i=0; i<nInsThisCycle; ++i )
                {
                    ParallelSection aparsec;
                    auto & ins = insInAllCycles[currCycle][i];
                    aparsec.push_back(ins);
                    abundle.ParallelSections.push_back(aparsec);
                    size_t iduration = ins->duration;
                    bduration = std::max(bduration, iduration);
                }
                abundle.start_cycle = currCycle;
                abundle.duration_in_cycles = std::ceil(static_cast<float>(bduration)/cycle_time);
                bundles.push_back(abundle);
            }
        }


        // insert buffer - buffer delays
        DOUT("buffer-buffer delay insertion ... ");
        std::vector<std::string> operations_prev_bundle;
        size_t buffer_cycles_accum = 0;
        for(auto & abundle : bundles)
        {
            std::vector<std::string> operations_curr_bundle;
            for( auto secIt = abundle.ParallelSections.begin(); secIt != abundle.ParallelSections.end(); ++secIt )
            {
                for(auto insIt = secIt->begin(); insIt != secIt->end(); ++insIt )
                {
                    auto & id = (*insIt)->name;
                    std::string op_type("none");
                    if ( !platform.instruction_settings[id]["type"].is_null() )
                    {
                        op_type = platform.instruction_settings[id]["type"];
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

        if(verbose) COUT("RC Scheduling ASAP to get bundles [DONE]");
        return bundles;
    }

};

#endif
