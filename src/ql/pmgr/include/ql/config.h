/** \file
 * Configuration file for things that have to be configured via header file
 * versus being compile-time only.
 */

#pragma once

// Whether ql::utils::Vec should guard against undefined behavior in iterators.
/* #undef QL_CHECKED_VEC */

// Whether ql::utils::List should guard against undefined behavior in
// iterators.
/* #undef QL_CHECKED_LIST */

// Whether ql::utils::Map should guard against undefined behavior in iterators.
/* #undef QL_CHECKED_MAP */

// Whether OpenQL was built as a static or dynamic library.
/* #undef QL_SHARED_LIB */

// Whether (experimental) pass group/hierarchy support is enabled in the API.
#undef QL_HIERARCHICAL_PASS_MANAGEMENT

// Enable hacks to allow wait/barrier gates to appear in decompositions for the
// old IR.
#define OPT_DECOMPOSE_WAIT_BARRIER
