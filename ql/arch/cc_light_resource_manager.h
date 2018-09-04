/**
 * @file   cc_light_resource_manager.h
 * @date   09/2017
 * @author Imran Ashraf
 * @author Hans van Someren
 * @brief  Resource mangement for cc light platform
 */

#ifndef _cclight_resource_manager_h
#define _cclight_resource_manager_h

#include <ql/json.h>
#include <fstream>
#include <vector>
#include <string>

using json = nlohmann::json;

namespace ql
{
namespace arch
{

class resource_t
{
public:
    std::string name;
    size_t count;

    resource_t(std::string n) : name(n)
    {
    }

    virtual bool available(size_t op_start_cycle, ql::gate * ins, std::string & operation_name,
        std::string & operation_type, std::string & instruction_type, size_t operation_duration)=0;
    virtual void reserve(size_t op_start_cycle, ql::gate * ins, std::string & operation_name,
        std::string & operation_type, std::string & instruction_type, size_t operation_duration)=0;
    virtual ~resource_t() {}
    virtual resource_t* clone() const & = 0;
    virtual resource_t* clone() && = 0;

    void Print(std::string s)
    {
        DOUT(s);
        DOUT("resource name=" << name << "; count=" << count );
    }
};

class qubit_resource_t : public resource_t
{
public:
    qubit_resource_t* clone() const & { return new qubit_resource_t(*this);}
    qubit_resource_t* clone() && { return new qubit_resource_t(std::move(*this)); }

    std::vector<size_t> state; // qubit q is busy till cycle=state[q]
    qubit_resource_t(ql::quantum_platform & platform) : resource_t("qubits")
    {
        count = platform.resources[name]["count"];
        state.resize(count);
        for(size_t i=0; i<count; i++)
        {
            state[i] = 0;
        }
    }

    bool available(size_t op_start_cycle, ql::gate * ins, std::string & operation_name,
        std::string & operation_type, std::string & instruction_type, size_t operation_duration)
    {
        for( auto q : ins->operands )
        {
            // DOUT(" available? op_start_cycle: " << op_start_cycle << "  qubit: " << q << " is busy till cycle : " << state[q]);
            if( op_start_cycle < state[q] )
            {
                // DOUT("    qubit resource busy ...");
                return false;
            }
        }
        // DOUT("    qubit resource available ...");
        return true;
    }

    void reserve(size_t op_start_cycle, ql::gate * ins, std::string & operation_name,
        std::string & operation_type, std::string & instruction_type, size_t operation_duration)
    {
        for( auto q : ins->operands )
        {
            state[q]  = op_start_cycle + operation_duration;
            // DOUT("reserved. op_start_cycle: " << op_start_cycle << " qubit: " << q << " reserved till cycle: " << state[q]);
        }
    }
    ~qubit_resource_t() {}
};


class qwg_resource_t : public resource_t
{
public:
    qwg_resource_t* clone() const & { return new qwg_resource_t(*this);}
    qwg_resource_t* clone() && { return new qwg_resource_t(std::move(*this)); }

    std::vector<size_t> state;              // qubit q is busy till cycle==state[q]
    std::vector<std::string> operations;    // with operation_name==operations[q]
    std::map<size_t,size_t> qubit2qwg;      // on qwg==qubit2qwg[q]

    qwg_resource_t(ql::quantum_platform & platform) : resource_t("qwgs")
    {
        count = platform.resources[name]["count"];
        state.resize(count);
        operations.resize(count);

        for(size_t i=0; i<count; i++)
        {
            state[i] = 0;
            operations[i] = "";
        }
        auto & constraints = platform.resources[name]["connection_map"];
        for (json::iterator it = constraints.begin(); it != constraints.end(); ++it)
        {
            // DOUT(it.key() << " : " << it.value() );
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
                // DOUT(" available? op_start_cycle: " << op_start_cycle << "  qwg: " << qubit2qwg[q] << " is busy till cycle : " << state[ qubit2qwg[q] ] << " for operation: " << operations[ qubit2qwg[q] ]);
                if( op_start_cycle < state[ qubit2qwg[q] ] )
                {
                    if( operations[ qubit2qwg[q] ] != operation_name )
                    {
                        // DOUT("    qwg resource busy ");
                        return false;
                    }
                }
            }
        }
        // DOUT("    qwg resource available ...");
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
                // op_start_cycle >= state[qubit2qwg[q]] || operations[qubit2qwg[q]] == operation_name
                if( state[ qubit2qwg[q] ] < op_start_cycle + operation_duration)
                    state[ qubit2qwg[q] ]  = op_start_cycle + operation_duration;
                operations[ qubit2qwg[q] ] = operation_name;
                // DOUT("reserved. op_start_cycle: " << op_start_cycle << " qwg: " << qubit2qwg[q] << " reserved till cycle: " << state[ qubit2qwg[q] ] << " for operation: " << operations[ qubit2qwg[q] ] );
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

    std::vector<size_t> start_cycle; // last measurement start cycle
    std::vector<size_t> state; // is busy till cycle
    std::map<size_t,size_t> qubit2meas;

    meas_resource_t(ql::quantum_platform & platform) : resource_t("meas_units")
    {
        count = platform.resources[name]["count"];
        state.resize(count);
        start_cycle.resize(count);

        for(size_t i=0; i<count; i++)
        {
            start_cycle[i] = 0;
            state[i] = 0;
        }
        auto & constraints = platform.resources[name]["connection_map"];
        for (json::iterator it = constraints.begin(); it != constraints.end(); ++it)
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
                // DOUT(" available? op_start_cycle: " << op_start_cycle << "  meas: " << qubit2meas[q] << " is busy till cycle : " << state[ qubit2meas[q] ] );
                if( op_start_cycle != start_cycle[ qubit2meas[q] ] )
                {
                    // If current measurement on same measurement-unit does not start in the
                    // same cycle, then it should wait for current measurement to finish
                    if( op_start_cycle < state[ qubit2meas[q] ] )
                    {
                        // DOUT("    measure resource busy ");
                        return false;
                    }
                }
            }
            // DOUT("    measure resource available ...");
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
                start_cycle[ qubit2meas[q] ] = op_start_cycle;
                state[ qubit2meas[q] ] = op_start_cycle + operation_duration;
                // DOUT("reserved. op_start_cycle: " << op_start_cycle << " meas: " << qubit2meas[q] << " reserved till cycle: " << state[ qubit2meas[q] ] );
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

    std::vector<size_t> state; // is busy till cycle
    typedef std::pair<size_t,size_t> qubits_pair_t;
    std::map< qubits_pair_t, size_t > qubits2edge;
    std::map<size_t, std::vector<size_t> > edge2edges;

    edge_resource_t(ql::quantum_platform & platform) : resource_t("edges")
    {
        count = platform.resources[name]["count"];
        state.resize(count);

        for(size_t i=0; i<count; i++)
        {
            state[i] = 0;
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
        for (json::iterator it = constraints.begin(); it != constraints.end(); ++it)
        {
            // COUT(it.key() << " : " << it.value() << "\n";
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
            auto q0 = ins->operands[0];
            auto q1 = ins->operands[1];
            qubits_pair_t aqpair(q0, q1);
            auto it = qubits2edge.find(aqpair);
            if( it != qubits2edge.end() )
            {
                auto edge_no = qubits2edge[aqpair];

                // DOUT(" available? op_start_cycle: " << op_start_cycle << ", edge: " << edge_no << " is busy till cycle : " << state[edge_no] << " for operation: " << ins->name);

                std::vector<size_t> edges2check(edge2edges[edge_no]);
                edges2check.push_back(edge_no);
                for(auto & e : edges2check)
                {
                    if( op_start_cycle < state[e] )
                    {
                        // DOUT("    edge resource busy ");
                        return false;
                    }
                }
                // DOUT("    edge resource available ...");
            }
            else
            {
                EOUT("Use of illegal edge: " << q0 << "->" << q1 << " in operation: " << ins->name << " !");
                throw ql::exception("[x] Error : Use of illegal edge"+std::to_string(q0)+"->"+std::to_string(q1)+"in operation:"+ins->name+" !",false);
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
            auto q0 = ins->operands[0];
            auto q1 = ins->operands[1];
            qubits_pair_t aqpair(q0, q1);
            auto edge_no = qubits2edge[aqpair];
            state[edge_no] = op_start_cycle + operation_duration;
            for(auto & e : edge2edges[edge_no])
            {
                state[e] = op_start_cycle + operation_duration;
            }

            // DOUT("reserved. op_start_cycle: " << op_start_cycle << " edge: " << edge_no << " reserved till cycle: " << state[ edge_no ] << " for operation: " << ins->name);
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
// - state[q]: qubit q is busy till cycle state[q], state[q] is the first free cycle
// - operations[q]: when busy, qubit q is busy with a "flux" or "mw" ("" is initial value different from these two)
// - starting[q]: when busy, qubit q switched to the current operation type (as in operations[q]) in cycle starting[q]
// The latter is needed since a qubit can be busy with multiple "flux"s (i.e. being the detuned qubit for several "flux"s),
// so the second, etc. of these can be scheduled in parallel to the first but not further back in the past than starting[q],
// since till that cycle is was likely to be busy with "mw", which doesn't allow a "flux" in parallel.
// The other members contain internal copies of the resource description and grid configuration of the json file.
class detuned_qubits_resource_t : public resource_t
{
public:
    detuned_qubits_resource_t* clone() const & { return new detuned_qubits_resource_t(*this);}
    detuned_qubits_resource_t* clone() && { return new detuned_qubits_resource_t(std::move(*this)); }

    std::vector<size_t> state;                                  // qubit q is busy till state[q]
    std::vector<std::string> operations;                        // with operation_type==operations[q]
    std::vector<size_t> starting;                               // from cycle starting[q]

    typedef std::pair<size_t,size_t> qubits_pair_t;
    std::map< qubits_pair_t, size_t > qubitpair2edge;           // map: pair of qubits to edge (from grid configuration)
    std::map<size_t, std::vector<size_t> > edge_detunes_qubits; // map: edge to vector of qubits that edge detunes (resource desc.)

    detuned_qubits_resource_t(ql::quantum_platform & platform) : resource_t("detuned_qubits")
    {
        count = platform.resources[name]["count"];
        state.resize(count);
        operations.resize(count);
        starting.resize(count);

        // initialize resource state machine to be free for all qubits
        for(size_t i=0; i<count; i++)
        {
            state[i] = 0;
            operations[i] = "";
            starting[i] = 0;
        }

        // initialize qubitpair2edge map from json description; this is a constant map
        for( auto & anedge : platform.topology["edges"] )
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
        for (json::iterator it = constraints.begin(); it != constraints.end(); ++it)
        {
            // COUT(it.key() << " : " << it.value() << "\n";
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
            auto q0 = ins->operands[0];
            auto q1 = ins->operands[1];
            qubits_pair_t aqpair(q0, q1);
            auto it = qubitpair2edge.find(aqpair);
            if( it != qubitpair2edge.end() )
            {
                auto edge_no = qubitpair2edge[aqpair];

                std::vector<size_t> qubits2check(edge_detunes_qubits[edge_no]);
                for(auto & q : qubits2check)
                {
                    // DOUT(" available detuned_qubits? op_start_cycle: " << op_start_cycle << ", edge: " << edge_no << " detuning qubit: " << q << " for operation: " << ins->name << " busy till: " << state[q] << " with operation_type: " << operation_type);
                    if( op_start_cycle < state[q] )
                    {
                        if( operations[q] != operation_type || op_start_cycle < starting[q] )
                        {
                            // DOUT("    detuned_qubits resource busy for a flux");
                            return false;
                        }
                    }
                    // state[q] <= op_start_cycle || operations[q] == operation_type && starting[q] <= op_start_cycle
                }
            }
            else
            {
                EOUT("Use of illegal edge: " << q0 << "->" << q1 << " in operation: " << ins->name << " !");
                throw ql::exception("[x] Error : Use of illegal edge"+std::to_string(q0)+"->"+std::to_string(q1)+"in operation:"+ins->name+" !",false);
            }
        }
        bool is_mw = (operation_type == "mw");
        if ( is_mw )
        {
            for( auto q : ins->operands )
            {
                // DOUT(" available detuned_qubits? op_start_cycle: " << op_start_cycle << ", qubit: " << q << " for operation: " << ins->name << " busy till: " << state[q] << " with operation_type: " << operation_type);
                if( op_start_cycle < state[q] )
                {
                    if( operations[q] != operation_type || op_start_cycle < starting[q] )
                    {
                        // DOUT("    detuned_qubits resource busy for a rotation");
                        return false;
                    }
                }
                // state[q] <= op_start_cycle || operations[q] == operation_type && starting[q] <= op_start_cycle
            }
        }
        // DOUT("    detuned_qubits resource available ...");
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
            auto q0 = ins->operands[0];
            auto q1 = ins->operands[1];
            qubits_pair_t aqpair(q0, q1);
            auto edge_no = qubitpair2edge[aqpair];

            std::vector<size_t> qubits2check(edge_detunes_qubits[edge_no]);
            for(auto & q : qubits2check)
            {
                // state[q] <= op_start_cycle || operations[q] == operation_type && starting[q] <= op_start_cycle
                state[q] = std::max(op_start_cycle + operation_duration, state[q]);
                if (operations[q] != operation_type)
                {
                    starting[q] = op_start_cycle;
                    operations[q] = operation_type;
                }
                // op_start_cycle + operation_duration <= state[q] && operations[q] == operation_type && starting[q] <= op_start_cycle
                // DOUT("reserved detuned_qubits. op_start_cycle: " << op_start_cycle << " edge: " << edge_no << " detunes qubit: " << q << " reserved till cycle: " << state[q] << " for operation: " << ins->name);
            }
        }
        bool is_mw = (operation_type == "mw");
        if ( is_mw )
        {
            for( auto q : ins->operands )
            {
                // state[q] <= op_start_cycle || operations[q] == operation_type && starting[q] <= op_start_cycle
                state[q] = std::max(op_start_cycle + operation_duration, state[q]);
                if (operations[q] != operation_type)
                {
                    starting[q] = op_start_cycle;
                    operations[q] = operation_type;
                }
                // op_start_cycle + operation_duration <= state[q] && operations[q] == operation_type && starting[q] <= op_start_cycle
                // DOUT("reserved detuned_qubits. op_start_cycle: " << op_start_cycle << " for qubit: " << q << " reserved till cycle: " << state[q] << " for operation: " << ins->name);
            }
        }
    }
    ~detuned_qubits_resource_t() {}
};

class resource_manager_t
{
public:
    std::vector<resource_t*> resource_ptrs;

    // constructor needed by mapper::FreeCycle to bridge time from its construction to its Init
    // see the note on the use of constructors and Init functions at the start of mapper.h
    resource_manager_t()
    {
        // DOUT("Constructing virgin resouce_manager_t");
    }

    resource_manager_t( ql::quantum_platform & platform )
    {
        // DOUT("Constructing inited resouce_manager_t");
        // COUT("New one with no of resources : " << platform.resources.size());
        for (json::iterator it = platform.resources.begin(); it != platform.resources.end(); ++it)
        {
            // COUT(it.key() << " : " << it.value() << "\n";
            std::string n = it.key();

            if( n == "qubits")
            {
                resource_t * ares = new qubit_resource_t(platform);
                resource_ptrs.push_back( ares );
            }
            else if( n == "qwgs")
            {
                resource_t * ares = new qwg_resource_t(platform);
                resource_ptrs.push_back( ares );
            }
            else if( n == "meas_units")
            {
                resource_t * ares = new meas_resource_t(platform);
                resource_ptrs.push_back( ares );
            }
            else if( n == "edges")
            {
                resource_t * ares = new edge_resource_t(platform);
                resource_ptrs.push_back( ares );
            }
            else if( n == "detuned_qubits")
            {
                resource_t * ares = new detuned_qubits_resource_t(platform);
                resource_ptrs.push_back( ares );
            }
            else
            {
                COUT("Error : Un-modelled resource: " << n );
                throw ql::exception("[x] Error : Un-modelled resource: "+n+" !",false);
            }
        }
    }

    void Print(std::string s)
    {
        DOUT(s);
    }

    // destructor destroying deep resource_t's
    // runs before shallow destruction using synthesized resource_manager_t destructor
    ~resource_manager_t()
    {
        // DOUT("Destroying resource_manager_t");
        for(auto rptr : resource_ptrs)
        {
            delete rptr;
        }
    }

    // copy constructor doing a deep copy
    // *orgrptr->clone() does the trick to create a copy of the actual derived class' object
    resource_manager_t(const resource_manager_t& org)
    {
        // DOUT("Copy constructing resouce_manager_t");
        resource_ptrs.clear();
        for(auto orgrptr : org.resource_ptrs)
        {
            resource_ptrs.push_back( orgrptr->clone() );
        }
    }

    // copy-assignment operator
    // follow pattern to use tmp copy to allow self-assignment and to be exception safe
    resource_manager_t& operator=(const resource_manager_t& rhs)
    {
        // DOUT("Copy assigning resouce_manager_t");
        std::vector<resource_t*> new_resource_ptrs;
        for(auto orgrptr : rhs.resource_ptrs)
        {
            new_resource_ptrs.push_back( orgrptr->clone() );
        }
        for(auto rptr : resource_ptrs)
        {
            delete rptr;
        }
        resource_ptrs = new_resource_ptrs;
        return *this;
    }

    bool available(size_t op_start_cycle, ql::gate * ins, std::string & operation_name,
        std::string & operation_type, std::string & instruction_type, size_t operation_duration)
    {
        // COUT("checking availablility of resources for: " << ins->qasm());
        for(auto rptr : resource_ptrs)
        {
            if( rptr->available(op_start_cycle, ins, operation_name, operation_type, instruction_type, operation_duration) == false)
            {
                return false;
            }
        }
        return true;
    }

    void reserve(size_t op_start_cycle, ql::gate * ins, std::string & operation_name,
        std::string & operation_type, std::string & instruction_type, size_t operation_duration)
    {
        // COUT("reserving resources for: " << ins->qasm());
        for(auto rptr : resource_ptrs)
        {
            rptr->reserve(op_start_cycle, ins, operation_name, operation_type, instruction_type, operation_duration);
        }
    }
};

} // end of namespace arch
} // end of namespace ql

#endif
