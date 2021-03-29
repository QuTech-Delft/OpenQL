/** \file
 * Rotation optimizer pass implementation.
 */

#include "optimizer.h"

#include "ql/utils/vec.h"
#include "ql/utils/num.h"
#include "ql/com/options/options.h"

namespace ql {

using namespace utils;

/**
 * optimizer interface
 */
class optimizer {
public:
    virtual ir::Circuit optimize(ir::Circuit &c) = 0;
};

/**
 * rotation fuser
 */
class rotations_merging : public optimizer {
public:

    ir::Circuit optimize(ir::Circuit &ic /*, bool verbose=false */) override {
        ir::Circuit c = ic;
        // if (verbose) COUT("optimizing circuit...");
        for (size_t i = c.size(); i > 1; i--) {
            // println("window size : " << i);
            c = optimize_sliding_window(c,i);
            if (c.size() < i) {
                break;
            }
        }

        if (c.size() > 1) {
            c = optimize_sliding_window(c,2);
        }

        // if (verbose) COUT("optimization done.");

        return c;
    }

protected:

    static ir::Complex2by2Matrix fuse(const ir::Complex2by2Matrix &m1, const ir::Complex2by2Matrix &m2) {
        ir::Complex2by2Matrix res;
        const Complex *x = m1.m;
        const Complex *y = m2.m;
        Complex *r = res.m;

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

    static bool is_id(const ir::Complex2by2Matrix &mat) {
        // mat.dump();
        const Complex *m = mat.m;
        if ((abs(abs(m[0].real())-1.0))>__epsilon__) return false;
        if ((abs(m[0].imag())  )>__epsilon__) return false;
        if ((abs(m[1].real())  )>__epsilon__) return false;
        if ((abs(m[1].imag())  )>__epsilon__) return false;
        if ((abs(m[2].real())  )>__epsilon__) return false;
        if ((abs(m[2].imag())  )>__epsilon__) return false;
        if ((abs(abs(m[3].real())-1.0))>__epsilon__) return false;
        if ((abs(m[3].imag())  )>__epsilon__) return false;
        return true;
    }

    static bool is_identity(const ir::Circuit &c) {
        if (c.size() == 1) {
            return false;
        }
        ir::Complex2by2Matrix m = c[0]->mat();
        for (size_t i = 1; i < c.size(); ++i) {
            ir::Complex2by2Matrix m2 = c[i]->mat();
            m = fuse(m,m2);
        }
        return is_id(m);
    }

    static ir::Circuit optimize_sliding_window(ir::Circuit &c, size_t window_size) {
        ir::Circuit oc;
        Vec<int> id_pos;
        for (size_t i = 0; i < c.size() - window_size + 1; ++i) {
            ir::Circuit w;
            w.get_vec().insert(w.begin(),c.begin()+i,c.begin()+i+window_size);
            if (is_identity(w)) {
                id_pos.push_back(i);
            }
        }
        if (id_pos.empty()) {
            return ir::Circuit(c);
        }
        // println("id pos:");
        // for (size_t i=0; i<id_pos.size(); i++)
        // println(id_pos[i]);

        if (id_pos.size() == 1) {
            // COUT("rotations cancelling...");
            size_t pos = id_pos[0];
            size_t i=0;
            while (i < c.size()) {
                if (i == pos) {
                    i += window_size;
                } else {
                    oc.add(c[i]);
                    i++;
                }
            }
            return oc;
        }

        // COUT("removing overlapping windows...");
        Vec<int> pid;
        // int prev = id_pos[0];
        // COUT("rotation cancelling...");
        size_t pos = id_pos[0];
        size_t ip = 0;
        size_t i = 0;
        while (i < c.size()) {
            if (i == pos) {
                i += window_size;
                ip++;
                if (ip < id_pos.size()) {
                    pos = id_pos[ip];
                }
            } else {
                oc.add(c[i]);
                i++;
            }
        }

        return oc;
    }

};

inline void rotation_optimize_kernel(const ir::KernelRef &kernel, const plat::PlatformRef &platform) {
    QL_DOUT("kernel " << kernel->name << " optimize_kernel(): circuit before optimizing: ");
    print(kernel->c);
    QL_DOUT("... end circuit");
    rotations_merging rm;
    if (contains_measurements(kernel->c)) {
        QL_DOUT("kernel contains measurements ...");
        // decompose the circuit
        Vec<ir::Circuit> cs = split_circuit(kernel->c);
        Vec<ir::Circuit> cs_opt;
        for (auto c : cs)
        {
            if (!contains_measurements(c)) {
                ir::Circuit opt = rm.optimize(c);
                cs_opt.push_back(opt);
            } else {
                cs_opt.push_back(c);
            }
        }
        // for (int i=0; i<cs_opt.size(); ++i)
        // print(cs_opt[i]);
        kernel->c.reset();
        for (size_t i = 0; i < cs_opt.size(); ++i) {
            for (size_t j = 0; j < cs_opt[i].size(); j++) {
                kernel->c.add(cs_opt[i][j]);
            }
        }
    } else {
        kernel->c = rm.optimize(kernel->c);
    }
    kernel->cycles_valid = false;
    QL_DOUT("kernel " << kernel->name << " rotation_optimize(): circuit after optimizing: ");
    print(kernel->c);
    QL_DOUT("... end circuit");
}

// rotation_optimize pass
void rotation_optimize(
    const ir::ProgramRef &program,
    const plat::PlatformRef &platform,
    const Str &passname
) {
    if (com::options::get("optimize") == "yes") {
        QL_IOUT("optimizing quantum kernels...");
        for (size_t k=0; k<program->kernels.size(); ++k) {
            rotation_optimize_kernel(program->kernels[k], platform);
        }
    }
}

} // namespace ql
