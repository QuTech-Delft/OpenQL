#include <iostream>

#include "ql/utils/rangemap.h"

using namespace ql::utils;

int main() {
    RangeMap<UInt, UInt> map([](const UInt &a, const UInt &b) { return a == b; });

    QL_ASSERT(map.key_lt(10, 20));
    QL_ASSERT(!map.key_lt(20, 20));
    QL_ASSERT(!map.key_lt(30, 20));

    QL_ASSERT(map.key_le(10, 20));
    QL_ASSERT(map.key_le(20, 20));
    QL_ASSERT(!map.key_le(30, 20));

    QL_ASSERT(!map.key_gt(10, 20));
    QL_ASSERT(!map.key_gt(20, 20));
    QL_ASSERT(map.key_gt(30, 20));

    QL_ASSERT(!map.key_ge(10, 20));
    QL_ASSERT(map.key_ge(20, 20));
    QL_ASSERT(map.key_ge(30, 20));

    QL_ASSERT(!map.key_eq(10, 20));
    QL_ASSERT(map.key_eq(20, 20));
    QL_ASSERT(!map.key_eq(30, 20));

    QL_ASSERT(map.key_ne(10, 20));
    QL_ASSERT(!map.key_ne(20, 20));
    QL_ASSERT(map.key_ne(30, 20));

    QL_ASSERT(map.range_valid({10, 20}));
    QL_ASSERT(map.range_valid({20, 20}));
    QL_ASSERT(!map.range_valid({30, 20}));

    QL_ASSERT(!map.range_empty({10, 20}));
    QL_ASSERT(map.range_empty({20, 20}));
    QL_ASSERT(!map.range_empty({30, 20}));

    QL_ASSERT(!map.range_envelop({6, 8}, {10, 20}));
    QL_ASSERT(!map.range_envelop({8, 10}, {10, 20}));
    QL_ASSERT(!map.range_envelop({8, 18}, {10, 20}));
    QL_ASSERT(map.range_envelop({8, 20}, {10, 20}));
    QL_ASSERT(map.range_envelop({8, 22}, {10, 20}));
    QL_ASSERT(!map.range_envelop({10, 18}, {10, 20}));
    QL_ASSERT(map.range_envelop({10, 20}, {10, 20}));
    QL_ASSERT(map.range_envelop({10, 22}, {10, 20}));
    QL_ASSERT(!map.range_envelop({12, 18}, {10, 20}));
    QL_ASSERT(!map.range_envelop({12, 20}, {10, 20}));
    QL_ASSERT(!map.range_envelop({12, 22}, {10, 20}));
    QL_ASSERT(!map.range_envelop({20, 22}, {10, 20}));
    QL_ASSERT(!map.range_envelop({22, 24}, {10, 20}));

    QL_ASSERT(!map.range_equal({6, 8}, {10, 20}));
    QL_ASSERT(!map.range_equal({8, 10}, {10, 20}));
    QL_ASSERT(!map.range_equal({8, 18}, {10, 20}));
    QL_ASSERT(!map.range_equal({8, 20}, {10, 20}));
    QL_ASSERT(!map.range_equal({8, 22}, {10, 20}));
    QL_ASSERT(!map.range_equal({10, 18}, {10, 20}));
    QL_ASSERT(map.range_equal({10, 20}, {10, 20}));
    QL_ASSERT(!map.range_equal({10, 22}, {10, 20}));
    QL_ASSERT(!map.range_equal({12, 18}, {10, 20}));
    QL_ASSERT(!map.range_equal({12, 20}, {10, 20}));
    QL_ASSERT(!map.range_equal({12, 22}, {10, 20}));
    QL_ASSERT(!map.range_equal({20, 22}, {10, 20}));
    QL_ASSERT(!map.range_equal({22, 24}, {10, 20}));

    QL_ASSERT(map.range_starts_before({6, 8}, {10, 20}));
    QL_ASSERT(map.range_starts_before({8, 10}, {10, 20}));
    QL_ASSERT(map.range_starts_before({8, 18}, {10, 20}));
    QL_ASSERT(map.range_starts_before({8, 20}, {10, 20}));
    QL_ASSERT(map.range_starts_before({8, 22}, {10, 20}));
    QL_ASSERT(!map.range_starts_before({10, 18}, {10, 20}));
    QL_ASSERT(!map.range_starts_before({10, 20}, {10, 20}));
    QL_ASSERT(!map.range_starts_before({10, 22}, {10, 20}));
    QL_ASSERT(!map.range_starts_before({12, 18}, {10, 20}));
    QL_ASSERT(!map.range_starts_before({12, 20}, {10, 20}));
    QL_ASSERT(!map.range_starts_before({12, 22}, {10, 20}));
    QL_ASSERT(!map.range_starts_before({20, 22}, {10, 20}));
    QL_ASSERT(!map.range_starts_before({22, 24}, {10, 20}));

    QL_ASSERT(!map.range_ends_after({6, 8}, {10, 20}));
    QL_ASSERT(!map.range_ends_after({8, 10}, {10, 20}));
    QL_ASSERT(!map.range_ends_after({8, 18}, {10, 20}));
    QL_ASSERT(!map.range_ends_after({8, 20}, {10, 20}));
    QL_ASSERT(map.range_ends_after({8, 22}, {10, 20}));
    QL_ASSERT(!map.range_ends_after({10, 18}, {10, 20}));
    QL_ASSERT(!map.range_ends_after({10, 20}, {10, 20}));
    QL_ASSERT(map.range_ends_after({10, 22}, {10, 20}));
    QL_ASSERT(!map.range_ends_after({12, 18}, {10, 20}));
    QL_ASSERT(!map.range_ends_after({12, 20}, {10, 20}));
    QL_ASSERT(map.range_ends_after({12, 22}, {10, 20}));
    QL_ASSERT(map.range_ends_after({20, 22}, {10, 20}));
    QL_ASSERT(map.range_ends_after({22, 24}, {10, 20}));

    QL_ASSERT(map.range_entirely_before({6, 8}, {10, 20}));
    QL_ASSERT(map.range_entirely_before({8, 10}, {10, 20}));
    QL_ASSERT(!map.range_entirely_before({8, 18}, {10, 20}));
    QL_ASSERT(!map.range_entirely_before({8, 20}, {10, 20}));
    QL_ASSERT(!map.range_entirely_before({8, 22}, {10, 20}));
    QL_ASSERT(!map.range_entirely_before({10, 18}, {10, 20}));
    QL_ASSERT(!map.range_entirely_before({10, 20}, {10, 20}));
    QL_ASSERT(!map.range_entirely_before({10, 22}, {10, 20}));
    QL_ASSERT(!map.range_entirely_before({12, 18}, {10, 20}));
    QL_ASSERT(!map.range_entirely_before({12, 20}, {10, 20}));
    QL_ASSERT(!map.range_entirely_before({12, 22}, {10, 20}));
    QL_ASSERT(!map.range_entirely_before({20, 22}, {10, 20}));
    QL_ASSERT(!map.range_entirely_before({22, 24}, {10, 20}));

    QL_ASSERT_EQ(map.to_string(), "empty");
    map.set({10, 20}, 10);
    map.check_consistency();
    QL_ASSERT_EQ(map.to_string(), "{[10..20): 10}");
    map.set({12, 18}, 10);
    map.check_consistency();
    QL_ASSERT_EQ(map.to_string(), "{[10..20): 10}");
    map.set({12, 18}, 6);
    map.check_consistency();
    QL_ASSERT_EQ(map.to_string(), "{[10..12): 10, [12..18): 6, [18..20): 10}");
    map.set({14, 16}, 2);
    map.check_consistency();
    QL_ASSERT_EQ(map.to_string(), "{[10..12): 10, [12..14): 6, [14..16): 2, [16..18): 6, [18..20): 10}");

    QL_ASSERT_RAISES(map.at({10, 11}));
    QL_ASSERT_EQ(map.at({10, 12}), 10);

    QL_ASSERT_EQ(map.find(9), map.end());
    QL_ASSERT_EQ(map.find(10), map.begin());
    QL_ASSERT_EQ(map.find(11), map.begin());
    QL_ASSERT_EQ(map.find(12), std::next(map.begin()));

    QL_ASSERT_EQ(map.find({0, 5}).type, RangeMatchType::NONE);
    QL_ASSERT_EQ(map.find({9, 11}).type, RangeMatchType::PARTIAL);
    QL_ASSERT_EQ(map.find({9, 13}).type, RangeMatchType::MULTIPLE);
    QL_ASSERT_EQ(map.find({10, 12}).type, RangeMatchType::EXACT);
    QL_ASSERT_EQ(map.find({9, 12}).type, RangeMatchType::SUPER);
    QL_ASSERT_EQ(map.find({11, 12}).type, RangeMatchType::SUB);

    map.set({16, 19}, 2);
    map.check_consistency();
    QL_ASSERT_EQ(map.to_string(), "{[10..12): 10, [12..14): 6, [14..19): 2, [19..20): 10}");
    map.set({11, 19}, 10);
    map.check_consistency();
    QL_ASSERT_EQ(map.to_string(), "{[10..20): 10}");
    map.set({20, 21}, 10);
    map.check_consistency();
    QL_ASSERT_EQ(map.to_string(), "{[10..21): 10}");
    map.set({9, 10}, 10);
    map.check_consistency();
    QL_ASSERT_EQ(map.to_string(), "{[9..21): 10}");
    map.set({8, 10}, 10);
    map.check_consistency();
    QL_ASSERT_EQ(map.to_string(), "{[8..21): 10}");
    map.set({10, 15}, 10, [](const UInt &a, const UInt &b) { return false; });
    map.check_consistency();
    QL_ASSERT_EQ(map.to_string(), "{[8..10): 10, [10..15): 10, [15..21): 10}");
    QL_ASSERT_RAISES(map.set({20, 10}, 3));
    map.erase({14, 16});
    map.check_consistency();
    QL_ASSERT_EQ(map.to_string(), "{[8..10): 10, [10..14): 10, [16..21): 10}");
    map.erase({13, 14});
    map.check_consistency();
    QL_ASSERT_EQ(map.to_string(), "{[8..10): 10, [10..13): 10, [16..21): 10}");
    map.erase({16, 17});
    map.check_consistency();
    QL_ASSERT_EQ(map.to_string(), "{[8..10): 10, [10..13): 10, [17..21): 10}");
    map.erase({14, 16});
    map.check_consistency();
    QL_ASSERT_EQ(map.to_string(), "{[8..10): 10, [10..13): 10, [17..21): 10}");
    map.erase({8, 10});
    map.check_consistency();
    QL_ASSERT_EQ(map.to_string(), "{[10..13): 10, [17..21): 10}");

    return 0;
}
