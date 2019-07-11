/**
 * @file   cc_light_resource_manager.h
 * @date   09/2017
 * @author Imran Ashraf
 * @date   09/2018
 * @author Hans van Someren
 * @brief  Resource mangement for cc light platform
 */

#ifndef _cclight_resource_manager_h
#define _cclight_resource_manager_h

#include <fstream>
#include <vector>
#include <string>
#include <json.h>
#include <resource_manager.h>

using json = nlohmann::json;

namespace ql
{
namespace arch
{

class qubit_resource_t : public resource_t
{
public:
    qubit_resource_t* clone() const & { return new qubit_resource_t(*this);}
    qubit_resource_t* clone() && { return new qubit_resource_t(std::move(*this)); }

    // fwd: qubit q is busy till cycle=state[q], i.e. all cycles < state[q] it is busy, i.e. start_cycle must be >= state[q]
    // bwd: qubit q is busy from cycle=state[q], i.e. all cycles >= state[q] it is busy, i.e. start_cycle+duration must be <= state[q]
    std::vector<size_t> state;

    qubit_resource_t(const ql::quantum_platform & platform, scheduling_direction_t dir) : resource_t("qubits", dir)
    {
        // DOUT("... creating " << name << " resource");
        count = platform.resources[name]["count"];
        state.resize(count);
        for(size_t q=0; q<count; q++)
        {
            state[q] = (forward_scheduling == dir ? 0 : MAX_CYCLE);
        }
    }

    bool available(size_t op_start_cycle, ql::gate * ins, std::string & operation_name,
        std::string & operation_type, std::string & instruction_type, size_t operation_duration)
    {
        for( auto q : ins->operands )
        {
            if (forward_scheduling == direction)
            {
                DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle << "  qubit: " << q << " is busy till cycle : " << state[q]);
                if (op_start_cycle < state[q])
                {
                    DOUT("    " << name << " resource busy ...");
                    return false;
                }
            }
            else
            {
                DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle << "  qubit: " << q << " is busy from cycle : " << state[q]);
                if (op_start_cycle + operation_duration > state[q])
                {
                    DOUT("    " << name << " resource busy ...");
                    return false;
                }
            }
        }
        DOUT("    " << name << " resource available ...");
        return true;
    }

    void reserve(size_t op_start_cycle, ql::gate * ins, std::string & operation_name,
        std::string & operation_type, std::string & instruction_type, size_t operation_duration)
    {
        for( auto q : ins->operands )
        {
            state[q] = (forward_scheduling == direction ?  op_start_cycle + operation_duration : op_start_cycle );
            DOUT("reserved " << name << ". op_start_cycle: " << op_start_cycle << " qubit: " << q << " reserved till/from cycle: " << state[q]);
        }
    }
    ~qubit_resource_t() {}
};


class qwg_resource_t : public resource_t
{
public:
    qwg_resource_t* clone() const & { return new qwg_resource_t(*this);}
    qwg_resource_t* clone() && { return new qwg_resource_t(std::move(*this)); }

    std::vector<size_t> fromcycle;          // qwg is busy from cycle==fromcycle[qwg], inclusive
    std::vector<size_t> tocycle;            // qwg is busy to cycle==tocycle[qwg], not inclusive

    // there was a bug here: when qwg is busy from cycle i with operation x
    // then a new x is ok when starting at i or later
    // but a new y must wait until the last x has finished;
    // the bug was that a new x was always ok (so also when starting earlier than cycle i)

    std::vector<std::string> operations;    // with operation_name==operations[qwg]
    std::map<size_t,size_t> qubit2qwg;      // on qwg==qubit2qwg[q]

    qwg_resource_t(const ql::quantum_platform & platform, scheduling_direction_t dir) : 
        resource_t("qwgs", dir)
    {
        // DOUT("... creating " << name << " resource");
        count = platform.resources[name]["count"];
        fromcycle.resize(count);
        tocycle.resize(count);
        operations.resize(count);

        for(size_t i=0; i<count; i++)
        {
            fromcycle[i] = (forward_scheduling == dir ? 0 : MAX_CYCLE);
            tocycle[i] = (forward_scheduling == dir ? 0 : MAX_CYCLE);
            operations[i] = "";
        }
        auto & constraints = platform.resources[name]["connection_map"];
        for (json::const_iterator it = constraints.begin(); it != constraints.end(); ++it)
        {
            // COUT(it.key() << " : " << it.value() );
            size_t qwgNo = stoi( it.key() );
            auto & connected_qubits = it.value();
            for(auto & q : connected_qubits)
                qubit2qwg[q] = qwgNo;
        }
    }

    bool available(size_t op_start_cycle, ql::gate * ins, std::string & operation_name,
        std::string & operation_type, std::string & instruction_type, size_t operation_duration)
    {
        bool is_mw = (operation_type == "mw");
        if( is_mw )
        {
            for( auto q : ins->operands )
            {
                DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle << "  qwg: " << qubit2qwg[q] << " is busy from cycle: " << fromcycle[ qubit2qwg[q] ] << " to cycle: " << tocycle[qubit2qwg[q]] << " for operation: " << operations[ qubit2qwg[q] ]);
                if (forward_scheduling == direction)
                {
                    if ( op_start_cycle < fromcycle[ qubit2qwg[q] ]
                    || ( op_start_cycle < tocycle[qubit2qwg[q]] && operations[ qubit2qwg[q] ] != operation_name ) )
                    {
                        DOUT("    " << name << " resource busy ...");
                        return false;
                    }
                }
                else
                {
                    if ( op_start_cycle + operation_duration > tocycle[ qubit2qwg[q] ]
                    || ( op_start_cycle + operation_duration > fromcycle[qubit2qwg[q]] && operations[ qubit2qwg[q] ] != operation_name ) )
                    {
                        DOUT("    " << name << " resource busy ...");
                        return false;
                    }
                }
            }
            DOUT("    " << name << " resource available ...");
        }
        return true;
    }

    void reserve(size_t op_start_cycle, ql::gate * ins, std::string & operation_name,
        std::string & operation_type, std::string & instruction_type, size_t operation_duration)
    {
        bool is_mw = (operation_type == "mw");
        if( is_mw )
        {
            for( auto q : ins->operands )
            {
                if (forward_scheduling == direction)
                {
                    if (operations[ qubit2qwg[q] ] == operation_name)
                    {
                        tocycle[ qubit2qwg[q] ]  = std::max( tocycle[qubit2qwg[q]], op_start_cycle + operation_duration);
                    }
                    else
                    {
                        fromcycle[ qubit2qwg[q] ]  = op_start_cycle;
                        tocycle[ qubit2qwg[q] ]  = op_start_cycle + operation_duration;
                        operations[ qubit2qwg[q] ] = operation_name;
                    }
                }
                else
                {
                    if (operations[ qubit2qwg[q] ] == operation_name)
                    {
                        fromcycle[ qubit2qwg[q] ]  = std::min( fromcycle[qubit2qwg[q]], op_start_cycle);
                    }
                    else
                    {
                        fromcycle[ qubit2qwg[q] ]  = op_start_cycle;
                        tocycle[ qubit2qwg[q] ]  = op_start_cycle + operation_duration;
                        operations[ qubit2qwg[q] ] = operation_name;
                    }
                }
                DOUT("reserved " << name << ". op_start_cycle: " << op_start_cycle << " qwg: " << qubit2qwg[q] << " reserved from cycle: " << fromcycle[ qubit2qwg[q] ] << " to cycle: " << tocycle[qubit2qwg[q]] << " for operation: " << operations[ qubit2qwg[q] ]);
            }
        }
    }
    ~qwg_resource_t() {}
};

class meas_resource_t : public resource_t
{
public:
    meas_resource_t* clone() const & { return new meas_resource_t(*this);}
    meas_resource_t* clone() && { return new meas_resource_t(std::move(*this)); }

    std::vector<size_t> fromcycle;  // last measurement start cycle
    std::vector<size_t> tocycle;    // is busy till cycle
    std::map<size_t,size_t> qubit2meas;

    meas_resource_t(const ql::quantum_platform & platform, scheduling_direction_t dir) : 
        resource_t("meas_units", dir)
    {
        // DOUT("... creating " << name << " resource");
        count = platform.resources[name]["count"];
        fromcycle.resize(count);
        tocycle.resize(count);

        for(size_t i=0; i<count; i++)
        {
            fromcycle[i] = (forward_scheduling == dir ? 0 : MAX_CYCLE);
            tocycle[i] = (forward_scheduling == dir ? 0 : MAX_CYCLE);
        }
        auto & constraints = platform.resources[name]["connection_map"];
        for (json::const_iterator it = constraints.begin(); it != constraints.end(); ++it)
        {
            // COUT(it.key() << " : " << it.value());
            size_t measUnitNo = stoi( it.key() );
            auto & connected_qubits = it.value();
            for(auto & q : connected_qubits)
                qubit2meas[q] = measUnitNo;
        }
    }

    bool available(size_t op_start_cycle, ql::gate * ins, std::string & operation_name,
        std::string & operation_type, std::string & instruction_type, size_t operation_duration)
    {
        bool is_measure = (operation_type == "readout");
        if( is_measure )
        {
            for(auto q : ins->operands)
            {
                DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle << "  meas: " << qubit2meas[q] << " is busy from cycle: " << fromcycle[ qubit2meas[q] ] << " to cycle: " << tocycle[qubit2meas[q]] );
                if (forward_scheduling == direction)
                {
	                if( op_start_cycle != fromcycle[ qubit2meas[q] ] )
	                {
	                    // If current measurement on same measurement-unit does not start in the
	                    // same cycle, then it should wait for current measurement to finish
	                    if( op_start_cycle < tocycle[ qubit2meas[q] ] )
	                    {
	                        DOUT("    " << name << " resource busy ...");
	                        return false;
	                    }
	                }
                }
                else
                {
	                if( op_start_cycle != fromcycle[ qubit2meas[q] ] )
	                {
	                    // If current measurement on same measurement-unit does not start in the
	                    // same cycle, then it should wait until it would finish at start of or earlier than current measurement
	                    if( op_start_cycle + operation_duration > fromcycle[ qubit2meas[q] ] )
	                    {
	                        DOUT("    " << name << " resource busy ...");
	                        return false;
	                    }
	                }
                }
            }
            DOUT("    " << name << " resource available ...");
        }
        return true;
    }

    void reserve(size_t op_start_cycle, ql::gate * ins, std::string & operation_name,
        std::string & operation_type, std::string & instruction_type, size_t operation_duration)
    {
        bool is_measure = (operation_type == "readout");
        if( is_measure )
        {
            for(auto q : ins->operands)
            {
                fromcycle[ qubit2meas[q] ] = op_start_cycle;
                tocycle[ qubit2meas[q] ] = op_start_cycle + operation_duration;
                DOUT("reserved " << name << ". op_start_cycle: " << op_start_cycle << " meas: " << qubit2meas[q] << " reserved from cycle: " << fromcycle[ qubit2meas[q] ] << " to cycle: " << tocycle[qubit2meas[q]] );
            }
        }
    }
    ~meas_resource_t() {}
};

class edge_resource_t : public resource_t
{
public:
    edge_resource_t* clone() const & { return new edge_resource_t(*this);}
    edge_resource_t* clone() && { return new edge_resource_t(std::move(*this)); }

    // fwd: edge is busy till cycle=state[edge], i.e. all cycles < state[edge] it is busy, i.e. start_cycle must be >= state[edge]
    // bwd: edge is busy from cycle=state[edge], i.e. all cycles >= state[edge] it is busy, i.e. start_cycle+duration must be <= state[edge]
    std::vector<size_t> state;
    typedef std::pair<size_t,size_t> qubits_pair_t;
    std::map< qubits_pair_t, size_t > qubits2edge;
    std::map<size_t, std::vector<size_t> > edge2edges;

    edge_resource_t(const ql::quantum_platform & platform, scheduling_direction_t dir) : resource_t("edges", dir)
    {
        // DOUT("... creating " << name << " resource");
        count = platform.resources[name]["count"];
        state.resize(count);

        for(size_t i=0; i<count; i++)
        {
            state[i] = (forward_scheduling == dir ? 0 : MAX_CYCLE);
        }

        for( auto & anedge : platform.topology["edges"] )
        {
            size_t s = anedge["src"];
            size_t d = anedge["dst"];
            size_t e = anedge["id"];

            qubits_pair_t aqpair(s,d);
            auto it = qubits2edge.find(aqpair);
            if( it != qubits2edge.end() )
            {
                EOUT("re-defining edge " << s <<"->" << d << " !");
                throw ql::exception("[x] Error : re-defining edge !",false);
            }
            else
            {
                qubits2edge[aqpair] = e;
            }
        }

        auto & constraints = platform.resources[name]["connection_map"];
        for (json::const_iterator it = constraints.begin(); it != constraints.end(); ++it)
        {
            // COUT(it.key() << " : " << it.value() << "\n");
            size_t edgeNo = stoi( it.key() );
            auto & connected_edges = it.value();
            for(auto & e : connected_edges)
                edge2edges[e].push_back(edgeNo);
        }
    }

    bool available(size_t op_start_cycle, ql::gate * ins, std::string & operation_name,
        std::string & operation_type, std::string & instruction_type, size_t operation_duration)
    {
        auto gname = ins->name;
        bool is_flux = (operation_type == "flux");
        if( is_flux )
        {
            auto nopers = ins->operands.size();
            if(nopers == 1)
            {
                // single qubit flux operation does not reserve an edge resource
                DOUT(" available for single qubit flux operation: " << name);
            }
            else if (nopers == 2)
            {
                auto q0 = ins->operands[0];
                auto q1 = ins->operands[1];
                qubits_pair_t aqpair(q0, q1);
                auto it = qubits2edge.find(aqpair);
                if( it != qubits2edge.end() )
                {
                    auto edge_no = qubits2edge[aqpair];

                    DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle 
                        << ", edge: " << edge_no << " is busy till/from cycle : " << state[edge_no] 
                        << " for operation: " << ins->name);

                    std::vector<size_t> edges2check(edge2edges[edge_no]);
                    edges2check.push_back(edge_no);
                    for(auto & e : edges2check)
                    {
                        if (forward_scheduling == direction)
                        {
                            if( op_start_cycle < state[e] )
                            {
                                DOUT("    " << name << " resource busy ...");
                                return false;
                            }
                        }
                        else
                        {
                            if( op_start_cycle + operation_duration > state[e] )
                            {
                                DOUT("    " << name << " resource busy ...");
                                return false;
                            }
                        }
                    }
                    DOUT("    " << name << " resource available ...");
                }
                else
                {
                    FATAL("Use of illegal edge: " << q0 << "->" << q1 << " in operation: " << ins->name << " !");
                }
            }
            else
            {
                FATAL("Incorrect number of operands used in operation: " << ins->name << " !");
            }
        }
        return true;
    }

    void reserve(size_t op_start_cycle, ql::gate * ins, std::string & operation_name,
        std::string & operation_type, std::string & instruction_type, size_t operation_duration)
    {
        auto gname = ins->name;
        bool is_flux = (operation_type == "flux");
        if( is_flux )
        {
            auto nopers = ins->operands.size();
            if(nopers == 1)
            {
                // single qubit flux operation does not reserve an edge resource
            }
            else if (nopers == 2)
            {
                auto q0 = ins->operands[0];
                auto q1 = ins->operands[1];
                qubits_pair_t aqpair(q0, q1);
                auto edge_no = qubits2edge[aqpair];
                if (forward_scheduling == direction)
                {
                    state[edge_no] = op_start_cycle + operation_duration;
                    for(auto & e : edge2edges[edge_no])
                    {
                        state[e] = op_start_cycle + operation_duration;
                    }
                }
                else
                {
                    state[edge_no] = op_start_cycle;
                    for(auto & e : edge2edges[edge_no])
                    {
                        state[e] = op_start_cycle;
                    }
                }
                DOUT("reserved " << name << ". op_start_cycle: " << op_start_cycle 
                    << " edge: " << edge_no << " reserved till cycle: " << state[ edge_no ] 
                    << " for operation: " << ins->name);
            }
	    else
            {
                FATAL("Incorrect number of operands used in operation: " << ins->name << " !");
            }
        }
    }
    ~edge_resource_t() {}
};

// A two-qubit flux gate lowers the frequency of its source qubit to get near the freq of its target qubit.
// Any two qubits which have near frequencies execute a two-qubit flux gate.
// To prevent any neighbor qubit of the source qubit that has the same frequency as the target qubit
// to interact as well, those neighbors must have their frequency detuned (lowered out of the way).
// A detuned qubit cannot execute a single-qubit rotation.
// An edge is a pair of qubits which can execute a two-qubit flux gate.
// The detuned_qubits resource describes for each edge doing a two-qubit gate which qubits it detunes.
//
// A two-qubit flux gate must check whether the qubits it would detune are not busy with a rotation.
// A one-qubit rotation gate must check whether its operand qubit is not detuned (busy with a flux gate).
//
// A two-qubit flux gate must set the qubits it would detune to detuned, busy with a flux gate.
// A one-qubit rotation gate must set its operand qubit to busy, busy with a rotation.
//
// The resource state machine maintains:
// - fromcycle[q]: qubit q is busy from cycle fromcycle[q]
// - tocycle[q]: to cycle tocycle[q] with an operation of the current operation type ...
// - operations[q]: a "flux" or a "mw" (note: "" is initial value different from these two)
// The fromcycle and tocycle are needed since a qubit can be busy with multiple "flux"s (i.e. being the detuned qubit for several "flux"s),
// so the second, third, etc. of these "flux"s can be scheduled in parallel to the first but not earlier than fromcycle[q],
// since till that cycle is was likely to be busy with "mw", which doesn't allow a "flux" in parallel. Similar for backward scheduling.
// The other members contain internal copies of the resource description and grid configuration of the json file.
class detuned_qubits_resource_t : public resource_t
{
public:
    detuned_qubits_resource_t* clone() const & { return new detuned_qubits_resource_t(*this);}
    detuned_qubits_resource_t* clone() && { return new detuned_qubits_resource_t(std::move(*this)); }

    std::vector<size_t> fromcycle;                              // qubit q is busy from cycle fromcycle[q]
    std::vector<size_t> tocycle;                                // till cycle tocycle[q]
    std::vector<std::string> operations;                        // with an operation of operation_type==operations[q]

    typedef std::pair<size_t,size_t> qubits_pair_t;
    std::map< qubits_pair_t, size_t > qubitpair2edge;           // map: pair of qubits to edge (from grid configuration)
    std::map<size_t, std::vector<size_t> > edge_detunes_qubits; // map: edge to vector of qubits that edge detunes (resource desc.)

    detuned_qubits_resource_t(const ql::quantum_platform & platform, scheduling_direction_t dir) : 
        resource_t("detuned_qubits", dir)
    {
        // DOUT("... creating " << name << " resource");
        count = platform.resources[name]["count"];
        fromcycle.resize(count);
        tocycle.resize(count);
        operations.resize(count);

        // initialize resource state machine to be free for all qubits
        for(size_t i=0; i<count; i++)
        {
            fromcycle[i] = (forward_scheduling == dir ? 0 : MAX_CYCLE);
            tocycle[i] = (forward_scheduling == dir ? 0 : MAX_CYCLE);
            operations[i] = "";
        }

        // initialize qubitpair2edge map from json description; this is a constant map
        for(auto & anedge : platform.topology["edges"])
        {
            size_t s = anedge["src"];
            size_t d = anedge["dst"];
            size_t e = anedge["id"];

            qubits_pair_t aqpair(s,d);
            auto it = qubitpair2edge.find(aqpair);
            if( it != qubitpair2edge.end() )
            {
                EOUT("re-defining edge " << s <<"->" << d << " !");
                throw ql::exception("[x] Error : re-defining edge !",false);
            }
            else
            {
                qubitpair2edge[aqpair] = e;
            }
        }

        // initialize edge_detunes_qubits map from json description; this is a constant map
        auto & constraints = platform.resources[name]["connection_map"];
        for (json::const_iterator it = constraints.begin(); it != constraints.end(); ++it)
        {
            // COUT(it.key() << " : " << it.value() << "\n");
            size_t edgeNo = stoi( it.key() );
            auto & detuned_qubits = it.value();
            for(auto & q : detuned_qubits)
                edge_detunes_qubits[edgeNo].push_back(q);
        }
    }

    // When a two-qubit flux gate, check whether the qubits it would detune are not busy with a rotation.
    // When a one-qubit rotation, check whether the qubit is not detuned (busy with a flux gate).
    bool available(size_t op_start_cycle, ql::gate * ins, std::string & operation_name,
        std::string & operation_type, std::string & instruction_type, size_t operation_duration)
    {
        auto gname = ins->name;
        bool is_flux = (operation_type == "flux");
        if( is_flux )
        {
            auto nopers = ins->operands.size();
            if (nopers == 1)
            {
                // single qubit flux operation does not reserve a detuned qubits resource
                DOUT(" available for single qubit flux operation: " << name);
            }
            else if (nopers == 2)
            {
	    	    auto q0 = ins->operands[0];
            	auto q1 = ins->operands[1];
            	qubits_pair_t aqpair(q0, q1);
            	auto it = qubitpair2edge.find(aqpair);
            	if( it != qubitpair2edge.end() )
            	{
                    auto edge_no = qubitpair2edge[aqpair];

                    for( auto & q : edge_detunes_qubits[edge_no])
                    {
                        DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle << ", edge: " << edge_no << " detuning qubit: " << q << " for operation: " << ins->name << " busy from: " << fromcycle[q] << " till: " << tocycle[q] << " with operation_type: " << operation_type);
                        if (forward_scheduling == direction)
                        {
                            if ( op_start_cycle < fromcycle[q]
                            || ( op_start_cycle < tocycle[q] && operations[q] != operation_type ) )
                            {
                                DOUT("    " << name << " resource busy for a two-qubit gate...");
                                return false;
                            }
                        }
                        else
                        {
                            if ( op_start_cycle + operation_duration > tocycle[q]
                            || ( op_start_cycle + operation_duration > fromcycle[q] && operations[q] != operation_type ) )
                            {
                                DOUT("    " << name << " resource busy for a two-qubit gate...");
                                return false;
                            }
                        }
                    }	// for over edges
                }   // edge found
                else
                {
                    EOUT("Use of illegal edge: " << q0 << "->" << q1 << " in operation: " << ins->name << " !");
                    throw ql::exception("[x] Error : Use of illegal edge"+std::to_string(q0)+"->"+std::to_string(q1)+"in operation:"+ins->name+" !",false);
                }
	        }   // nopers 1 or 2
	        else
            {
                FATAL("Incorrect number of operands used in operation: " << ins->name << " !");
            }
        }

        bool is_mw = (operation_type == "mw");
        if ( is_mw )
        {
            for( auto q : ins->operands )
            {
                DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle << ", qubit: " << q << " for operation: " << ins->name << " busy from: " << fromcycle[q] << " till: " << tocycle[q] << " with operation_type: " << operation_type);
                if (forward_scheduling == direction)
                {
                    if ( op_start_cycle < fromcycle[q])
                    {
                        DOUT("    " << name << " busy for rotation: op_start cycle " << op_start_cycle << " < fromcycle[" << q << "] " << fromcycle[q] );
                        return false;
                    }
                    if ( op_start_cycle < tocycle[q] && operations[q] != operation_type )
                    {
                        DOUT("    " << name << " busy for rotation with flux: op_start cycle " << op_start_cycle << " < tocycle[" << q << "] " << tocycle[q] );
                        return false;
                    }
                }
                else
                {
                    if ( op_start_cycle + operation_duration > tocycle[q])
                    {
                        DOUT("    " << name << " busy for rotation: op_start cycle " << op_start_cycle << " + duration > tocycle[" << q << "] " << tocycle[q] );
                        return false;
                    }
                    if ( op_start_cycle + operation_duration > fromcycle[q] && operations[q] != operation_type )
                    {
                        DOUT("    " << name << " busy for rotation with flux: op_start cycle " << op_start_cycle << " + duration > fromcycle[" << q << "] " << fromcycle[q] );
                        return false;
                    }
                }
            }
        }
        if (is_flux || is_mw) DOUT("    " << name << " resource available ...");
        return true;
    }

    // A two-qubit flux gate must set the qubits it would detune to detuned, busy with a flux gate.
    // A one-qubit rotation gate must set its operand qubit to busy, busy with a rotation.
    void reserve(size_t op_start_cycle, ql::gate * ins, std::string & operation_name,
        std::string & operation_type, std::string & instruction_type, size_t operation_duration)
    {
        auto gname = ins->name;
        bool is_flux = (operation_type == "flux");
        if( is_flux )
        {
            auto nopers = ins->operands.size();
            if (nopers == 1)
            {
                // single qubit flux operation does not reserve a detuned qubits resource
            }
            else if (nopers == 2)
            {
                auto q0 = ins->operands[0];
                auto q1 = ins->operands[1];
                qubits_pair_t aqpair(q0, q1);
                auto edge_no = qubitpair2edge[aqpair];

                for(auto & q : edge_detunes_qubits[edge_no])
                {
                    if (forward_scheduling == direction)
                    {
                        if (operations[q] == operation_type)
                        {
                            tocycle[q] = std::max( tocycle[q], op_start_cycle + operation_duration);
                            DOUT("reserving " << name << ". for qubit: " << q << " reusing cycle: " << fromcycle[q] << " to extending tocycle: " << tocycle[q] << " for old operation: " << ins->name);
                        }
                        else
                        {
                            fromcycle[q] = op_start_cycle;
                            tocycle[q] = op_start_cycle + operation_duration;
                            operations[q] = operation_type;
                            DOUT("reserving " << name << ". for qubit: " << q << " from fromcycle: " << fromcycle[q] << " to new tocycle: " << tocycle[q] << " for new operation: " << ins->name);
                        }
                    }
                    else
                    {
                        if (operations[q] == operation_type)
                        {
                            fromcycle[q] = std::min( fromcycle[q], op_start_cycle);
                            DOUT("reserving " << name << ". for qubit: " << q << " from extended cycle: " << fromcycle[q] << " reusing tocycle: " << tocycle[q] << " for old operation: " << ins->name);
                        }
                        else
                        {
                            fromcycle[q] = op_start_cycle;
                            tocycle[q] = op_start_cycle + operation_duration;
                            operations[q] = operation_type;
                            DOUT("reserving " << name << ". for qubit: " << q << " from new cycle: " << fromcycle[q] << " to tocycle: " << tocycle[q] << " for new operation: " << ins->name);
                        }
                    }
                    DOUT("reserved " << name << ". op_start_cycle: " << op_start_cycle << " edge: " << edge_no << " detunes qubit: " << q << " reserved from cycle: " << fromcycle[q] << " till cycle: " << tocycle[q] << " for operation: " << ins->name);
                }
            }
	    else
            {
                FATAL("Incorrect number of operands used in operation: " << ins->name << " !");
            }
        }
        bool is_mw = (operation_type == "mw");
        if ( is_mw )
        {
            for( auto q : ins->operands )
            {
                if (forward_scheduling == direction)
                {
                    if (operations[q] == operation_type)
                    {
                        tocycle[q] = std::max( tocycle[q], op_start_cycle + operation_duration);
                        DOUT("reserving " << name << ". for qubit: " << q << " reusing cycle: " << fromcycle[q] << " to extending tocycle: " << tocycle[q] << " for old operation: " << ins->name);
                    }
                    else
                    {
                        fromcycle[q] = op_start_cycle;
                        tocycle[q] = op_start_cycle + operation_duration;
                        operations[q] = operation_type;
                        DOUT("reserving " << name << ". for qubit: " << q << " from fromcycle: " << fromcycle[q] << " to new tocycle: " << tocycle[q] << " for new operation: " << ins->name);
                    }
                }
                else
                {
                    if (operations[q] == operation_type)
                    {
                        fromcycle[q] = std::min( fromcycle[q], op_start_cycle);
                        DOUT("reserving " << name << ". for qubit: " << q << " from extended cycle: " << fromcycle[q] << " reusing tocycle: " << tocycle[q] << " for old operation: " << ins->name);
                    }
                    else
                    {
                        fromcycle[q] = op_start_cycle;
                        tocycle[q] = op_start_cycle + operation_duration;
                        operations[q] = operation_type;
                        DOUT("reserving " << name << ". for qubit: " << q << " from new cycle: " << fromcycle[q] << " to tocycle: " << tocycle[q] << " for new operation: " << ins->name);
                    }
                }
                DOUT("... reserved " << name << ". op_start_cycle: " << op_start_cycle << " for qubit: " << q << " reserved from cycle: " << fromcycle[q] << " till cycle: " << tocycle[q] << " for operation: " << ins->name);
            }
        }
    }
    ~detuned_qubits_resource_t() {}
};

class cc_light_resource_manager_t : public resource_manager_t
{
public:
    cc_light_resource_manager_t() : resource_manager_t()
    {
        DOUT("Constructing virgin cc_light_resouce_manager_t");
    }


    cc_light_resource_manager_t(const ql::quantum_platform & platform, scheduling_direction_t dir) : resource_manager_t(platform, dir)
    {
        DOUT("Constructing inited resouce_manager_t");
        DOUT("New one for direction " << dir << " with no of resources : " << platform.resources.size() );
        for (json::const_iterator it = platform.resources.begin(); it != platform.resources.end(); ++it)
        {
            // COUT(it.key() << " : " << it.value() << "\n");
            std::string n = it.key();

            // DOUT("... about to create " << n << " resource");
            if( n == "qubits")
            {
                resource_t * ares = new qubit_resource_t(platform, dir);
                resource_ptrs.push_back( ares );
            }
            else if( n == "qwgs")
            {
                resource_t * ares = new qwg_resource_t(platform, dir);
                resource_ptrs.push_back( ares );
            }
            else if( n == "meas_units")
            {
                resource_t * ares = new meas_resource_t(platform, dir);
                resource_ptrs.push_back( ares );
            }
            else if( n == "edges")
            {
                resource_t * ares = new edge_resource_t(platform, dir);
                resource_ptrs.push_back( ares );
            }
            else if( n == "detuned_qubits")
            {
                resource_t * ares = new detuned_qubits_resource_t(platform, dir);
                resource_ptrs.push_back( ares );
            }
            else
            {
                COUT("Error : Un-modelled resource: '" << n << "'");
                throw ql::exception("[x] Error : Un-modelled resource: "+n+" !",false);
            }
        }
        // DOUT("Done constructing inited resouce_manager_t");
    }
};

} // end of namespace arch
} // end of namespace ql

#endif

