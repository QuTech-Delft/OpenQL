/**
 * @file   optimizer.h
 * @date   11/2016
 * @author Nader Khammassi
 * @brief  optimizer interface and its implementation
 * @todo   implementations should be in separate files for better readability
 */
#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "circuit.h"


#define println(x) std::cout << x << std::endl

namespace ql
{
/**
 * optimizer interface
 */
class optimizer
{
public:
    virtual circuit optimize(circuit& c) = 0;
};

/**
 * rotation fuser
 */
class rotations_merging : public optimizer
{
public:

    circuit optimize(circuit& ic /*, bool verbose=false */)
    {
        circuit c=ic;
        // if (verbose) println("[+] optimizing circuit...");
        for (size_t i=c.size(); i>1; i--)
        {
            // println("[+] window size : " << i);
            c = optimize_sliding_window(c,i);
            if (c.size()<i) break;
        }

        if (c.size()>1)
            c = optimize_sliding_window(c,2);

        // if (verbose) println("[+] optimization done.");

        return c;
    }

protected:

    ql::cmat_t fuse(ql::cmat_t& m1, ql::cmat_t& m2)
    {
        ql::cmat_t      res;
        ql::complex_t * x = m1.m;
        ql::complex_t * y = m2.m;
        ql::complex_t * r = res.m;

        // m1.dump();
        // m2.dump();

        r[0] = x[0]*y[0] + x[1]*y[2];
        r[1] = x[0]*y[1] + x[1]*y[3];
        r[2] = x[2]*y[0] + x[3]*y[2];
        r[3] = x[2]*y[1] + x[3]*y[3];

        // res.dump();

        return res;
    }

#define __epsilon__ (1e-4)

    bool is_id(ql::cmat_t& mat)
    {
        // mat.dump();
        ql::complex_t * m = mat.m;
        if ((std::abs(std::abs(m[0].real())-1.0))>__epsilon__) return false;
        if ((std::abs(m[0].imag())  )>__epsilon__) return false;
        if ((std::abs(m[1].real())  )>__epsilon__) return false;
        if ((std::abs(m[1].imag())  )>__epsilon__) return false;
        if ((std::abs(m[2].real())  )>__epsilon__) return false;
        if ((std::abs(m[2].imag())  )>__epsilon__) return false;
        if ((std::abs(std::abs(m[3].real())-1.0))>__epsilon__) return false;
        if ((std::abs(m[3].imag())  )>__epsilon__) return false;
        return true;
    }



    bool is_identity(ql::circuit& c)
    {
        if (c.size()==1)
            return false;
        ql::cmat_t m = c[0]->mat();
        for (size_t i=1; i<c.size(); ++i)
        {
            ql::cmat_t m2 = c[i]->mat();
            m = fuse(m,m2);
        }
        return (is_id(m));
    }

    ql::circuit optimize_sliding_window(ql::circuit& c, size_t window_size)
    {
        ql::circuit oc;
        std::vector<int> id_pos;
        for (size_t i=0; i<c.size()-window_size+1; ++i)
        {
            ql::circuit w;
            w.insert(w.begin(),c.begin()+i,c.begin()+i+window_size);
            if (is_identity(w))
                id_pos.push_back(i);
        }
        if (id_pos.empty())
        {
            return ql::circuit(c);
        }
        // println("[+] id pos:");
        // for (size_t i=0; i<id_pos.size(); i++)
        // println(id_pos[i]);

        if (id_pos.size()==1)
        {
            // println("[+] rotations cancelling...");
            size_t pos = id_pos[0];
            size_t i=0;
            while (i<c.size())
            {
                if (i==pos)
                    i+=window_size;
                else
                {
                    oc.push_back(c[i]);
                    i++;
                }
            }
            return oc;
        }

        // println("[+] removing overlapping windows...");
        std::vector<int> pid;
        // int prev = id_pos[0];
        // println("[+] rotation cancelling...");
        size_t pos = id_pos[0];
        size_t ip  = 0;
        size_t i=0;
        while (i<c.size())
        {
            if (i==pos)
            {
                i+=window_size;
                ip++;
                if (ip<id_pos.size())
                    pos=id_pos[ip];
            }
            else
            {
                oc.push_back(c[i]);
                i++;
            }
        }

        return oc;
    }

};
}

#endif // OPTIMIZER_H
