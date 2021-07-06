/** \file
 * Defines static information about operator types and names, such as their
 * associativity and precedence level.
 */

#include "ql/ir/operator_info.h"

namespace ql {
namespace ir {

/**
 * Metadata for operators as they appear in cQASM (or just logically in
 * general). Used to avoid excessive parentheses when printing expressions.
 * The first element in the key pair is the function name, the second is the
 * number of operands.
 */
const utils::Map<utils::Pair<utils::Str, utils::UInt>, OperatorInfo> OPERATOR_INFO = {
    {{"operator?:",  3}, { 1, OperatorAssociativity::RIGHT, "",  " ? ",   " : "}},
    {{"operator||",  2}, { 2, OperatorAssociativity::LEFT,  "",  " || ",  ""}},
    {{"operator^^",  2}, { 3, OperatorAssociativity::LEFT,  "",  " ^^ ",  ""}},
    {{"operator&&",  2}, { 4, OperatorAssociativity::LEFT,  "",  " && ",  ""}},
    {{"operator|",   2}, { 5, OperatorAssociativity::LEFT,  "",  " | ",   ""}},
    {{"operator^",   2}, { 6, OperatorAssociativity::LEFT,  "",  " ^ ",   ""}},
    {{"operator&",   2}, { 7, OperatorAssociativity::LEFT,  "",  " & ",   ""}},
    {{"operator==",  2}, { 8, OperatorAssociativity::LEFT,  "",  " == ",  ""}},
    {{"operator!=",  2}, { 8, OperatorAssociativity::LEFT,  "",  " != ",  ""}},
    {{"operator<",   2}, { 9, OperatorAssociativity::LEFT,  "",  " < ",   ""}},
    {{"operator>",   2}, { 9, OperatorAssociativity::LEFT,  "",  " > ",   ""}},
    {{"operator<=",  2}, { 9, OperatorAssociativity::LEFT,  "",  " <= ",  ""}},
    {{"operator>=",  2}, { 9, OperatorAssociativity::LEFT,  "",  " >= ",  ""}},
    {{"operator<<",  2}, {10, OperatorAssociativity::LEFT,  "",  " << ",  ""}},
    {{"operator<<<", 2}, {10, OperatorAssociativity::LEFT,  "",  " <<< ", ""}},
    {{"operator>>",  2}, {10, OperatorAssociativity::LEFT,  "",  " >> ",  ""}},
    {{"operator>>>", 2}, {10, OperatorAssociativity::LEFT,  "",  " >>> ", ""}},
    {{"operator+",   2}, {11, OperatorAssociativity::LEFT,  "",  " + ",   ""}},
    {{"operator-",   2}, {11, OperatorAssociativity::LEFT,  "",  " - ",   ""}},
    {{"operator*",   2}, {12, OperatorAssociativity::LEFT,  "",  " * ",   ""}},
    {{"operator/",   2}, {12, OperatorAssociativity::LEFT,  "",  " / ",   ""}},
    {{"operator//",  2}, {12, OperatorAssociativity::LEFT,  "",  " // ",  ""}},
    {{"operator%",   2}, {12, OperatorAssociativity::LEFT,  "",  " % ",   ""}},
    {{"operator**",  2}, {13, OperatorAssociativity::RIGHT, "",  " ** ",  ""}},
    {{"operator-",   1}, {14, OperatorAssociativity::RIGHT, "-", "",      ""}},
    {{"operator+",   1}, {14, OperatorAssociativity::RIGHT, "+", "",      ""}},
    {{"operator~",   1}, {14, OperatorAssociativity::RIGHT, "~", "",      ""}},
    {{"operator!",   1}, {14, OperatorAssociativity::RIGHT, "!", "",      ""}},
};

} // namespace ir
} // namespace ql
