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

public:
    Scheduler(): instruction(graph), name(graph), weight(graph),
        cause(graph), depType(graph), dist(graph) {}

    void Init( size_t nQubits, ql::circuit& ckt, ql::quantum_platform platform, bool verbose=false)
    {
        cycle_time=platform.cycle_time;

        // populate buffer map
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
                    buffer_cycles_map[ bpair ] = size_t(platform.hardware_settings[bname]) / cycle_time;
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
        LastReaders.resize(nQubits);

        int srcID = graph.id(srcNode);
        vector<int> LastWriter(nQubits,srcID);

        for( auto ins : ckt )
        {
            // std::cout << "\nCurrent instruction : " << ins->qasm() << std::endl;

            // Add nodes
            ListDigraph::Node consNode = graph.addNode();
            int consID = graph.id(consNode);
            instruction[consNode] = ins;
            name[consNode] = ins->qasm();

            // Add edges
            size_t operandNo=0;
            auto operands = ins->operands;
            for( auto operand : operands )
            {
                // cout << "Operand is " << operand << std::endl;
                int prodID = LastWriter[operand];
                ListDigraph::Node prodNode = graph.nodeFromId(prodID);
                ListDigraph::Arc arc = graph.addArc(prodNode,consNode);
                if(prodID == srcID)
                    weight[arc] = 1; // TODO OR 0 as SOURCE is dummy node?
                else
                {
                    weight[arc] = (instruction[prodNode]->duration)/cycle_time;
                    // COUT("Case 1: " << name[prodNode] << " -> " << name[consNode] 
                    //                    << ", duration (ns) : " << instruction[prodNode]->duration 
                    //                    << ", weight: " << weight[arc] );
                }

                cause[arc] = operand;
                if(operandNo == 0)
                {
                    ReadersListType readers = LastReaders[operand];
                    for(auto & readerID : readers)
                    {
                        ListDigraph::Node readerNode = graph.nodeFromId(readerID);
                        ListDigraph::Arc arc1 = graph.addArc(readerNode,consNode);
                        if(prodID == srcID)
                            weight[arc1] = 1; // TODO OR 0 as SOURCE is dummy node?
                        else
                        {
                            weight[arc1] = (instruction[readerNode]->duration)/cycle_time;;
                            // COUT("Case 2: " << name[readerNode] << " -> " << name[consNode] 
                            //                    << ", duration (ns): " << instruction[readerNode]->duration 
                            //                    << ", weight: " << weight[arc1] );
                        }

                        cause[arc1] = operand;
                        depType[arc1] = RAR; // on control
                    }
                }

                if( operandNo == operands.size()-1 ) // Last operand is target, well Mostsly! TODO Fix it for other cases
                {   
                    depType[arc] = WAW;
                    LastWriter[operand] = consID;

                    // for WAR dependencies
                    ReadersListType readers = LastReaders[operand];
                    for(auto & readerID : readers)
                    {
                        ListDigraph::Node readerNode = graph.nodeFromId(readerID);
                        ListDigraph::Arc arc1 = graph.addArc(readerNode,consNode);
                        if(prodID == srcID)
                            weight[arc1] = 1; // TODO OR 0 as SOURCE is dummy node?
                        else
                        {
                            weight[arc1] = (instruction[readerNode]->duration)/cycle_time;;
                            // COUT("Case 3: " << name[readerNode] << " -> " << name[consNode] 
                            //                    << ", duration (ns): " << instruction[readerNode]->duration 
                            //                    << ", weight: " << weight[arc1] );
                        }

                        cause[arc1] = operand;
                        depType[arc1] = WAR;
                    }
                }
                else
                {
                    LastReaders[operand].push_back(consID);
                    depType[arc] = RAW;
                }
                operandNo++;
            } // end of operand for
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
                weight[arc] = 1; // TODO OR 0?
                cause[arc] = 0; // NA
                depType[arc] = RAW; // NA
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
                // << " , " << weight[arc]
                // << " , " << DepTypesNames[ depType[arc] ]
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
        // std::cout << "Performing Topological sort." << std::endl;
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
            // std::cout << "Scheduling " << name[*currNode] << std::endl;
            size_t currCycle=0;
            for( ListDigraph::InArcIt arc(graph,*currNode); arc != INVALID; ++arc )
            {
                ListDigraph::Node srcNode  = graph.source(arc);
                size_t srcCycle = cycle[srcNode];
                if(currCycle <= srcCycle)
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
        ListDigraph::NodeMap<size_t> cycle(graph);
        std::vector<ListDigraph::Node> order;
        ScheduleASAP(cycle,order);
        PrintDot1_(false,true,cycle,order,dotout);

        dotout.close();
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

        ListDigraph::NodeMap<size_t> cycle(graph);
        std::vector<ListDigraph::Node> order;
        ScheduleASAP(cycle, order, verbose);
        if(verbose) COUT("Printing Scheduled QASM in " << qcfname);

        typedef std::vector<std::string> insInOneCycle;
        std::map<size_t,insInOneCycle> insInAllCycles;

        std::vector<ListDigraph::Node>::reverse_iterator it;
        for ( it = order.rbegin(); it != order.rend(); ++it)
        {
            insInAllCycles[ cycle[*it] ].push_back( name[*it] );
        }

        size_t TotalCycles = 0;
        if( ! order.empty() )
        {
            TotalCycles =  cycle[ *( order.begin() ) ];
        }

        for(size_t currCycle = 1; currCycle<TotalCycles; ++currCycle)
        {
            auto it = insInAllCycles.find(currCycle);
            if( it != insInAllCycles.end() )
            {
                auto nInsThisCycle = insInAllCycles[currCycle].size();
		fout << "{ "; 
                for(size_t i=0; i<nInsThisCycle; ++i )
                {
                    fout << insInAllCycles[currCycle][i];
                    if( i != nInsThisCycle - 1 ) // last instruction
                        fout << " | ";
                }
		fout << " }"; 
            }
            else
            {
                fout << "   qwait 1";
            }
            fout << endl;
        }

        fout.close();
    }

    std::string GetQASMScheduledASAP(bool verbose=false)
    {
        std::stringstream ss;

        ListDigraph::NodeMap<size_t> cycle(graph);
        std::vector<ListDigraph::Node> order;
        ScheduleASAP(cycle, order, verbose);

        typedef std::vector<std::string> insInOneCycle;
        std::map<size_t,insInOneCycle> insInAllCycles;

        std::vector<ListDigraph::Node>::reverse_iterator it;
        for ( it = order.rbegin(); it != order.rend(); ++it)
        {
            insInAllCycles[ cycle[*it] ].push_back( name[*it] );
        }

        size_t TotalCycles = 0;
        if( ! order.empty() )
        {
            TotalCycles =  cycle[ *( order.begin() ) ];
        }

        size_t empty_cycles=0;
        for(size_t currCycle = 1; currCycle<TotalCycles; ++currCycle)
        {
            auto it = insInAllCycles.find(currCycle);
            if( it != insInAllCycles.end() )
            {
                if( empty_cycles > 0 )
                {
                    ss << "qwait " << empty_cycles << '\n';
                    empty_cycles=0;
                }

                auto nInsThisCycle = insInAllCycles[currCycle].size();
                for(size_t i=0; i<nInsThisCycle; ++i )
                {
                    ss << insInAllCycles[currCycle][i];
                    if( i != nInsThisCycle - 1 ) // last instruction
                        ss << " | ";
                }
                ss << '\n';
            }
            else
            {
                ++empty_cycles;
            }
        }
        return ss.str();
    }

    // without rc
    void ScheduleALAP(ListDigraph::NodeMap<size_t> & cycle, std::vector<ListDigraph::Node> & order, bool verbose=false)
    {
        if(verbose) COUT("Performing ALAP Scheduling");
        TopologicalSort(order);

        std::vector<ListDigraph::Node>::iterator currNode = order.begin();
        cycle[*currNode]=MAX_CYCLE; // src dummy in cycle 0
        ++currNode;
        while(currNode != order.end() )
        {
            // std::cout << "Scheduling " << name[*currNode] << std::endl;
            size_t currCycle=MAX_CYCLE;
            for( ListDigraph::OutArcIt arc(graph,*currNode); arc != INVALID; ++arc )
            {
                ListDigraph::Node targetNode  = graph.target(arc);
                size_t targetCycle = cycle[targetNode];
                if(currCycle >= targetCycle)
                {
                    currCycle = targetCycle - weight[arc];
                }
            }
            cycle[*currNode]=currCycle;
            ++currNode;
        }
    }


    /*
    // with rc but without buffer-buffer delays and latency compensation
    void ScheduleALAP1( ListDigraph::NodeMap<size_t> & cycle, std::vector<ListDigraph::Node> & order,
                       ql::arch::resource_manager_t & rm, bool verbose=false )
    {
        if(verbose) COUT("Performing RC ALAP Scheduling");
        TopologicalSort(order);

        std::vector<ListDigraph::Node>::iterator currNode = order.begin();
        cycle[*currNode]=MAX_CYCLE; // src dummy in cycle 0
        ++currNode;
        while(currNode != order.end() )
        {
            size_t currCycle=MAX_CYCLE;
            for( ListDigraph::OutArcIt arc(graph,*currNode); arc != INVALID; ++arc )
            {
                ListDigraph::Node targetNode  = graph.target(arc);
                size_t targetCycle = cycle[targetNode];
                if( currCycle >= targetCycle )
                {
                    currCycle = targetCycle - weight[arc];
                }
            }

            while(currCycle > 0)
            {
                std::cout << "Trying to scheduling: " << name[*currNode] << "  in cycle: " << currCycle << std::endl;
                if( rm.available(currCycle, instruction[*currNode]) )
                {
                    std::cout << "Resource available, Scheduled. \n";
                    // auto prevNode = std::prev(currNode);
                    // if( prevNode != order.end() )
                    //     std::cout << "Previous node: " << name[*prevNode] << std::endl;
                    rm.reserve(currCycle, instruction[*currNode] );
                    cycle[*currNode]=currCycle;
                    break;
                }
                else
                {
                    std::cout << "Resource not available, trying again ...\n";
                    --currCycle;
                }
            }
            if(currCycle <= 0)
            {
                COUT("Error: could not find schedule");
                throw ql::exception("[x] Error : could not find schedule !",false);
            }
            std::cout << std::endl;
            ++currNode;
        }
        if(verbose) COUT("Performing RC ALAP Scheduling [Done].");
    }
    */

    /*
    // with rc, buffer-buffer delays (but only for dependent edges) and latency compensation
    void ScheduleALAP2( ListDigraph::NodeMap<size_t> & cycle, std::vector<ListDigraph::Node> & order,
                       ql::arch::resource_manager_t & rm, ql::quantum_platform & platform, bool verbose=false )
    {
        if(verbose) COUT("Performing RC ALAP Scheduling");
        TopologicalSort(order);

        std::vector<std::string> buffer_names = {"none", "mw", "flux", "readout"};
        std::map< std::pair<std::string,std::string>, size_t> buffer_cycles_map;
        for(auto & buf1 : buffer_names)
        {
            for(auto & buf2 : buffer_names)
            {
                auto bpair = std::pair<std::string,std::string>(buf1,buf2);
                auto bname = buf1+ "_" + buf2 + "_buffer";
                if( buf1 == "none" || buf2 == "none" )
                    buffer_cycles_map[ bpair ] = 0;
                else
                    buffer_cycles_map[ bpair ] = size_t(platform.hardware_settings[bname]) / cycle_time;
                DOUT("Initializing " << bname << ": "<< buffer_cycles_map[bpair]);
            }
        }

        ListDigraph::NodeMap<std::string> operation_type(graph);

        std::vector<ListDigraph::Node>::iterator currNode = order.begin();
        cycle[*currNode]=MAX_CYCLE; // src dummy in cycle 0
        operation_type[*currNode] = "none";
        ++currNode;
        while(currNode != order.end() )
        {
            DOUT("");
            auto & curr_ins = instruction[*currNode];
            auto & id = curr_ins->name;
            if ( !platform.instruction_settings[id]["type"].is_null() )
            {
                operation_type[*currNode] = (platform.instruction_settings[id]["type"]) ;
            }
            else
            {
                operation_type[*currNode] = "none";
            }

            std::string operation_name(id);
            if ( !platform.instruction_settings[id]["cc_light_instr"].is_null() )
            {
                operation_name = platform.instruction_settings[id]["cc_light_instr"];
            }

            size_t currCycle=MAX_CYCLE;
            size_t buffer_cycles = 0;
            for( ListDigraph::OutArcIt arc(graph,*currNode); arc != INVALID; ++arc )
            {
                ListDigraph::Node targetNode  = graph.target(arc);
                size_t targetCycle = cycle[targetNode];
                if( currCycle >= targetCycle )
                {
                    currCycle = targetCycle - weight[arc];
                }

                auto & curr_op_type = operation_type[*currNode];
                auto & prev_op_type = operation_type[targetNode];

                auto temp_buf_cycles = buffer_cycles_map[ std::pair<std::string,std::string>(curr_op_type, prev_op_type) ];
                buffer_cycles = std::max(temp_buf_cycles, buffer_cycles);
                DOUT("Curr node: " << name[*currNode] << ", type: " << curr_op_type);
                DOUT("Prev node: " << name[targetNode] << ", type: " << prev_op_type);
                DOUT(curr_op_type+ "_" + prev_op_type + "_buffer cycles to be inserted: " << buffer_cycles);
            }

            while(currCycle > 0)
            {
                DOUT("Trying to schedule: " << name[*currNode] << "  in cycle: " << currCycle);
                if( rm.available(currCycle, instruction[*currNode], operation_type[*currNode]) )
                {
                    DOUT("Resources available, Scheduled.");

                    rm.reserve(currCycle-buffer_cycles, curr_ins, operation_name);
                    cycle[*currNode]=currCycle-buffer_cycles;
                    break;
                }
                else
                {
                    DOUT("Resources not available, trying again ...");
                    --currCycle;
                }
            }
            if(currCycle <= 0)
            {
                EOUT("Could not find schedule");
                throw ql::exception("[x] Error : could not find schedule !",false);
            }
            ++currNode;
        }

        DOUT("\nPrinting ALAP Schedule before latency compensation");
        DOUT("Cycle   Cycle-simplified    Instruction");
        for ( auto it = order.begin(); it != order.end(); ++it)
        {
            DOUT( cycle[*it] << " :: " << MAX_CYCLE-cycle[*it] << "  <- " << name[*it] );
        }

        // latency compensation
        for ( auto it = order.begin(); it != order.end(); ++it)
        {
            auto & curr_ins = instruction[*it];
            auto & id = curr_ins->name;
            long latency_cycles=0;
            if ( !platform.instruction_settings[id]["latency"].is_null() )
            {
                float latency_ns = platform.instruction_settings[id]["latency"];
                latency_cycles = (std::ceil( std::abs(latency_ns) / cycle_time)) * ql::utils::sign_of(latency_ns);
            }
            cycle[*it] = cycle[*it] + latency_cycles;
            // DOUT( MAX_CYCLE-cycle[*it] << " <- " << name[*it] << latency_cycles );
        }

        // re-order
        std::sort
            (
                order.begin(),
                order.end(),
                [&](ListDigraph::Node & n1, ListDigraph::Node & n2) { return cycle[n1] > cycle[n2]; }
            );

        DOUT("\nPrinting ALAP Schedule after latency compensation");
        DOUT("Cycle   Cycle-simplified    Instruction");
        for ( auto it = order.begin(); it != order.end(); ++it)
        {
            DOUT( cycle[*it] << "     =     " << MAX_CYCLE-cycle[*it] << "        " << name[*it] );
        }

        if(verbose) COUT("Performing RC ALAP Scheduling [Done].");
    }
    */

    // with rc and latency compensation
    void ScheduleALAP( ListDigraph::NodeMap<size_t> & cycle, std::vector<ListDigraph::Node> & order,
                       ql::arch::resource_manager_t & rm, ql::quantum_platform & platform, bool verbose=false )
    {
        if(verbose) COUT("Performing RC ALAP Scheduling");
        TopologicalSort(order);

        std::vector<ListDigraph::Node>::iterator currNode = order.begin();
        cycle[*currNode]=MAX_CYCLE; // src dummy in cycle 0

        ++currNode;
        while(currNode != order.end() )
        {
            DOUT("");
            auto & curr_ins = instruction[*currNode];
            auto & id = curr_ins->name;
            std::string operation_name(id);
            if ( !platform.instruction_settings[id]["cc_light_instr"].is_null() )
            {
                operation_name = platform.instruction_settings[id]["cc_light_instr"];
            }

            size_t currCycle=MAX_CYCLE;
            for( ListDigraph::OutArcIt arc(graph,*currNode); arc != INVALID; ++arc )
            {
                ListDigraph::Node targetNode  = graph.target(arc);
                size_t targetCycle = cycle[targetNode];
                if( currCycle >= targetCycle )
                {
                    currCycle = targetCycle - weight[arc];
                }
            }

            while(currCycle > 0)
            {
                DOUT("Trying to schedule: " << name[*currNode] << "  in cycle: " << currCycle);
                if( rm.available(currCycle, instruction[*currNode], operation_name) )
                {
                    DOUT("Resources available, Scheduled.");

                    rm.reserve(currCycle, curr_ins, operation_name);
                    cycle[*currNode]=currCycle;
                    break;
                }
                else
                {
                    DOUT("Resources not available, trying again ...");
                    --currCycle;
                }
            }
            if(currCycle <= 0)
            {
                COUT("Error: could not find schedule");
                throw ql::exception("[x] Error : could not find schedule !",false);
            }
            ++currNode;
        }

        DOUT("\nPrinting ALAP Schedule before latency compensation");
        DOUT("Cycle   Cycle-simplified    Instruction");
        for ( auto it = order.begin(); it != order.end(); ++it)
        {
            DOUT( cycle[*it] << " :: " << MAX_CYCLE-cycle[*it] << "  <- " << name[*it] );
        }

        // latency compensation
        for ( auto it = order.begin(); it != order.end(); ++it)
        {
            auto & curr_ins = instruction[*it];
            auto & id = curr_ins->name;
            long latency_cycles=0;
            if ( !platform.instruction_settings[id]["latency"].is_null() )
            {
                float latency_ns = platform.instruction_settings[id]["latency"];
                latency_cycles = (std::ceil( std::abs(latency_ns) / cycle_time)) * ql::utils::sign_of(latency_ns);
            }
            cycle[*it] = cycle[*it] + latency_cycles;
            // DOUT( MAX_CYCLE-cycle[*it] << " <- " << name[*it] << latency_cycles );
        }

        // re-order
        std::sort
            (
                order.begin(),
                order.end(),
                [&](ListDigraph::Node & n1, ListDigraph::Node & n2) { return cycle[n1] > cycle[n2]; }
            );

        DOUT("\nPrinting ALAP Schedule after latency compensation");
        DOUT("Cycle   Cycle-simplified    Instruction");
        for ( auto it = order.begin(); it != order.end(); ++it)
        {
            DOUT( cycle[*it] << "     =     " << MAX_CYCLE-cycle[*it] << "        " << name[*it] );
        }

        if(verbose) COUT("Performing RC ALAP Scheduling [Done].");
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
                // << " , " << weight[arc]
                // << " , " << DepTypesNames[ depType[arc] ]
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

        ListDigraph::NodeMap<size_t> cycle(graph);
        std::vector<ListDigraph::Node> order;
        ScheduleALAP(cycle,order);
        if(verbose) COUT("Printing Scheduled QASM in " << qcfname);

        typedef std::vector<std::string> insInOneCycle;
        std::map<size_t,insInOneCycle> insInAllCycles;

        std::vector<ListDigraph::Node>::iterator it;
        for ( it = order.begin(); it != order.end(); ++it)
        {
            insInAllCycles[ MAX_CYCLE - cycle[*it] ].push_back( name[*it] );
        }

        size_t TotalCycles = 0;
        if( ! order.empty() )
        {
            TotalCycles =  MAX_CYCLE - cycle[ *( order.rbegin() ) ];
        }

        for(size_t currCycle = TotalCycles-1; currCycle>0; --currCycle)
        {
            auto it = insInAllCycles.find(currCycle);
            if( it != insInAllCycles.end() )
            {
                auto nInsThisCycle = insInAllCycles[currCycle].size();
		        fout << "{ "; 
                for(size_t i=0; i<nInsThisCycle; ++i )
                {
                    fout << insInAllCycles[currCycle][i];
                    if( i != nInsThisCycle - 1 ) // last instruction
                        fout << " | ";
                }
		        fout << " }"; 
            }
            else
            {
                fout << "   qwait 1";
            }
            fout << endl;
        }
        fout.close();
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

            curr_cycle+=delta;
        }
        return ssbundles.str();
    }

    // the following without nops
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
                abundle.duration_in_cycles = bduration/cycle_time;
                bundles.push_back(abundle);
            }
        }
        if(verbose) COUT("Scheduling ALAP to get bundles [DONE]");
        return bundles;
    }



    // the following inserts nops
    Bundles GetBundlesScheduleALAPWithNOPS(bool verbose=false)
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
            if( it != insInAllCycles.end() )
            {
                auto nInsThisCycle = insInAllCycles[currCycle].size();
                for(size_t i=0; i<nInsThisCycle; ++i )
                {
                    ParallelSection aparsec;
                    auto & ins = insInAllCycles[currCycle][i];
                    aparsec.push_back(ins);
                    abundle.ParallelSections.push_back(aparsec);
                }
            }
            else
            {
                // insert empty bundle
                ParallelSection aparsec;
                auto ins = new ql::nop();
                aparsec.push_back(ins);
                abundle.ParallelSections.push_back(aparsec);
            }
            bundles.push_back(abundle);
        }

        if(verbose) COUT("Scheduling ALAP to get bundles [DONE]");
        return bundles;
    }

    // the following without nops but with rc
    Bundles GetBundlesScheduleALAP( ql::arch::resource_manager_t & rm, ql::quantum_platform & platform, bool verbose=false )
    {
        if(verbose) COUT("RC Scheduling ALAP to get bundles ...");
        Bundles bundles;
        ListDigraph::NodeMap<size_t> cycle(graph);
        std::vector<ListDigraph::Node> order;
        ScheduleALAP(cycle, order, rm, platform, verbose);

        typedef std::vector<ql::gate*> insInOneCycle;
        std::map<size_t,insInOneCycle> insInAllCycles;

        std::vector<ListDigraph::Node>::iterator it;
        for ( it = order.begin(); it != order.end(); ++it)
        {
            if( instruction[*it]->type() != ql::gate_type_t::__dummy_gate__ )
            {
                insInAllCycles[ MAX_CYCLE - cycle[*it] ].push_back( instruction[*it] );
                // insInAllCycles[ cycle[*it] ].push_back( instruction[*it] );
            }
        }

        size_t TotalCycles = 0;
        if( ! order.empty() )
        {
            TotalCycles =  MAX_CYCLE - cycle[ *( order.rbegin() ) ];
            // TotalCycles =  cycle[ *( order.rbegin() ) ];
        }

        for(int currCycle = TotalCycles; currCycle>=0; --currCycle)
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
                abundle.start_cycle = TotalCycles - currCycle;
                abundle.duration_in_cycles = bduration/cycle_time;
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

        if(verbose) COUT("RC Scheduling ALAP to get bundles [DONE]");
        return bundles;
    }

};

#endif
