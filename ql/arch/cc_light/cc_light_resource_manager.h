/**
 * @file   cc_light_resource_manager.h
 * @date   09/2017
 * @author Imran Ashraf
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
        DOUT("constructing resource : " << n);
    }
    virtual bool available(size_t op_start_cycle, ql::gate * ins, std::string & operation_name,
        std::string & operation_type, std::string & instruction_type, size_t operation_duration)=0;
    virtual void reserve(size_t op_start_cycle, ql::gate * ins, std::string & operation_name,
        std::string & operation_type, std::string & instruction_type, size_t operation_duration)=0;
    virtual ~resource_t() {}
};

class qubit_resource_t : public resource_t
{
public:
    std::vector<size_t> state; // is busy till cycle
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
            DOUT(" available? curr op_start_cycle: " << op_start_cycle << "  qubit: " << q 
                << " is busy till cycle : " << state[q]);
            if( op_start_cycle < state[q] )
            {
                DOUT("    qubit resource busy ...");
                return false;
            }
        }
        DOUT("    qubit resource available ...");
        return true;
    }

    void reserve(size_t op_start_cycle, ql::gate * ins, std::string & operation_name,
        std::string & operation_type, std::string & instruction_type, size_t operation_duration)
    {
        for( auto q : ins->operands )
        {
            state[q]  = op_start_cycle + operation_duration;
            DOUT("reserved. curr op_start_cycle: " << op_start_cycle << " qubit: " << q 
                << " reserved till cycle: " << state[q]);
        }
    }
    ~qubit_resource_t() {}
};


class qwg_resource_t : public resource_t
{
public:
    std::vector<size_t> state; // is busy till cycle
    std::vector<std::string> operations;
    std::map<size_t,size_t> qubit2qwg;

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
                DOUT(" available? curr op_start_cycle: " << op_start_cycle << "  qwg: " << qubit2qwg[q] 
                       << " is busy till op_start_cycle : " << state[ qubit2qwg[q] ] 
                       << " for operation: " << operations[ qubit2qwg[q] ]);
                if( op_start_cycle < state[ qubit2qwg[q] ] )
                {
                    if( operations[ qubit2qwg[q] ] != operation_name )
                    {
                        DOUT("    qwg resource busy ");
                        return false;
                    }
                }
            }
        }
        DOUT("    qwg resource available ...");
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
                if( state[ qubit2qwg[q] ] < op_start_cycle + operation_duration)
                    state[ qubit2qwg[q] ]  = op_start_cycle + operation_duration;
                operations[ qubit2qwg[q] ] = operation_name;
                DOUT("reserved. curr op_start_cycle: " << op_start_cycle << " qwg: " << qubit2qwg[q] 
                    << " reserved till cycle: " << state[ qubit2qwg[q] ] 
                    << " for operation: " << operations[ qubit2qwg[q] ] );
            }
        }
    }
    ~qwg_resource_t() {}
};

class meas_resource_t : public resource_t
{
public:
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
                DOUT(" available? curr op_start_cycle: " << op_start_cycle << "  meas: " << qubit2meas[q] 
                          << " is busy till cycle : " << state[ qubit2meas[q] ] );
                if( op_start_cycle != start_cycle[ qubit2meas[q] ] )
                {
                    // If current measurement on same measurement-unit does not start in the
                    // same cycle, then it should wait for current measurement to finish
                    if( op_start_cycle < state[ qubit2meas[q] ] )
                    {
                        DOUT("    measure resource busy ");
                        return false;
                    }
                }
            }
            DOUT("    measure resource available ...");
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
                DOUT("reserved. curr op_start_cycle: " << op_start_cycle << " meas: " << qubit2meas[q] 
                    << " reserved till cycle: " << state[ qubit2meas[q] ] );
            }
        }
    }
    ~meas_resource_t() {}
};

class edge_resource_t : public resource_t
{
public:
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

                DOUT(" available? curr op_start_cycle: " << op_start_cycle << ", edge: " << edge_no
                          << " is busy till cycle : " << state[edge_no] << " for operation: cz");

                std::vector<size_t> edges2check(edge2edges[edge_no]);
                edges2check.push_back(edge_no);
                for(auto & e : edges2check)
                {
                    if( op_start_cycle < state[e] )
                    {
                        DOUT("    edge resource busy ");
                        return false;
                    }
                }
                DOUT("    edge resource available ...");
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

            DOUT("reserved. curr op_start_cycle: " << op_start_cycle << " edge: " << edge_no 
                << " reserved till cycle: " << state[ edge_no ] 
                << " for operation: " << ins->name);
        }
    }
    ~edge_resource_t() {}
};


class resource_manager_t
{
public:
    std::vector<resource_t*> resource_ptrs;

    resource_manager_t( ql::quantum_platform & platform )
    {
        // COUT("No of resources : " << platform.resources.size());
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
            else
            {
                COUT("Error : Un-modelled resource: " << n );
                throw ql::exception("[x] Error : Un-modelled resource: "+n+" !",false);
            }
        }
    }

    bool available(size_t op_start_cycle, ql::gate * ins, std::string & operation_name,
        std::string & operation_type, std::string & instruction_type, size_t operation_duration)
    {
        // COUT("checking availablility of resources for: " << ins->qasm());
        for(auto rptr : resource_ptrs)
        {
            if( rptr->available(op_start_cycle, ins, operation_name, operation_type, instruction_type, operation_duration) == false)
                return false;
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
    ~resource_manager_t()
    {
        for(auto rptr : resource_ptrs)
            delete rptr;
    }

};

} // end of namespace arch
} // end of namespace ql

#endif
