
%feature("docstring") ql::api::Pass
"""
Wrapper for a pass that belongs to some pass manager.

NOTE: while it's possible to construct a pass manually, the resulting object
cannot be used in any way. The only way to obtain a valid pass object is through
a Compiler object.
"""


%feature("docstring") ql::api::Pass::get_type
"""
Returns the full, desugared type name that this pass was constructed with.

Parameters
----------
None

Returns
-------
str
    The type name.
"""


%feature("docstring") ql::api::Pass::get_name
"""
Returns the instance name of the pass within the surrounding group.

Parameters
----------
None

Returns
-------
str
    The instance name.
"""


%feature("docstring") ql::api::Pass::print_pass_documentation
"""
Prints the documentation for this pass.

Parameters
----------
None

Returns
-------
None
"""


%feature("docstring") ql::api::Pass::dump_pass_documentation
"""
Returns the documentation for this pass as a string.

Parameters
----------
None

Returns
-------
str
    The documentation for this pass as a multiline string.
"""


%feature("docstring") ql::api::Pass::print_options
"""
Prints the current state of the options.

Parameters
----------
only_set : bool
    When set, only the options that were explicitly configured are dumped.

Returns
-------
None
"""


%feature("docstring") ql::api::Pass::dump_options
"""
Returns the string printed by print_options().

Parameters
----------
only_set : bool
    When set, only the options that were explicitly configured are dumped.

Returns
-------
str
    The option documentation as a multiline string.
"""


%feature("docstring") ql::api::Pass::print_strategy
"""
Prints the entire compilation strategy including configured options of
this pass and all sub-passes.

Parameters
----------
None

Returns
-------
None
"""


%feature("docstring") ql::api::Pass::dump_strategy
"""
Returns the string printed by print_strategy().

Parameters
----------
None

Returns
-------
str
    The current compilation strategy as a multiline string.
"""


%feature("docstring") ql::api::Pass::set_option
"""
Sets an option. Periods may be used as hierarchy separators to set
options for sub-passes; the last element will be the option name, and the
preceding elements represent pass instance names. Furthermore, wildcards
may be used for the pass name elements (asterisks for zero or more
characters and a question mark for a single character) to select multiple
or all immediate sub-passes of that group, and a double asterisk may be
used for the element before the option name to chain to
set_option_recursively() instead. The return value is the number of
passes that were affected; passes are only affected when they are
selected by the option path AND have an option with the specified name.
If must_exist is set an exception will be thrown if none of the passes
were affected, otherwise 0 will be returned.

Parameters
----------
option : str
    The option name or path to the subpass option, the latter consisting of pass
    names and the actual option name separated by periods.
value : str
    The value to set the option to.
must_exist : bool
    When set, an exception will be thrown when no options matched the path.

Returns
-------
int
    The number of pass options affected.
"""


%feature("docstring") ql::api::Pass::set_option_recursively
"""
Sets an option for all sub-passes recursively. The return value is the
number of passes that were affected; passes are only affected when they
have an option with the specified name. If must_exist is set an exception
will be thrown if none of the passes were affected, otherwise 0 will be
returned.

Parameters
----------
option : str
    The name of the option.
value : str
    The value to set the option to.
must_exist : bool
    When set, an exception will be thrown when there are no passes with an
    option with this name.

Returns
-------
int
    The number of pass options affected.
"""


%feature("docstring") ql::api::Pass::get_option
"""
Returns the current value of an option. Periods may be used as hierarchy
separators to get options from sub-passes (if any).

Parameters
----------
path : str
    The path to the option.

Returns
-------
str
    The value of the option. If the option has not been set, the default value
    is returned.
"""


%feature("docstring") ql::api::Pass::construct
"""
Constructs this pass. During construction, the pass implementation may
decide, based on its options, to become a group of passes or a normal
pass. If it decides to become a group, the group may be introspected or
modified by the user. The options are frozen after this, so set_option()
will start throwing exceptions when called. construct() may be called any
number of times, but becomes no-op after the first call.

Parameters
----------
None

Returns
-------
None
"""


%feature("docstring") ql::api::Pass::is_constructed
"""
Returns whether this pass has been constructed yet.

Parameters
----------
None

Returns
-------
bool
    Whether this pass has been constructed yet.
"""


%feature("docstring") ql::api::Pass::is_group
"""
Returns whether this pass has configurable sub-passes.

Parameters
----------
None

Returns
-------
bool
    Whether this pass has configurable sub-passes.
"""


%feature("docstring") ql::api::Pass::is_collapsible
"""
Returns whether this pass is a simple group of which the sub-passes can
be collapsed into the parent pass group without affecting the strategy.

Parameters
----------
None

Returns
-------
bool
    Whether this pass is a simple group of which the sub-passes can
    be collapsed into the parent pass group without affecting the strategy.
"""


%feature("docstring") ql::api::Pass::is_root
"""
Returns whether this is the root pass group in a pass manager.

Parameters
----------
None

Returns
-------
bool
    Whether this is the root pass group in a pass manager.
"""


%feature("docstring") ql::api::Pass::is_conditional
"""
Returns whether this pass contains a conditionally-executed group.

Parameters
----------
None

Returns
-------
bool
    Whether this pass contains a conditionally-executed group.
"""


%feature("docstring") ql::api::Pass::append_sub_pass
"""
If this pass constructed into a group of passes, appends a pass to the
end of its pass list. Otherwise, an exception is thrown. If type_name is
empty or unspecified, a generic subgroup is added. Returns a reference to
the constructed pass.

Parameters
----------
type_name : str
    The type of the pass to add. If empty or unspecified, a group will be added.
instance_name : str
    A unique name for the pass instance. If empty or unspecified, a name will
    be generated.
options : dict[str, str]
    A list of initial options to set for the pass. This is just shorthand
    notation for calling set_option() on the returned Pass object.

Returns
-------
Pass
    A reference to the added pass.
"""


%feature("docstring") ql::api::Pass::prefix_sub_pass
"""
If this pass constructed into a group of passes, appends a pass to the
beginning of its pass list. Otherwise, an exception is thrown. If
type_name is empty or unspecified, a generic subgroup is added. Returns a
reference to the constructed pass.

Parameters
----------
type_name : str
    The type of the pass to add. If empty or unspecified, a group will be added.
instance_name : str
    A unique name for the pass instance. If empty or unspecified, a name will
    be generated.
options : dict[str, str]
    A list of initial options to set for the pass. This is just shorthand
    notation for calling set_option() on the returned Pass object.

Returns
-------
Pass
    A reference to the added pass.
"""


%feature("docstring") ql::api::Pass::insert_sub_pass_after
"""
If this pass constructed into a group of passes, inserts a pass
immediately after the target pass (named by instance). If target does not
exist or this pass is not a group of sub-passes, an exception is thrown.
If type_name is empty or unspecified, a generic subgroup is added.
Returns a reference to the constructed pass. Periods may be used in
target to traverse deeper into the pass hierarchy.

Parameters
----------
target : str
    The name of the pass to insert the new pass after.
type_name : str
    The type of the pass to add. If empty or unspecified, a group will be added.
instance_name : str
    A unique name for the pass instance. If empty or unspecified, a name will
    be generated.
options : dict[str, str]
    A list of initial options to set for the pass. This is just shorthand
    notation for calling set_option() on the returned Pass object.

Returns
-------
Pass
    A reference to the added pass.
"""


%feature("docstring") ql::api::Pass::insert_sub_pass_before
"""
If this pass constructed into a group of passes, inserts a pass
immediately before the target pass (named by instance). If target does
not exist or this pass is not a group of sub-passes, an exception is
thrown. If type_name is empty or unspecified, a generic subgroup is
added. Returns a reference to the constructed pass. Periods may be used
in target to traverse deeper into the pass hierarchy.

Parameters
----------
target : str
    The name of the pass to insert the new pass before.
type_name : str
    The type of the pass to add. If empty or unspecified, a group will be added.
instance_name : str
    A unique name for the pass instance. If empty or unspecified, a name will
    be generated.
options : dict[str, str]
    A list of initial options to set for the pass. This is just shorthand
    notation for calling set_option() on the returned Pass object.

Returns
-------
Pass
    A reference to the added pass.
"""


%feature("docstring") ql::api::Pass::group_sub_pass
"""
If this pass constructed into a group of passes, looks for the pass with
the target instance name, and embeds it into a newly generated group. The
group will assume the name of the original pass, while the original pass
will be renamed as specified by sub_name. Note that this ultimately does
not modify the pass order. If target does not exist or this pass is not a
group of sub-passes, an exception is thrown. Returns a reference to the
constructed group. Periods may be used in target to traverse deeper into
the pass hierarchy.

Parameters
----------
target : str
    The name of the pass to embed into a new group.
sub_name : str
    The new name for the pass, after inserting it into the new group.

Returns
-------
Pass
    A reference to the constructed pass group.
"""


%feature("docstring") ql::api::Pass::group_sub_passes
"""
Like group_sub_pass(), but groups an inclusive range of passes into a
group with the given name, leaving the original pass names unchanged.
Periods may be used in from/to to traverse deeper into the pass
hierarchy, but the hierarchy prefix must be the same for from and to.

Parameters
----------
from : str
    The name of the first pass to embed into the new group.
to : str
    The name of the last pass to embed into the new group.
group_name : str
    The name for the group.

Returns
-------
Pass
    A reference to the constructed pass group.
"""


%feature("docstring") ql::api::Pass::flatten_subgroup
"""
If this pass constructed into a group of passes, looks for the pass with
the target instance name, treats it as a generic group, and flattens its
contained passes into the list of sub-passes of its parent. The names of
the passes found in the collapsed subgroup are prefixed with name_prefix
before they are added to the parent group. Note that this ultimately does
not modify the pass order. If target does not exist, does not construct
into a group of passes (construct() is called automatically), or this
pass is not a group of sub-passes, an exception is thrown. Periods may be
used in target to traverse deeper into the pass hierarchy.

Parameters
----------
target : str
    The name of the group to flatten.
name_prefix : str
    An optional prefix for the pass names as they are moved from the subgroup to
    the parent group.

Returns
-------
None
"""


%feature("docstring") ql::api::Pass::get_sub_pass
"""
If this pass constructed into a group of passes, returns a reference to
the pass with the given instance name. If target does not exist or this
pass is not a group of sub-passes, an exception is thrown. Periods may be
used as hierarchy separators to get nested sub-passes.

Parameters
----------
target : str
    The name of the pass to retrieve a reference to.

Returns
-------
Pass
    A reference to the targeted pass.
"""


%feature("docstring") ql::api::Pass::does_sub_pass_exist
"""
If this pass constructed into a group of passes, returns whether a
sub-pass with the target instance name exists. Otherwise, an exception is
thrown. Periods may be used in target to traverse deeper into the pass
hierarchy.

Parameters
----------
target : str
    The name of the pass to query existence of.

Returns
-------
bool
    Whether a pass with the target name exists.
"""


%feature("docstring") ql::api::Pass::get_num_sub_passes
"""
If this pass constructed into a group of passes, returns the total number
of immediate sub-passes. Otherwise, an exception is thrown.

Parameters
----------
None

Returns
-------
int
    The number of passes (or groups) within the root pass list.
"""


%feature("docstring") ql::api::Pass::get_sub_passes
"""
If this pass constructed into a group of passes, returns a list with references
to all sub-passes.

Parameters
----------
None

Returns
-------
list[Pass]
    The list of all passes in the root hierarchy.
"""


%feature("docstring") ql::api::Pass::get_sub_passes_by_type
"""
If this pass constructed into a group of passes, returns a list with references
to all sub-passes with the given type.

Parameters
----------
target : str
    The target pass type name.

Returns
-------
list[Pass]
    The list of all passes in the root hierarchy with the given type.
"""


%feature("docstring") ql::api::Pass::remove_sub_pass
"""
If this pass constructed into a group of passes, removes the sub-pass
with the target instance name. If target does not exist or this pass is
not a group of sub-passes, an exception is thrown. Periods may be used in
target to traverse deeper into the pass hierarchy.

Parameters
----------
target : str
    The name of the pass to remove.

Returns
-------
None
"""


%feature("docstring") ql::api::Pass::clear_sub_passes
"""
If this pass constructed into a group of passes, removes all sub-passes.
Otherwise, an exception is thrown.

Parameters
----------
None

Returns
-------
None
"""


%include "ql/api/pass.h"
