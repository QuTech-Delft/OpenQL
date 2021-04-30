
%feature("docstring") ql::api::Compiler
"""
Wrapper for the compiler/pass manager.

You can get access to a Compiler via several methods:

 - using Platform.get_compiler();
 - using Program.get_compiler();
 - using one of the constructors.

Using the constructors, you can get an empty compiler (by specifying no
arguments or only specifying name), a default compiler for a given platform
(by specifying a name and a platform), or a compiler based on a compiler
configuration JSON file (by specifying a name and a filename). This JSON file
must have the following structure:

.. code-block::

   {
       \"architecture\": <optional string, default \"\">,
       \"dnu\": <optional list of strings, default []>,
       \"pass-options\": <optional object, default {}>,
       \"compatibility-mode\": <optional boolean, default false>,
       \"passes\": [
           <pass description>
       ]
   },

The optional \"architecture\" key may be used to make shorthands for
architecture- specific passes, normally prefixed with
\"arch.<architecture>.\". If it's not specified or an empty string, no
shorthand aliases are made.

The optional \"dnu\" key may be used to specify a list of do-not-use pass
types (experimental passes, deprecated passes, or any other pass that's
considered unfit for \"production\" use) that you explicitly want to use,
including the \"dnu\" namespace they are defined in. Once specified, you'll
be able to use the pass type without the \"dnu\" namespace element. For
example, if you would include \"dnu.whatever\" in the list, the pass type
\"whatever\" may be used to add the pass.

The optional \"pass-options\" key may be used to specify options common to
all passes. The values may be booleans, integers, strings, or null, but
nothing else. Null is used to reset an option to its hardcoded default
value. An option need not exist for each pass affected by it; if it
doesn't, the default value is silently ignored for that pass. However, if
it *does* exist, it must be a valid value for the option with that name.
These option values propagate through the pass tree recursively, so
setting a default option in the root using this record will affect all
passes.

If \"compatibility-mode\" is enabled, some of OpenQL's global options add
implicit entries to the \"pass-options\" structure when set, for backward
compatibility. However, entries in \"pass-options\" always take precedence.
The logic for which options map to which is mostly documented in the
global option docs now, since those options don't do anything else
anymore. Note that the global options by their original design have no
way to specify what pass they refer to, so each option is attempted for
each pass type! Which means we have to be a bit careful with picking
option names for the passes that are included in compatibility mode.

Pass descriptions can either be strings (in which case the string is
interpreted as a pass type alias and everything else is
inferred/default), or an object with the following structure.

.. code-block::

   {
       \"type\": <optional string, default \"\">,
       \"name\": <optional string, default \"\">,
       \"options\": <optional object, default {}>
       \"group-options\": <optional object, default {}>,
       \"group\": [
           <optional list of pass descriptions>
       ]
   }

The \"type\" key, if specified, must identify a pass type that OpenQL knows
about. If it's not specified or empty, a group is made instead, and
\"group\" must be specified for the group to do anything.

The \"name\" key, if specified, is a user-defined name for the pass, that
must match ``[a-zA-Z0-9_\\\\-]+`` and be unique within the surrounding pass
list. If not specified, a name that complies with these requirements is
generated automatically, but the actual generated name should not be
relied upon to be consistent between OpenQL versions. The name may be
used to programmatically refer to passes after construction, and passes
may use it for logging or unique output filenames. However, passes should
not use the name for anything that affects the behavior of the pass.

The \"options\" key, if specified, may be an object that maps option names
to option values. The values may be booleans, integers, strings, or null,
but nothing else. Null is used to enforce usage of the OpenQL-default
value for the option. The option names and values must be supported by
the particular pass type.

The \"group-options\" key, if specified, works just like \"pass-options\" in
the root, but affects only the sub-passes of this pass (so *not* this
pass itself). Any option specified here will override any
previously-specified option. Specifying null resets the option to its
OpenQL-hardcoded default value.

The \"group\" key must only be used when \"type\" is set to an empty string
or left unspecified, turning the pass into a basic group. The list then
specifies the sub-passes for the group. A normal pass may or may not have
configurable sub-passes depending on its type and configuration; if it
doesn't, \"group\" must not be specified.
"""


%feature("docstring") ql::api::Compiler::name
"""
User-given name for this compiler.

NOTE: not actually used for anything. It's only here for consistency with
the rest of the API objects.
"""


%feature("docstring") ql::api::Compiler::print_pass_types
"""
Prints documentation for all available pass types, as well as the option
documentation for the passes.

Parameters
----------
None

Returns
-------
None
"""


%feature("docstring") ql::api::Compiler::dump_pass_types
"""
Returns documentation for all available pass types, as well as the option
documentation for the passes.

Parameters
----------
None

Returns
-------
str
    The list of pass types and their documentation as a multiline string.
"""


%feature("docstring") ql::api::Compiler::print_strategy
"""
Prints the currently configured compilation strategy.

Parameters
----------
None

Returns
-------
None
"""


%feature("docstring") ql::api::Compiler::dump_strategy
"""
Returns the currently configured compilation strategy as a string.

Parameters
----------
None

Returns
-------
str
    The current compilation strategy as a multiline string.
"""


%feature("docstring") ql::api::Compiler::set_option
"""
Sets a pass option. Periods are used as hierarchy separators; the last
element will be the option name, and the preceding elements represent
pass instance names. Furthermore, wildcards may be used for the pass name
elements (asterisks for zero or more characters and a question mark for a
single character) to select multiple or all immediate sub-passes of that
group, and a double asterisk may be used for the element before the
option name to chain to set_option_recursively() instead. The return
value is the number of passes that were affected; passes are only
affected when they are selected by the option path AND have an option
with the specified name. If must_exist is set an exception will be thrown
if none of the passes were affected, otherwise 0 will be returned.

Parameters
----------
path : str
    The path to the option, consisting of pass names and the actual option name
    separated by periods.
value : str
    The value to set the option to.
must_exist : bool
    When set, an exception will be thrown when no options matched the path.

Returns
-------
int
    The number of pass options affected.
"""


%feature("docstring") ql::api::Compiler::set_option_recursively
"""
Sets an option for all passes recursively. The return value is the number
of passes that were affected; passes are only affected when they have an
option with the specified name. If must_exist is set an exception will be
thrown if none of the passes were affected, otherwise 0 will be returned.

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


%feature("docstring") ql::api::Compiler::get_option
"""
Returns the current value of an option. Periods are used as hierarchy
separators; the last element will be the option name, and the preceding
elements represent pass instance names.

Parameters
----------
path : str
    The path to the option, consisting of pass names and the actual option name
    separated by periods.

Returns
-------
str
    The value of the option. If the option has not been set, the default value
    is returned.
"""


%feature("docstring") ql::api::Compiler::append_pass
"""
Appends a pass to the end of the pass list. If type_name is empty
or unspecified, a generic subgroup is added. Returns a reference to the
constructed pass.

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


%feature("docstring") ql::api::Compiler::prefix_pass
"""
Appends a pass to the beginning of the pass list. If type_name is empty
or unspecified, a generic subgroup is added. Returns a reference to the
constructed pass.

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


%feature("docstring") ql::api::Compiler::insert_pass_after
"""
Inserts a pass immediately after the target pass (named by instance). If
target does not exist, an exception is thrown. If type_name is empty or
unspecified, a generic subgroup is added. Returns a reference to the
constructed pass. Periods may be used in target to traverse deeper into
the pass hierarchy.

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


%feature("docstring") ql::api::Compiler::insert_pass_before
"""
Inserts a pass immediately before the target pass (named by instance). If
target does not exist, an exception is thrown. If type_name is empty or
unspecified, a generic subgroup is added. Returns a reference to the
constructed pass. Periods may be used in target to traverse deeper into
the pass hierarchy.

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


%feature("docstring") ql::api::Compiler::group_pass
"""
Looks for the pass with the target instance name, and embeds it into a
newly generated group. The group will assume the name of the original
pass, while the original pass will be renamed as specified by sub_name.
Note that this ultimately does not modify the pass order. If target does
not exist or this pass is not a group of sub-passes, an exception is
thrown. Returns a reference to the constructed group. Periods may be used
in target to traverse deeper into the pass hierarchy.

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


%feature("docstring") ql::api::Compiler::group_passes
"""
Like group_pass(), but groups an inclusive range of passes into a
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


%feature("docstring") ql::api::Compiler::flatten_subgroup
"""
Looks for an unconditional pass group with the target instance name and
flattens its contained passes into its parent group. The names of the
passes found in the collapsed group are prefixed with name_prefix before
they are added to the parent group. Note that this ultimately does not
modify the pass order. If the target instance name does not exist or is
not an unconditional group, an exception is thrown. Periods may be used
in target to traverse deeper into the pass hierarchy.

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


%feature("docstring") ql::api::Compiler::get_pass
"""
Returns a reference to the pass with the given instance name. If no such
pass exists, an exception is thrown. Periods may be used as hierarchy
separators to get nested sub-passes.

Parameters
----------
target : str
    The name of the pass to retrieve a reference to.

Returns
-------
Pass
    A reference to the targeted pass.
"""


%feature("docstring") ql::api::Compiler::does_pass_exist
"""
Returns whether a pass with the target instance name exists. Periods may
be used in target to traverse deeper into the pass hierarchy.

Parameters
----------
target : str
    The name of the pass to query existence of.

Returns
-------
bool
    Whether a pass with the target name exists.
"""


%feature("docstring") ql::api::Compiler::get_num_passes
"""
Returns the total number of passes in the root hierarchy.

Parameters
----------
None

Returns
-------
int
    The number of passes (or groups) within the root pass list.
"""


%feature("docstring") ql::api::Compiler::get_passes
"""
Returns a list with references to all passes in the root hierarchy.

Parameters
----------
None

Returns
-------
list[Pass]
    The list of all passes in the root hierarchy.
"""


%feature("docstring") ql::api::Compiler::get_passes_by_type
"""
Returns a list with references to all passes in the root hierarchy with the
given type.

Parameters
----------
target : str
    The target pass type name.

Returns
-------
list[Pass]
    The list of all passes in the root hierarchy with the given type.
"""


%feature("docstring") ql::api::Compiler::remove_pass
"""
Removes the pass with the given target instance name, or throws an
exception if no such pass exists.

Parameters
----------
target : str
    The name of the pass to remove.

Returns
-------
None
"""


%feature("docstring") ql::api::Compiler::clear_passes
"""
Clears the entire pass list.

Parameters
----------
None

Returns
-------
None
"""


%feature("docstring") ql::api::Compiler::construct
"""
Constructs all passes recursively. This freezes the pass options, but
allows subtrees to be modified.

Parameters
----------
None

Returns
-------
None
"""


%feature("docstring") ql::api::Compiler::compile
"""
Ensures that all passes have been constructed, and then runs the passes
on the given program. This is the same as Program.compile() when the
program is referencing the same compiler.

Parameters
----------
program : Program
    The program to compile.

Returns
-------
None
"""


%include "ql/api/compiler.h"
