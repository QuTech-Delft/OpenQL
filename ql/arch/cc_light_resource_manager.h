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

// auto MAX_CYCLE = std::numeric_limits<std::size_t>::max(); // TODO should go to utils
size_t MAX_CYCLE = 100; // revert back to above

namespace ql
{
namespace arch
{


class resource_t
{
public:
    std::string name;
    size_t count;
    size_t platform_cycle_time;

    resource_t(std::string n, size_t cycle_time) : name(n), platform_cycle_time(cycle_time)
    {
        std::cout << "constructing resource : " << n << std::endl;
    }
    virtual bool available(size_t cycle, ql::gate * ins)=0;
    virtual void reserve(size_t cycle, ql::gate * ins)=0;
};

bool inline is_measure(std::string & name)
{
    return ( (name).find("meas") != std::string::npos );
}

class qubit_resource_t : public resource_t
{
public:
    std::vector<size_t> state; // is busy till cycle number contained in state
    qubit_resource_t(json & jplatform, size_t cycle_time) : resource_t("qubits", cycle_time)
    {
        count = jplatform["resources"][name]["count"];
        state.resize(count);
        for(size_t i=0; i<count; i++)
        {
            state[i] = MAX_CYCLE;
        }
    }

    bool available(size_t cycle, ql::gate * ins)
    {
        for( auto q : ins->operands )
        {
            std::cout << " available? curr cycle: " << cycle << "  qubit: " << q << " is busy till cycle : " << state[q] << std::endl;
            if( cycle > state[q] )
            {
                std::cout << "    qubit resource busy ..." << std::endl;
                return false;
            }
        }
        std::cout << "    qubit resource available ..." << std::endl;
        return true;
    }

    void reserve(size_t cycle, ql::gate * ins)
    {
        for( auto q : ins->operands )
        {
            state[q] = cycle - (ins->duration)/platform_cycle_time;
            std::cout << "reserved. curr cycle: " << cycle << " qubit: " << q << " reserved till cycle: " << state[q] << std::endl;
        }
    }
};


class qwg_resource_t : public resource_t
{
public:
    std::vector<size_t> state; // is busy till cycle number contained in state
    std::vector<std::string> operations;
    std::map<size_t,size_t> qubit2qwg;

    qwg_resource_t(json & jplatform, size_t cycle_time) : resource_t("qwgs", cycle_time)
    {
        count = jplatform["resources"][name]["count"];
        state.resize(count);
        operations.resize(count);

        for(size_t i=0; i<count; i++)
        {
            state[i] = MAX_CYCLE;
            operations[i] = "";
        }
        auto & constraints = jplatform["resources"][name]["connection_map"];
        for (json::iterator it = constraints.begin(); it != constraints.end(); ++it)
        {
            // std::cout << it.key() << " : " << it.value() << "\n";
            size_t qwgNo = stoi( it.key() );
            auto & connected_qubits = it.value();
            for(auto & q : connected_qubits)
                qubit2qwg[q] = qwgNo;
        }
    }

    bool available(size_t cycle, ql::gate * ins)
    {
        if( ! is_measure(ins->name) )
        {
            for( auto q : ins->operands )
            {
                std::cout << " available? curr cycle: " << cycle << "  qwg: " << qubit2qwg[q] 
                          << " is busy till cycle : " << state[ qubit2qwg[q] ] << " for operation: " << operations[ qubit2qwg[q] ] << std::endl;
                if( cycle > state[ qubit2qwg[q] ] )
                {
                    if( operations[ qubit2qwg[q] ] != (ins->name) )
                    {
                        std::cout << "    qwg resource busy " << std::endl;
                        return false;
                    }
                }
            }
        }
        std::cout << "    qwg resource available ..." << std::endl;
        return true;
    }

    void reserve(size_t cycle, ql::gate * ins)
    {
        if( ! is_measure(ins->name) )
        {
            for( auto q : ins->operands )
            {
                state[ qubit2qwg[q] ]  = cycle - (ins->duration)/platform_cycle_time;
                operations[ qubit2qwg[q] ] = ins->name;
                std::cout << "reserved. curr cycle: " << cycle << " qwg: " << qubit2qwg[q] << " reserved till cycle: " << state[ qubit2qwg[q] ] 
                          << " for operation: " << operations[ qubit2qwg[q] ]  << std::endl;
            }
        }
    }
};

class meas_resource_t : public resource_t
{
public:
    std::vector<size_t> state; // is busy till cycle number contained in state
    std::map<size_t,size_t> qubit2meas;

    meas_resource_t(json & jplatform, size_t cycle_time) : resource_t("meas_units", cycle_time)
    {
        count = jplatform["resources"][name]["count"];
        state.resize(count);

        for(size_t i=0; i<count; i++)
        {
            state[i] = MAX_CYCLE;
        }
        auto & constraints = jplatform["resources"][name]["connection_map"];
        for (json::iterator it = constraints.begin(); it != constraints.end(); ++it)
        {
            // std::cout << it.key() << " : " << it.value() << "\n";
            size_t measUnitNo = stoi( it.key() );
            auto & connected_qubits = it.value();
            for(auto & q : connected_qubits)
                qubit2meas[q] = measUnitNo;
        }
    }

    bool available(size_t cycle, ql::gate * ins)
    {
        if( is_measure(ins->name) )
        {
            for(auto q : ins->operands)
            {
                std::cout << " available? curr cycle: " << cycle << "  meas: " << qubit2meas[q] 
                          << " is busy till cycle : " << state[ qubit2meas[q] ] << " for operation: measure" << std::endl;
                if( cycle == state[ qubit2meas[q] ] )
                {
                    std::cout << "    measure resource busy " << std::endl;
                    return false;
                }
            }
            std::cout << "    measure resource available ..." << std::endl;
        }
        return true;
    }

    void reserve(size_t cycle, ql::gate * ins)
    {
        if( is_measure(ins->name) )
        {
            for(auto q : ins->operands)
            {
                state[ qubit2meas[q] ] = cycle - (ins->duration)/platform_cycle_time;
                std::cout << "reserved. curr cycle: " << cycle << " meas: " << qubit2meas[q] << " reserved till cycle: " << state[ qubit2meas[q] ] 
                          << " for operation: measure" << std::endl;
            }
        }
    }
};

class edge_resource_t : public resource_t
{
public:
    std::vector<size_t> state; // is busy till cycle number contained in state
    typedef std::pair<size_t,size_t> qubits_pair_t;
    std::map< qubits_pair_t, size_t > qubits2edge;
    std::map<size_t, std::vector<size_t> > edge2edges;

    edge_resource_t(json & jplatform, size_t cycle_time) : resource_t("edges", cycle_time)
    {
        count = jplatform["resources"][name]["count"];
        state.resize(count);

        for(size_t i=0; i<count; i++)
        {
            state[i] = MAX_CYCLE;
        }

        for( auto & anedge : jplatform["topology"]["edges"] )
        {
            size_t s = anedge["src"];
            size_t d = anedge["dst"];
            size_t e = anedge["id"];
            // std::cout << s << " " << d << " : " << e << std::endl;

            qubits_pair_t aqpair(s,d);
            auto it = qubits2edge.find(aqpair);
            if( it != qubits2edge.end() )
            {
                std::cout << "Error: re-defining edge number !" << std::endl;
                exit(0);
            }
            else
            {
                qubits_pair_t arqpair(d,s);
                qubits2edge[aqpair] = e;
                qubits2edge[arqpair] = e;
            }
        }

        auto & constraints = jplatform["resources"][name]["connection_map"];
        for (json::iterator it = constraints.begin(); it != constraints.end(); ++it)
        {
            // std::cout << it.key() << " : " << it.value() << "\n";
            size_t edgeNo = stoi( it.key() );
            auto & connected_edges = it.value();
            for(auto & e : connected_edges)
                edge2edges[e].push_back(edgeNo);
        }
    }

    bool available(size_t cycle, ql::gate * ins)
    {
        if(ins->name == "cz")
        {
            auto q0 = ins->operands[0];
            auto q1 = ins->operands[1];
            qubits_pair_t aqpair(q0, q1);
            auto it = qubits2edge.find(aqpair);
            if( it != qubits2edge.end() )
            {
                auto edge_no = qubits2edge[aqpair];

                std::cout << " available? curr cycle: " << cycle << ", edge: " << edge_no
                          << " is busy till cycle : " << state[edge_no] << " for operation: cz" << std::endl;

                std::vector<size_t> edges2check(edge2edges[edge_no]);
                edges2check.push_back(edge_no);
                for(auto & e : edges2check)
                {
                    if( cycle > state[e] )
                    {
                        std::cout << "    edge resource busy " << std::endl;
                        return false;
                    }
                }
                std::cout << "    edge resource available ..." << std::endl;
            }
            else
            {
                std::cout << "Error: Use of illegal edge !" << std::endl;
                exit(0);
            }
        }
        return true;
    }

    void reserve(size_t cycle, ql::gate * ins)
    {
        if(ins->name == "cz")
        {
            auto q0 = ins->operands[0];
            auto q1 = ins->operands[1];
            qubits_pair_t aqpair(q0, q1);
            auto edge_no = qubits2edge[aqpair];
            state[edge_no] = cycle + ins->duration - 1;
            for(auto & e : edge2edges[edge_no])
            {
                state[e] = cycle - (ins->duration)/platform_cycle_time;
            }

            std::cout << "reserved. curr cycle: " << cycle << " edge: " << edge_no << " reserved till cycle: " << state[ edge_no ] 
                      << " for operation: cz" << std::endl;
        }
    }
};


class resource_manager_t
{
public:
    std::vector<resource_t*> resource_ptrs;
    json jplatform;

    resource_manager_t( size_t cycle_time )
    {
        std::ifstream jin("CCL_platform.json");
        if( ! jin.good() )
        {
            std::cout << "Error opening CCL_platform.json" << std::endl;
        }
        jin >> jplatform;
        jin.close();

        std::cout << "No of resources : " << jplatform["resources"].size() << std::endl;
        for (json::iterator it = jplatform["resources"].begin(); it != jplatform["resources"].end(); ++it)
        {
            // std::cout << it.key() << " : " << it.value() << "\n";
            std::string n = it.key();


            if( n == "qubits")
            {
                resource_t * ares = new qubit_resource_t(jplatform, cycle_time);
                resource_ptrs.push_back( ares );
            }
            else if( n == "qwgs")
            {
                resource_t * ares = new qwg_resource_t(jplatform, cycle_time);
                resource_ptrs.push_back( ares );
            }
            else if( n == "meas_units")
            {
                resource_t * ares = new meas_resource_t(jplatform, cycle_time);
                resource_ptrs.push_back( ares );
            }
            else if( n == "edges")
            {
                resource_t * ares = new edge_resource_t(jplatform, cycle_time);
                resource_ptrs.push_back( ares );
            }
            else
            {
                std::cout << "Error : Un-modelled resource: " << n <<  std::endl;
            }

        }
    }
    bool available(size_t cycle, ql::gate * ins)
    {
        std::cout << "\nchecking availablility of resources for: " << ins->qasm() << std::endl;
        for(auto rptr : resource_ptrs)
        {
            if( rptr->available(cycle, ins) == false)
                return false;
        }
        return true;
    }
    void reserve(size_t cycle, ql::gate * ins)
    {
        std::cout << "reserving resources for: " << ins->qasm() << std::endl;
        for(auto rptr : resource_ptrs)
        {
            rptr->reserve(cycle, ins);
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
