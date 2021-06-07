/** \file
 * Configures the custom tree-gen support library that OpenQL uses.
 */

#include "ql/utils/vec.h"
#include "ql/utils/map.h"
#include "ql/utils/exception.h"

#define TREE_NAMESPACE_BEGIN        namespace ql { namespace utils { namespace tree {
#define TREE_NAMESPACE_END          }}}
#define TREE_VECTOR(V)              ::ql::utils::Vec<V>
#define TREE_MAP(K, V)              ::ql::utils::Map<K, V>
#define TREE_MAP_SET(m, k, v)       (m).set(k) = (v)
#define TREE_RUNTIME_ERROR          ::ql::utils::Exception
#define TREE_RANGE_ERROR            ::ql::utils::Exception
