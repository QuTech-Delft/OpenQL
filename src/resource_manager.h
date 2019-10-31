/**
 * @file   resource_manager.h
 * @date   09/2017
 * @author Imran Ashraf
 * @date   09/2018
 * @author Hans van Someren
 * @brief  Resource mangement for cc light platform
 */

#ifndef QL_RESOURCE_MANAGER_H
#define QL_RESOURCE_MANAGER_H

#include <vector>
#include <string>

namespace ql
{
    typedef enum {
        forward_scheduling = 0,
        backward_scheduling = 1
    } scheduling_direction_t;

    namespace arch
    {
        class resource_t;
        class resource_manager_t;
    }
}

class ql::arch::resource_t
{
public:
    std::string name;
    size_t count;
    scheduling_direction_t direction;

    resource_t(std::string n, scheduling_direction_t dir) : name(n), direction(dir)
    {
        DOUT("constructing resource: " << n << " for direction (0:fwd,1:bwd): " << dir);
    }

    virtual bool available(size_t op_start_cycle, ql::gate * ins, std::string & operation_name,
        std::string & operation_type, std::string & instruction_type, size_t operation_duration) = 0;
    virtual void reserve(size_t op_start_cycle, ql::gate * ins, std::string & operation_name,
        std::string & operation_type, std::string & instruction_type, size_t operation_duration) = 0;
    virtual ~resource_t() {}
    virtual resource_t* clone() const & = 0;
    virtual resource_t* clone() && = 0;

    void Print(std::string s)
    {
        DOUT(s);
        DOUT("resource name=" << name << "; count=" << count);
    }
};

class ql::arch::resource_manager_t
{
protected:

    std::vector<resource_t*> resource_ptrs;

    // constructor needed by mapper::FreeCycle to bridge time from its construction to its Init
    // see the note on the use of constructors and Init functions at the start of mapper.h
    resource_manager_t()
    {
        DOUT("Constructing virgin resouce_manager_t");
    }

    // backward compatible delegating constructor, only doing forward_scheduling
    resource_manager_t(const ql::quantum_platform & platform) : resource_manager_t(platform, forward_scheduling) {}

    resource_manager_t(const ql::quantum_platform & platform, scheduling_direction_t dir)
    {
    }

public:

    void Print(std::string s)
    {
        DOUT(s);
    }

    // copy constructor doing a deep copy
    // *orgrptr->clone() does the trick to create a copy of the actual derived class' object
    resource_manager_t(const resource_manager_t& org)
    {
        DOUT("Copy constructing resouce_manager_t");
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
        DOUT("Copy assigning resouce_manager_t");
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
        // COUT("checking availability of resources for: " << ins->qasm());
        for(auto rptr : resource_ptrs)
        {
            // DOUT("... checking availability for resource " << rptr->name);
            if( rptr->available(op_start_cycle, ins, operation_name, operation_type, instruction_type, operation_duration) == false)
            {
                // DOUT("... resource " << rptr->name << "not available");
                return false;
            }
        }
        // DOUT("all resources available for: " << ins->qasm());
        return true;
    }

    void reserve(size_t op_start_cycle, ql::gate * ins, std::string & operation_name,
        std::string & operation_type, std::string & instruction_type, size_t operation_duration)
    {
        // COUT("reserving resources for: " << ins->qasm());
        for(auto rptr : resource_ptrs)
        {
            // DOUT("... reserving resource " << rptr->name);
            rptr->reserve(op_start_cycle, ins, operation_name, operation_type, instruction_type, operation_duration);
        }
        // DOUT("all resources reserved for: " << ins->qasm());
    }

    // destructor destroying deep resource_t's
    // runs before shallow destruction which is done by synthesized resource_manager_t destructor
    ~resource_manager_t()
    {
        DOUT("Destroying resource_manager_t");
        for(auto rptr : resource_ptrs)
        {
            delete rptr;
        }
    }
};

#endif // QL_RESOURCE_MANAGER_H

