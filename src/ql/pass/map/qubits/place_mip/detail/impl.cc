/** \file
 * Initial placement engine.
 */

#include "impl.h"

#include "Highs.h"
#include "io/FilereaderMps.h"
#include "util/HighsMatrixPic.h"

#include <filesystem>

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace place_mip {
namespace detail {

namespace {    
static constexpr double EPSILON = 0.000000001;

bool isPermutation(const utils::Vec<utils::UInt> &v2r) {
    std::set<utils::UInt> image;

    for(auto x: v2r) {
        if (x >= v2r.size() || image.count(x) > 0) {
            return false;
        }
        image.insert(x);
    }

    if (image.size() != v2r.size()) {
        return false;
    }

    return true;
}

void logNewMapping(const utils::Vec<utils::UInt> &v2r) {
    std::stringstream ss;
    ss << "Initial placement resulted in the following mapping (virtual -> real): { ";
    bool first = true;
    for(utils::UInt i = 0; i < v2r.size(); ++i) {
        if (!first) {
            ss << ", ";
        }
        first = false;
        ss << i << " -> " << v2r[i];
    }
    ss << " }";
    QL_IOUT(ss.str());
}
}

using namespace utils;

/**
 * String conversion for initial placement results.
 */
std::ostream &operator<<(std::ostream &os, Result ipr) {
    switch (ipr) {
        case Result::ANY:       os << "any";        break;
        case Result::CURRENT:   os << "current";    break;
        case Result::NEW_MAP:   os << "newmap";     break;
        case Result::FAILED:    os << "failed";     break;
        case Result::TIMED_OUT: os << "timedout";   break;
    }
    return os;
}

bool Impl::hasNonNN2QGates() {
    QL_ASSERT(!twoQGatesCount.empty());

    for (const auto& kv: twoQGatesCount) {
        QL_ASSERT(kv.first.first != kv.first.second);
        if (distanceProvider(kv.first.first, kv.first.second) > 1) {
            return true;
        }
    }

    return false;
}

Vec<Vec<UInt>> Impl::computeCostMax(const Vec<Vec<UInt>> &refcount) {
    Vec<Vec<UInt>>  costmax;
    costmax.resize(nfac);
    for (UInt i=0; i<nfac; i++) {
        costmax[i].resize(qubitsCount, 0);
    }

    for (UInt i = 0; i < nfac; i++) {
        for (UInt k = 0; k < qubitsCount; k++) {
            for (UInt j = 0; j < nfac; j++) {
                for (UInt l = 0; l < qubitsCount; l++) {
                    QL_ASSERT(distanceProvider(k, l) < utils::MAX && "All qubits in the topology should be connected");
                    costmax[i][k] += refcount[i][j] * distanceProvider(k, l);
                }
            }
        }
    }

    return costmax;
}

std::unique_ptr<HighsModel> Impl::createHiGHSModel(const Vec<Vec<UInt>> &refcount) {
    // This describes the MIP problem using HiGHS's API. Some examples can be found in deps/highs/examples.

    std::unique_ptr<HighsModel> model(new HighsModel());

    QL_ASSERT(nfac >= 2);
    QL_ASSERT(qubitsCount >= 2);

    auto& lp_ = model->lp_;

    /*
        HiGHS will solve the following problem:
            min c^T.x     subject to    L <= A.x <= U and l <= x <= u
        where   c is a vector (lp_.col_cost_), ^T is transposition, the dot (.) is matrix product,
                L and U are vectors (lp_.row_lower_, resp. lp_.row_upper_),
                l and u are vectors (resp. lp_.col_lower_ and lp_.col_upper_),
                A is a matrix (lp_.a_matrix_)

        lp_.num_col_: number of columns in A, that is, the size of x
        lp_.num_row_: number of rows in A, that is, the number of constraints

        In this case, the x vector consists of 2*nfac*qubitsCount elements:
            - the first half of them are the integers x[i][k], where i is a facility and k a location, ordered like so:
                x[0][0], x[0][1], x[0][2], ..., x[1][0], x[1][1], ...
            - the second half are the reals w[i][k], ordered the same way.

        The matrix A has nfac+qubitsCount+nfac*qubitsCount rows:
        - the first nfac rows describe forall k: ( sum i: x[i][k] <= 1 )
        - the following qubitsCount rows describe forall i: ( sum k: x[i][k] == 1 )
        - the remaining nfac * qubitsCount rows describe forall i: forall k: costmax[i][k] * x[i][k]
            + ( sum j: sum l: refcount[i][j]*distance(k,l)*x[j][l] ) - w[i][k] <= costmax[i][k]

    */

    lp_.num_col_ = 2 * nfac * qubitsCount;
    lp_.num_row_ = nfac + qubitsCount + nfac * qubitsCount;
    lp_.sense_ = ObjSense::kMinimize;
    lp_.offset_ = 0;

    lp_.col_cost_ = std::vector<double>(lp_.num_col_, 0.);
    std::fill(lp_.col_cost_.begin() + nfac * qubitsCount, lp_.col_cost_.end(), .1);

    lp_.col_lower_ = std::vector<double>(lp_.num_col_, 0.);

    // A large value for the HiGHS model, since HiGHS doesn't seem to have a way to express infinity when describing bounds.
    static constexpr double HIGHS_LARGE_VALUE = 1.0e5;

    lp_.col_upper_ = std::vector<double>(lp_.num_col_, 1.);
    std::fill(lp_.col_upper_.begin() + nfac * qubitsCount, lp_.col_upper_.end(), HIGHS_LARGE_VALUE);

    lp_.row_lower_ = std::vector<double>(lp_.num_row_, 0.);
    std::fill(lp_.row_lower_.begin() + qubitsCount, lp_.row_lower_.begin() + qubitsCount + nfac, 1.);
    std::fill(lp_.row_lower_.begin() + qubitsCount + nfac, lp_.row_lower_.end(), -HIGHS_LARGE_VALUE);

    lp_.row_upper_ = std::vector<double>(lp_.num_row_, 1.);
    
    auto costmax = computeCostMax(refcount);

    for (UInt i = 0; i < nfac; ++i) {
        for (UInt k = 0; k < qubitsCount; ++k) {
            lp_.row_upper_[qubitsCount + nfac + i * qubitsCount + k] = costmax[i][k];
        }
    }

    /*

        lp_.a_matrix_ is a sparse matrix, non-zero entries are described using 3 vectors index_, value_ and start_.

        It is here described row-wise. The non-zeros of each row are stored after value_[start_[<row_number>]],
        and the indices of those non-zeros in that given row are stored after index_[start_[<row_number>]].
        The last elements of start_ needs to be the total number of non-zeros (or size of value_).

        Example: the following vectors
        index_ = {1, 3, 0, 3, 3}
        value_ = {1, 2, 4, 2, 7}
        start_ = {0, 2, 4, 5}

        describe in a row-wise fashion the following sparse matrix:

        0  1  0  2
        4  0  0  2
        0  0  0  7

    */

    lp_.a_matrix_.num_col_ = lp_.num_col_;
    lp_.a_matrix_.num_row_ = lp_.num_row_;
    lp_.a_matrix_.format_ = MatrixFormat::kRowwise;
    lp_.a_matrix_.start_ = std::vector<int>();

    for (UInt k = 0; k < qubitsCount; ++k) {
        lp_.a_matrix_.start_.push_back(lp_.a_matrix_.value_.size());
        lp_.a_matrix_.value_.insert(lp_.a_matrix_.value_.end(), nfac, 1);
        for (UInt i = 0; i < nfac; ++i) {
            lp_.a_matrix_.index_.push_back(i * qubitsCount + k);
        }
    }

    for (UInt i = 0; i < nfac; ++i) {
        lp_.a_matrix_.start_.push_back(lp_.a_matrix_.value_.size());
        lp_.a_matrix_.value_.insert(lp_.a_matrix_.value_.end(), qubitsCount, 1);
        for (UInt k = 0; k < qubitsCount; ++k) {
            lp_.a_matrix_.index_.push_back(i * qubitsCount + k);
        }
    }

    for (UInt i = 0; i < nfac; ++i) {
        for (UInt k = 0; k < qubitsCount; ++k) {
            lp_.a_matrix_.start_.push_back(lp_.a_matrix_.value_.size());
            for (UInt j = 0; j < nfac; ++j) {
                for (UInt l = 0; l < qubitsCount; ++l) {
                    QL_ASSERT(distanceProvider(k, l) < utils::MAX && "All qubits in the topology should be connected");
                    double newValue = refcount[i][j] * distanceProvider(k, l);
                    if (j == i && l == k) {
                        newValue += costmax[i][k];
                    }

                    // This describes a sparse matrix; 0s don't have to be added.
                    if (std::abs(newValue) > EPSILON) {
                        lp_.a_matrix_.value_.push_back(newValue);
                        lp_.a_matrix_.index_.push_back(j * qubitsCount + l);
                    }

                }
            }
            lp_.a_matrix_.value_.push_back(-1);
            lp_.a_matrix_.index_.push_back(qubitsCount * nfac + i * qubitsCount + k);
        }
    }

    // Last element of start_ needs to be the total number of non-zero elements in the sparse matrix.
    lp_.a_matrix_.start_.push_back(lp_.a_matrix_.value_.size());

    QL_ASSERT(lp_.num_row_ > 0);
    QL_ASSERT(lp_.a_matrix_.start_.size() == static_cast<std::size_t>(lp_.num_row_) + 1);
    QL_ASSERT(lp_.a_matrix_.value_.size() == lp_.a_matrix_.index_.size());

    // Convert the matrix to col-wise since dumping the problem to a file is currently not supported for row-wise matrices.
    lp_.a_matrix_.ensureColwise();

    lp_.integrality_.resize(lp_.num_col_);
    for (UInt col = 0; col < nfac * qubitsCount; ++col) {
        lp_.integrality_[col] = HighsVarType::kInteger;
    }

    return model;
}

Result Impl::run(Vec<UInt> &v2r) {
    QL_ASSERT(v2r.size() == qubitsCount);

    if (twoQGatesCount.empty()) {
        return Result::ANY;
    }

    if (!hasNonNN2QGates()) {
        return Result::CURRENT;
    }

    // Create map from virtual qubit index to facility index.
    std::set<utils::UInt> virtualQubitsUsedIn2QGates{};
    for (const auto& kv: twoQGatesCount) {
        virtualQubitsUsedIn2QGates.insert(kv.first.first);
        virtualQubitsUsedIn2QGates.insert(kv.first.second);
    }

    Vec<UInt> v2fac;
    v2fac.resize(qubitsCount, utils::MAX);

    QL_ASSERT(nfac == 0);
    for (UInt v=0; v < qubitsCount; v++) {
        if (virtualQubitsUsedIn2QGates.count(v) >= 1) {
            v2fac[v] = nfac;
            nfac += 1;
        }
    }
    QL_DOUT("Number of facilities is: " << nfac << " while total number of virtual qubits is: " << qubitsCount);
    
    // Fill reverse mapping from facility to virtual qubit index.
    Vec<UInt> fac2v(nfac, utils::MAX);
    for (UInt v = 0; v < qubitsCount; ++v) {
        auto fac = v2fac[v];

        if (fac != utils::MAX) {
            QL_ASSERT(fac < nfac);
            QL_ASSERT(fac2v[fac] == utils::MAX);
            fac2v[fac] = v;
        }
    }

    // refcount contains # of 2q gates between given facilities.
    Vec<Vec<UInt>> refcount;
    refcount.resize(nfac);
    for (UInt i=0; i<nfac; i++) {
        refcount[i].resize(nfac,0);
    }
    for (const auto& kv: twoQGatesCount) {
        QL_ASSERT(kv.second >= 1);

        refcount[v2fac[kv.first.first]][v2fac[kv.first.second]] = kv.second;
    }

    auto model = createHiGHSModel(refcount);

    if (opts.write_model_to_file) {
        FilereaderMps().writeModelToFile(HighsOptions(), opts.model_filename, *model);
        auto lp_matrix_file_path = std::filesystem::temp_directory_path() / "LpMatrix";
        writeLpMatrixPicToFile(HighsOptions(), lp_matrix_file_path.generic_string(), model->lp_);
    }
    
    Highs highs;
    highs.passModel(*model);

    static constexpr double MIN_TIMEOUT = 0.0000001;
    if (opts.timeout > MIN_TIMEOUT) {
        highs.setOptionValue("time_limit", opts.timeout);
    }

    std::chrono::high_resolution_clock::time_point time_at_start = std::chrono::high_resolution_clock::now();

    auto return_status = highs.run();

    if (return_status != HighsStatus::kOk) {
        if (highs.getModelStatus() == HighsModelStatus::kTimeLimit) {
            return Result::TIMED_OUT;
        }
        return Result::FAILED;
    }
    
    std::chrono::high_resolution_clock::time_point time_at_end = std::chrono::high_resolution_clock::now();
    auto time_span = time_at_end - time_at_start;
    time_taken = time_span.count();

    const HighsSolution& solution = highs.getSolution();
    
    // highs.writeInfo("highs_initial_placement_info.txt");

    // *** Reconstruct mapping with MIP solution ***
    // Fill v2r with mapped facilities.
    for (UInt fac = 0; fac < nfac; fac++) {
        UInt real_qubit = 0;
        for (; real_qubit < qubitsCount; real_qubit++) {
            if (std::abs(solution.col_value[fac * qubitsCount + real_qubit] - 1) < EPSILON) {
                QL_ASSERT(fac2v[fac] < qubitsCount);
                QL_ASSERT(v2r[fac2v[fac]] == UNDEFINED_QUBIT);

                v2r[fac2v[fac]] = real_qubit;
                break;
            }
        }

        QL_ASSERT(real_qubit < qubitsCount && "Each facility has to be allocated (problem constraint)");
    }

    // Allocate randomly remaining virtual qubits, while trying to preserve the original circuit if possible.
    std::set<UInt> unused_real_qubits{};
    for (UInt r = 0; r < qubitsCount; r++) {
        unused_real_qubits.insert(r);
    }
    
    for (UInt v = 0; v < qubitsCount; v++) {
        unused_real_qubits.erase(v2r[v]);
    }

    for (UInt v = 0; v < qubitsCount; v++) {
        if (v2r[v] == UNDEFINED_QUBIT) {
            QL_ASSERT(!unused_real_qubits.empty());

            auto it = unused_real_qubits.find(v);
            if (it != unused_real_qubits.end()) {
                v2r[v] = v;
                unused_real_qubits.erase(it);
            }
        }
    }

    for (UInt v = 0; v < qubitsCount; v++) {
        if (v2r[v] == UNDEFINED_QUBIT) {
            QL_ASSERT(!unused_real_qubits.empty());

            v2r[v] = *unused_real_qubits.begin();
            unused_real_qubits.erase(unused_real_qubits.begin());
        }
    }

    QL_ASSERT(isPermutation(v2r));

    logNewMapping(v2r);

    return Result::NEW_MAP;
}

Impl::Impl(UInt aQubitsCount, const TwoQGatesCount &aTwoQGatesCount, DistanceProvider aDistanceProvider, const Options &aOpts)
    : qubitsCount(aQubitsCount), twoQGatesCount(aTwoQGatesCount), distanceProvider(aDistanceProvider), opts(aOpts) {}

/**
 * Returns the amount of time taken by the mixed-integer-programming solver
 * for the call to run() in seconds.
 */
utils::Real Impl::getTimeTaken() const {
    return time_taken;
}

Impl::TwoQGatesCount inventorize2QGates(ir::Ref ir) {
    class Inventorize2QGates : public ir::RecursiveVisitor {
    public:
        Inventorize2QGates(ir::Ref aIr, Impl::TwoQGatesCount &m) : ir(aIr), twoQGatesCount(m) {}

        void visit_node(ir::Node &) override {}

        void visit_instruction_decomposition(ir::InstructionDecomposition &) override {}

        void visit_custom_instruction(ir::CustomInstruction &instr) override {
            std::pair<UInt, UInt> qubit_operands;
            std::uint8_t n_operands = 0;
            for (auto &op : instr.operands) {
                if (op->as_reference() && op->as_reference()->data_type->as_qubit_type()) {
                    auto qubit_index = op->as_reference()->indices[0]->as_int_literal()->value;
                    
                    if (n_operands >= 2) {
                        QL_FATAL("Gate: " << instr.instruction_type->name << " has more than 2 operand qubits; please decompose such gates first before mapping.");
                    }

                    if (n_operands == 0) {
                        qubit_operands.first = qubit_index;
                    } else {
                        qubit_operands.second = qubit_index;
                    }

                    ++n_operands;
                }
            }

            if (n_operands == 2) {
                ++twoQGatesCount[qubit_operands];
            }
        }

    private:
        ir::Ref ir;
        Impl::TwoQGatesCount &twoQGatesCount;
    };

    Impl::TwoQGatesCount twoQGatesCount{};

    Inventorize2QGates inventorize{ir, twoQGatesCount};

    ir->visit(inventorize);

    return twoQGatesCount;
}

void applyHorizon(UInt horizon, Impl::TwoQGatesCount &twoQGatesCount) {
    if (horizon == 0) {
        return;
    }

    if (horizon > twoQGatesCount.size()) {
        return;
    }

    std::vector<Impl::TwoQGatesCount::iterator> iterators;
    auto it = twoQGatesCount.begin();
    while (it != twoQGatesCount.end()) {
        iterators.push_back(it);
        ++it;
    }

    std::sort(iterators.begin(), iterators.end(), [](Impl::TwoQGatesCount::iterator it1, Impl::TwoQGatesCount::iterator it2) {
        return it1->second > it2->second;
    });

    for (auto itit = std::next(iterators.begin(), horizon); itit != iterators.end(); ++itit) {
        twoQGatesCount.erase(*itit);
    }

    QL_ASSERT(twoQGatesCount.size() == horizon);
}

Result performInitialPlacement(ir::Ref ir, const Options &opts, utils::Vec<utils::UInt> &mapping) {
    auto twoQGatesCount = inventorize2QGates(ir);

    applyHorizon(opts.horizon, twoQGatesCount);

    Impl::DistanceProvider platformDistanceProvider = [ir](UInt q1, UInt q2) {
        return ir->platform->topology->get_distance(q1, q2);
    };

    return Impl(ir->platform->qubits->shape[0], twoQGatesCount, platformDistanceProvider, opts).run(mapping);
};

} // namespace detail
} // namespace place
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql