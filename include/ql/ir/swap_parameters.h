#pragma once

#include "ql/utils/num.h"

namespace ql {
namespace ir {

struct SwapParameters {
    utils::Bool part_of_swap = false;
    // at the end of the swap r0 stores v0 and r1 stores v1
    utils::Int r0 = -1;
    utils::Int r1 = -1;
    utils::Int v0 = -1;
    utils::Int v1 = -1;

    // default constructor
    SwapParameters() {}

    // initializer list
    SwapParameters(utils::Bool _part_of_swap, utils::Int _r0, utils::Int _r1, utils::Int _v0, utils::Int _v1)
        : part_of_swap(_part_of_swap), r0(_r0), r1(_r1), v0(_v0), v1(_v1)
    {}
};

}
}
