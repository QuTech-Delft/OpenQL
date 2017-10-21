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
        // COUT("constructing resource : " << n);
    }
    virtual bool available(size_t cycle, ql::gate * ins, std::string & operation)=0;
    virtual void reserve(size_t cycle, ql::gate * ins, std::string & operation)=0;
    virtual ~resource_t() {}
};

bool inline is_measure(std::string & name)
{
    return ( (name).find("meas") != std::string::npos );
}

class qubit_resource_t : public resource_t
{
public:
    std::vector<size_t> state; // is busy till cycle number contained in state
    qubit_resource_t(ql::quantum_platform & platform) : resource_t("qubits")
    {
        count = platform.resources[name]["count"];
        state.resize(count);
        for(size_t i=0; i<count; i++)
        {
            state[i] = MAX_CYCLE;
        }
    }

    bool available(size_t cycle, ql::gate * ins, std::string & operation)
    {
        for( auto q : ins->operands )
        {
            DOUT(" available? curr cycle: " << cycle << "  qubit: " << q << " is busy till cycle : " << state[q]);
            if( cycle >= state[q] )
            {
                DOUT("    qubit resource busy ...");
                return false;
            }
        }
        DOUT("    qubit resource available ...");
        return true;
    }

    void reserve(size_t cycle, ql::gate * ins, std::string & operation)
    {
        for( auto q : ins->operands )
        {
            state[q] = cycle;
            DOUT("reserved. curr cycle: " << cycle << " qubit: " << q << " reserved till cycle: " << state[q]);
        }
    }
    ~qubit_resource_t() {}
};


class qwg_resource_t : public resource_t
{
public:
    std::vector<size_t> state; // is busy till cycle number contained in state
    std::vector<std::string> operations;
    std::map<size_t,size_t> qubit2qwg;

    qwg_resource_t(ql::quantum_platform & platform) : resource_t("qwgs")
    {
        count = platform.resources[name]["count"];
        state.resize(count);
        operations.resize(count);

        for(size_t i=0; i<count; i++)
        {
            state[i] = MAX_CYCLE;
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

    bool available(size_t cycle, ql::gate * ins, std::string & operation)
    {
        if( ! is_measure(ins->name) )
        {
            for( auto q : ins->operands )
            {
                DOUT(" available? curr cycle: " << cycle << "  qwg: " << qubit2qwg[q] 
                       << " is busy till cycle : " << state[ qubit2qwg[q] ] << " for operation: " << operations[ qubit2qwg[q] ]);
                if( cycle >= state[ qubit2qwg[q] ] )
                {
                    if( operations[ qubit2qwg[q] ] != operation )
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

    void reserve(size_t cycle, ql::gate * ins, std::string & operation)
    {
        if( ! is_measure(ins->name) )
        {
            for( auto q : ins->operands )
            {
                state[ qubit2qwg[q] ]  = cycle;
                operations[ qubit2qwg[q] ] = operation;
                DOUT("reserved. curr cycle: " << cycle << " qwg: " << qubit2qwg[q] << " reserved till cycle: " << state[ qubit2qwg[q] ] 
                          << " for operation: " << operations[ qubit2qwg[q] ] );
            }
        }
    }
    ~qwg_resource_t() {}
};

class meas_resource_t : public resource_t
{
public:
    std::vector<size_t> state; // is busy till cycle number contained in state
    std::map<size_t,size_t> qubit2meas;

    meas_resource_t(ql::quantum_platform & platform) : resource_t("meas_units")
    {
        count = platform.resources[name]["count"];
        state.resize(count);

        for(size_t i=0; i<count; i++)
        {
            state[i] = MAX_CYCLE;
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

    bool available(size_t cycle, ql::gate * ins, std::string & operation)
    {
        if( is_measure(ins->name) )
        {
            for(auto q : ins->operands)
            {
                DOUT(" available? curr cycle: " << cycle << "  meas: " << qubit2meas[q] 
                          << " is busy till cycle : " << state[ qubit2meas[q] ] );
                if( cycle >= state[ qubit2meas[q] ] )
                {
                    DOUT("    measure resource busy ");
                    return false;
                }
            }
            DOUT("    measure resource available ...");
        }
        return true;
    }

    void reserve(size_t cycle, ql::gate * ins, std::string & operation)
    {
        if( is_measure(ins->name) )
        {
            for(auto q : ins->operands)
            {
                state[ qubit2meas[q] ] = cycle;
                DOUT("reserved. curr cycle: " << cycle << " meas: " << qubit2meas[q] << " reserved till cycle: " << state[ qubit2meas[q] ] );
            }
        }
    }
    ~meas_resource_t() {}
};

class edge_resource_t : public resource_t
{
public:
    std::vector<size_t> state; // is busy till cycle number contained in state
    typedef std::pair<size_t,size_t> qubits_pair_t;
    std::map< qubits_pair_t, size_t > qubits2edge;
    std::map<size_t, std::vector<size_t> > edge2edges;

    edge_resource_t(ql::quantum_platform & platform) : resource_t("edges")
    {
        count = platform.resources[name]["count"];
        state.resize(count);

        for(size_t i=0; i<count; i++)
        {
            state[i] = MAX_CYCLE;
        }

        for( auto & anedge : platform.topology["edges"] )
        {
            size_t s = anedge["src"];
            size_t d = anedge["dst"];
            size_t e = anedge["id"];
            // COUT(s << " " << d << " : " << e);

            qubits_pair_t aqpair(s,d);
            auto it = qubits2edge.find(aqpair);
            if( it != qubits2edge.end() )
            {
                COUT("Error: re-defining edge number !");
                throw ql::exception("[x] Error : re-defining edge number !",false);
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

    bool available(size_t cycle, ql::gate * ins, std::string & operation)
    {
        if( 2 == (ins->operands).size() ) // 2 operand instruction reserve edges
        {
            auto q0 = ins->operands[0];
            auto q1 = ins->operands[1];
            qubits_pair_t aqpair(q0, q1);
            auto it = qubits2edge.find(aqpair);
            if( it != qubits2edge.end() )
            {
                auto edge_no = qubits2edge[aqpair];

                DOUT(" available? curr cycle: " << cycle << ", edge: " << edge_no
                          << " is busy till cycle : " << state[edge_no] << " for operation: cz");

                std::vector<size_t> edges2check(edge2edges[edge_no]);
                edges2check.push_back(edge_no);
                for(auto & e : edges2check)
                {
                    if( cycle >= state[e] )
                    {
                        DOUT("    edge resource busy ");
                        return false;
                    }
                }
                DOUT("    edge resource available ...");
            }
            else
            {
                COUT("Error: Use of illegal edge !");
                throw ql::exception("[x] Error : Use of illegal edge !",false);
            }
        }
        return true;
    }

    void reserve(size_t cycle, ql::gate * ins, std::string & operation)
    {
        if( 2 == (ins->operands).size() ) // 2 operand instruction reserve edges
        {
            auto q0 = ins->operands[0];
            auto q1 = ins->operands[1];
            qubits_pair_t aqpair(q0, q1);
            auto edge_no = qubits2edge[aqpair];
            state[edge_no] = cycle;
            for(auto & e : edge2edges[edge_no])
            {
                state[e] = cycle;
            }

            DOUT("reserved. curr cycle: " << cycle << " edge: " << edge_no << " reserved till cycle: " << state[ edge_no ] 
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
    bool available(size_t cycle, ql::gate * ins, std::string & operation)
    {
        // COUT("checking availablility of resources for: " << ins->qasm());
        for(auto rptr : resource_ptrs)
        {
            if( rptr->available(cycle, ins, operation) == false)
                return false;
        }
        return true;
    }
    void reserve(size_t cycle, ql::gate * ins, std::string & operation)
    {
        // COUT("reserving resources for: " << ins->qasm());
        for(auto rptr : resource_ptrs)
        {
            rptr->reserve(cycle, ins, operation);
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
