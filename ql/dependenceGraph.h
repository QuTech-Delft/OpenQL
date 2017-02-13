/*
 * Author: Imran Ashraf
 */

#ifndef DEPENDENCEGRAPH_H
#define DEPENDENCEGRAPH_H

#include <lemon/list_graph.h>
#include <lemon/lgf_reader.h>
#include <lemon/lgf_writer.h>
#include <lemon/dijkstra.h>
#include <lemon/connectivity.h>

#include "gate.h"
#include "circuit.h"

using namespace std;
using namespace lemon;

enum DepTypes{RAW,WAW,WAR};
const string DepTypesNames[] = {"RAW", "WAW", "WAR"};

class DependGraph
{
private:
    ListDigraph graph;

    ListDigraph::NodeMap<ql::gate*> instruction;
    ListDigraph::NodeMap<string> name;
    ListDigraph::ArcMap<int> weight;
    ListDigraph::ArcMap<int> weightNeg;
    ListDigraph::ArcMap<int> cause;
    ListDigraph::ArcMap<int> depType;

    ListDigraph::NodeMap<double> dist;
    Path<ListDigraph> p;

    ListDigraph::Node s, t;

public:
    DependGraph(): instruction(graph), name(graph), weight(graph), weightNeg(graph),
        cause(graph), depType(graph), dist(graph) {}

    void Init( ql::circuit& ckt, size_t nqubits)
    {
        size_t nQubits = nqubits;
        // std::cout << "nQubits : " << nQubits << std::endl;

        // add dummy source node
        ListDigraph::Node srcNode = graph.addNode();
        instruction[srcNode] = new ql::dummy();
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
                    weight[arc] = 1;

                weightNeg[arc] = 10 - weight[arc];
                cause[arc] = operand;
                if( operandNo == operands.size()-1 ) // Last operand is target, well Mostsly! TODO Fix it for other case
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
                            weight[arc1] = 1;

                        weightNeg[arc1] = 10 - weight[arc1];
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
        instruction[targetNode] = new ql::dummy();;
        name[targetNode] = instruction[targetNode]->qasm();
        t=targetNode;

        OutDegMap<ListDigraph> outDeg(graph);
        for (ListDigraph::NodeIt n(graph); n != INVALID; ++n)
        {
            if( outDeg[n] == 0 && n!=targetNode )
            {
                ListDigraph::Arc arc = graph.addArc(n,targetNode);
                weight[arc] = 1; // TODO OR 0?
                weightNeg[arc] = 10 - weight[arc];
                cause[arc] = 0; // NA
                depType[arc] = RAW; // NA
            }
        }
    }

    void Print()
    {
        std::cout << "Printing Dependence Graph " << std::endl;
        digraphWriter(graph).
        nodeMap("name", name).
        arcMap("cause", cause).
        arcMap("weight", weight).
        arcMap("depType", depType).
        node("source", s).
        node("target", t).
        run();
    }

    void PrintMatrix()
    {
        std::cout << "Printing Dependence Graph as Matrix" << std::endl;
        ofstream fout;
        // OpenOutFile("dependenceMatrix.dat",fout);
        fout.open( "dependenceMatrix.dat", ios::binary);
        if ( fout.fail() )
        {
            std::cout << "Error opening file" << std::endl;
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

    void PrintDot_(
                bool WithCritical,
                bool WithCycles,
                ListDigraph::NodeMap<size_t> & cycle,
                std::vector<ListDigraph::Node> & order,
                ofstream& dotout
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
                //<< " , " << weight[arc]
                //<< " , " << DepTypesNames[ depType[arc] ]
                <<"\""
                << " " << EdgeStyle << " "
                << "]"
                << endl;
        }

        dotout << "}" << endl;
    }

    void PrintDot()
    {
        std::cout << "Printing Dependence Graph in DOT" << std::endl;
        ofstream dotout;
        // OpenOutFile("dependenceGraph.dot",dotout);
        dotout.open( "dependenceGraph.dot", ios::binary);
        if ( dotout.fail() )
        {
            std::cout << "Error opening file" << std::endl;
            return;
        }

        ListDigraph::NodeMap<size_t> cycle(graph);
        std::vector<ListDigraph::Node> order;
        PrintDot_(false, false, cycle, order, dotout);
        dotout.close();
    }

    void FindShortestPath()
    {
        dijkstra(graph, weight).distMap(dist).path(p).run(s,t);

        std::cout << std::endl << "Printing distances from sources " << std::endl;
        std::cout << "id   Name     Distance" << std::endl;
        for (ListDigraph::NodeIt n(graph); n != INVALID; ++n)
        {
            int nid = graph.id(n);
            string nodeName = name[n];
            std::cout  << nid << "    " << nodeName <<"    " << dist[n] << std::endl;
        }

        std::cout << "Number of nodes in the shortest path = " << p.length() << std::endl;
        std::cout << "Total distance of shortest path = " << dist[t] << std::endl;
        std::cout << "Shortest path : ";
        PathNodeIt< Path<ListDigraph> > nit( graph, p);
        for ( ; nit != INVALID; ++nit)
        {
            if( graph.id(nit)  == graph.id(s) )
                std::cout  << name[nit];
            else
                std::cout  << " -> " << name[nit];
        }
        std::cout << std::endl;
    }

    void FindLongestPath()
    {
        dijkstra(graph, weightNeg).distMap(dist).path(p).run(s,t);

        std::cout << std::endl << "Printing distances from sources " << std::endl;
        std::cout << "id   Name     Distance" << std::endl;
        for (ListDigraph::NodeIt n(graph); n != INVALID; ++n)
        {
            int nid = graph.id(n);
            string nodeName = name[n];
            std::cout  << nid << "    " << nodeName <<"    " << dist[n] << std::endl;
        }

        std::cout << "Number of nodes in the longest path = " << p.length() << std::endl;
        std::cout << "Total distance of longest path = " << dist[t] << std::endl;
        std::cout << "Longest path : ";
        PathNodeIt< Path<ListDigraph> > nit( graph, p);
        for ( ; nit != INVALID; ++nit)
        {
            if( graph.id(nit)  == graph.id(s) )
                std::cout  << name[nit];
            else
                std::cout  << " -> " << name[nit];
        }
        std::cout << std::endl;
    }

    void TopologicalSort(std::vector<ListDigraph::Node> & order)
    {
        // std::cout << "Performing Topological sort." << std::endl;
        ListDigraph::NodeMap<int> rorder(graph);
        if( !dag(graph) )
            std::cout << "This digraph is not a DAG." << std::endl;

        topologicalSort(graph, rorder);

#ifdef DEBUG
        for (ListDigraph::ArcIt a(graph); a != INVALID; ++a)
        {
            if( rorder[graph.source(a)] > rorder[graph.target(a)] )
                std::cout << "Wrong topologicalSort()" << std::endl;
        }
#endif

        for ( ListDigraph::NodeMap<int>::MapIt it(rorder); it != INVALID; ++it)
        {
            order.push_back(it);
        }
    }

    void PrintTopologicalOrder()
    {
        std::vector<ListDigraph::Node> order;
        TopologicalSort(order);

        std::cout << "Printing nodes in Topological order" << std::endl;
        for ( std::vector<ListDigraph::Node>::reverse_iterator it = order.rbegin(); it != order.rend(); ++it)
        {
            std::cout << name[*it] << std::endl;
        }
    }

    void ScheduleASAP(ListDigraph::NodeMap<size_t> & cycle, std::vector<ListDigraph::Node> & order)
    {
        std::cout << "Performing ASAP Scheduling" << std::endl;
        TopologicalSort(order);

        std::vector<ListDigraph::Node>::reverse_iterator currNode;
        for ( currNode = order.rbegin(); currNode != order.rend(); ++currNode)
        {
            //cout << "Scheduling " << name[*currNode] << endl;
            size_t currCycle=0;
            ArcLookUp<ListDigraph> lookup(graph);
            std::vector<ListDigraph::Node>::reverse_iterator prevNode;
            for( prevNode=order.rbegin(); prevNode != currNode; ++prevNode)
            {
                if( lookup(*prevNode, *currNode) != INVALID)
                {
                    if( currCycle <= cycle[*prevNode] )
                        currCycle = cycle[*prevNode] + 1;
                }
            }
            cycle[*currNode]=currCycle;
        }
    }

    void PrintScheduleASAP()
    {
        ListDigraph::NodeMap<size_t> cycle(graph);
        std::vector<ListDigraph::Node> order;
        ScheduleASAP(cycle,order);

        std::cout << "\nPrinting ASAP Schedule" << std::endl;
        std::cout << "Cycle <- Instruction " << std::endl;
        std::vector<ListDigraph::Node>::reverse_iterator it;
        for ( it = order.rbegin(); it != order.rend(); ++it)
        {
            std::cout << cycle[*it] << "     <- " <<  name[*it] << std::endl;
        }
    }

    void PrintDotScheduleASAP()
    {
        std::cout << "Printing Scheduled Graph in scheduledGraph.dot" << std::endl;
        ofstream dotout;
        dotout.open( "scheduledGraph.dot", ios::binary);
        if ( dotout.fail() )
        {
            std::cout << "Error opening file" << std::endl;
            return;
        }

        ListDigraph::NodeMap<size_t> cycle(graph);
        std::vector<ListDigraph::Node> order;
        ScheduleASAP(cycle,order);
        PrintDot_(false,true,cycle,order,dotout);

        dotout.close();
    }

    void PrintScheduledQASM()
    {
        std::cout << "Printing Scheduled QASM in scheduled.qc" << std::endl;
        ofstream fout;
        fout.open( "scheduled.qc", ios::binary);
        if ( fout.fail() )
        {
            std::cout << "Error opening file" << std::endl;
            return;
        }

        ListDigraph::NodeMap<size_t> cycle(graph);
        std::vector<ListDigraph::Node> order;
        ScheduleASAP(cycle,order);

        typedef std::vector<std::string> insInOneCycle;
        std::map<size_t,insInOneCycle> insInAllCycles;

        std::vector<ListDigraph::Node>::reverse_iterator it;
        for ( it = order.rbegin(); it != order.rend(); ++it)
        {
            insInAllCycles[ cycle[*it] ].push_back( name[*it] );
        }

        size_t TotalCycles = insInAllCycles.size();
        for(size_t c=1; c<TotalCycles-1;  ++c )
        {
            for(size_t i=0; i<insInAllCycles[c].size(); ++i )
            {
                fout << insInAllCycles[c][i];
                if( i != insInAllCycles[c].size() - 1 ) // last instruction
                    fout << " | ";
            }
            fout << std::endl;
        }

        fout.close();
    }

};

#endif
