.. _dev_passes:

Passes
======

Fundamentally, compiler passes are bits of code that transform or analyze the
current IR in some way. For example, a scheduler pass will change the cycle
numbers of instructions and possibly reorder them accordingly.

Conceptually, OpenQL's passes can get a bit more complicated than that, at
least internally. Instead of a linear pass list, OpenQL's pass manager supports
a tree of passes (useful for instance to set options for a whole group of
passes at once, for establishing logging or performance monitoring regions if
that's ever implemented, or otherwise manipulate a group of passes as if it's
a single pass), and even has basic support for conditional passes, or repeating
a group of passes until a condition is true. To support all of this without
making simple cases needlessly complex for users, the lifecycle of a pass is
nontrivial.

 - First, the pass is created via an ``append_pass()`` API call (or
   equivalent).
 - Options may then be set on the pass via ``set_option()`` API calls (or
   equivalent).
 - At some point, the pass is "constructed." This doesn't imply C++
   construction of the pass class (this happens at the start of this list);
   rather, this is about the ``construct()`` method. The user can either call
   this directly, or the pass manager will do it automatically when needed.
   During construction, a pass can choose to become a *group* of passes, rather
   than staying a single pass. This can be a normal group, a conditional group,
   a "while" group, or a "repeat until" group. It can make this decision based
   on its options; therefore, to avoid potential confusion, it's illegal to
   change the options of a pass after it has been constructed. Instead, if the
   pass turned itself into a group, the user is subsequently allowed to modify
   its list of sub-passes, including setting options directly on the
   sub-passes, or adding new sub-passes. These sub-passes will eventually be
   constructed again, repeating this process recursively.
 - If the pass did not turn into a group, it will eventually be "run" for the
   given IR tree. A single pass may be run multiple times if it's a sub-pass
   of a looping block, or it may never be run. If the pass did decide to turn
   into a group upon construction, the overridable ``run()`` method is never
   called; instead, the base class for the pass will ensure that its sub-passes
   are run appropriately.

As an example of when this might become useful, imagine that eventually the
mapper pass is split up into its logical sub-passes, namely optional initial
placement, routing, and primitive decomposition. At this point, legacy user
code may still assume that the old combined mapper pass exists, and behaves
as a single pass within the context of pass management. To support this, the
old mapper pass can be defined to construct into a group of its sub-passes,
with the initial placement pass added only if initial placement is actually
enabled. Now, until the user explicitly constructs the mapper pass (which the
old code would have no reason to ever do), the mapper pass behaves exactly as
it would have before, i.e. a single pass that can have options applied on it
or be deleted, yet the compiler necessarily behaves exactly as if the user had
created the initial placement, routing, and primitive decomposition passes
manually.

.. note::

    Most of the APIs related to pass groups and management thereof are
    currently disabled via the ``QL_HIERARCHICAL_PASS_MANAGEMENT`` preprocessor
    directive, explicitly ``#undef``'d in ``src/ql/config.h.template``.
    Internally, however, everything should already be there.

Implementation
--------------

Pass classes
^^^^^^^^^^^^

Pass instances and/or groups of passes are represented by a class that
(ultimately) derives from ``ql::pmgr::pass_types::Base``. There is no
associated object for a pass type; rather, the C++ type itself is used for the
pass type, so in principle you only have to implement one class to define a
pass.

Passes don't normally implement ``ql::pmgr::pass_types::Base`` directly.
Instead, they may use:

 - ``ql::pmgr::pass_types::Transformation`` for regular transformation passes
   operating on the new IR;
 - ``ql::pmgr::pass_types::Analysis`` for new-IR passes that don't change the
   IR, aside from possibly adding metadata to it via annotations;
 - ``ql::pmgr::pass_types::ProgramTransformation`` for old-IR passes that
   transform the complete program in one go;
 - ``ql::pmgr::pass_types::KernelTransformation`` for old-IR passes that
   transform one kernel at a time;
 - ``ql::pmgr::pass_types::ProgramAnalysis`` for old-IR passes that analyze
   the complete program in one go; or
 - ``ql::pmgr::pass_types::KernelAnalysis`` for old-IR passes that analyze one
   kernel at a time.

Note that there is currently no difference between the ``*Transformation`` and
``*Analysis`` passes, because there is currently no good way to guarantee
constness with the IR tree. They're really just hints right now.

Most passes override the following methods to define their functionality:

 - ``dump_docs(...)``: a
   `dump function <conventions.html#runtime-documentation-and-dump-functions>`_
   that writes the documentation for the pass type. This must not depend on any
   pass options; it is only called on "virgin" objects (unfortunately, there is
   no such thing as overriding static methods in C++).
 - ``get_friendly_name()``: used by the documentation generation logic to get
   a user-friendly name for the pass, to use as section header.
 - The constructor: used to define pass-specific options.
 - ``run(...)``: actually runs the pass.

The pass class itself is not the correct place to store variables/fields for
the actual algorithm that ``run()`` implements. Instead, if the implementation
of a pass is complex, it's better to make a ``detail`` namespace for the
pass-specific types and functions, and leave the pass class as a thin wrapper
around it. Ideally, these wrappers (and pass registration) can then ultimately
be generated, preventing a lot of boilerplate code. Even without the generator,
it's good to have the boilerplate and documentation generation stuff separate
from the actual implementation; the implementation will probably be complex
enough as it is.

The pass factory
^^^^^^^^^^^^^^^^

For a pass type to be usable within a compilation strategy, its class must be
registered with the pass factory (``ql::pmgr::Factory``) used to build the
strategy with. While the code is written such that it's possible for a user
program to eventually make its own pass factory (which would probably be
necessary to let them define their own passes), currently everything just uses
a default-constructed ``Factory`` object initially, and its default constructor
is where the pass types are registered. For example, this constructor currently
contains the following line, among others like it:

.. code-block:: c++

    register_pass<::ql::pass::io::cqasm::read::Pass>("io.cqasm.Read");

The template argument (typedefs to) the pass class, while the string argument
defines its externally-usable type name.

.. note::

    The C++ namespace path and externally-usable type name path should be kept
    in sync! Please avoid using differing naming conventions for the two. If
    needed for backward compatibility, different aliases can be made for the
    same pass type, but the complement of the C++ name should also be usable
    as a pass type externally.

.. note::

    The capitalization of the pass types is chosen such to be as familiar as
    possible to Python users: the last entry represents a class, while the
    remaining period-separated entries represent module names. In C++ it works
    the same, except that passes have their own namespace in addition, so you
    end up with ``...::name::Pass`` rather than ``...::Name``.

After default-construction, the ``Factory`` object will be "configured" by the
pass manager. During configuration, aliases are added for the 
architecture-specific passes of the selected architecture, preventing the user
from having to explicitly prefix these passes using ``arch.<arch-name>.``. This
mechanism also allows an architecture to override the implementation of a
generic pass if it needs to, without breaking backward compatibility, as
architecture-specific passes take precedence over generic passes when these
aliases are created. Aliases may also be generated for "dnu" (do-not-use)
passes that are explicitly requested by the user.

The pass manager
^^^^^^^^^^^^^^^^

Pass instances are glued together into a pass strategy by the pass manager
(``ql::pmgr::Manager``), also known as just the ``Compiler`` in API
terminology. For the most part, this class is just boilerplate around a factory
and a single group pass that represents the first level of the pass group
hierarchy. However, it also contains a bunch of backward compatibility logic
from the olden days when there was no pass management at all by way of the
``from_defaults()`` and ``convert_global_to_pass_options()`` methods, and the
compiler configuration JSON file loading logic by way of the ``from_json()``
method.

``convert_global_to_pass_options()`` especially requires a bit of attention,
because its implementation is currently very stupid: whenever a global option
is defined, it effectively calls ``set_option()`` on any default pass that
has an option going by the (converted) global option name. This may not be
good enough when more passes are added eventually; for example, if multiple
passes have a ``heuristic`` option, the global option conversion logic has no
way of only setting the option for a particular pass type (incidentally, this
is why the scheduler heuristic pass option is redundantly named
``scheduler_heuristic`` instead).

Adding a new pass
-----------------

Having read the above, adding a new pass should be a fairly straightforward
process. Nevertheless, here's a checklist that should handle the common cases.

 - Figure out what you want to call the pass, keeping in mind the naming
   conventions and organizing groups (i.e. ``ana``, ``io``, ``map``, ``opt``,
   and ``sch``, see `namespaces <conventions.html#namespaces>`_).

 - Create a source file for the pass corresponding to the pass type you
   settled on in ``src/ql/pass``, and an accompanying header file in
   ``include/ql/pass``. The contents can mostly be copypasted from existing
   passes; much of it is boilerplate.

 - Derive from the right base class for your pass (probably ``Transformation``
   or ``Analysis``). If needed, change the prototype of the ``run()`` function
   accordingly.

 - Implement the documentation generation functions. If you can't be bothered
   to put anything useful there until you're done with the implementation yet
   then that's on you, but at least put a one-liner placeholder there. Don't
   just copypaste the documentation of another pass!

 - Update the constructor to define the pass options you want for your pass.

 - Put an appropriate placeholder in ``run()``, such as
   ``QL_ICE("not yet implemented")``.

 - Register your pass with the pass factory by adding it to its default
   constructor.

 - At this point, you should have everything needed for the user to be able to
   create the pass, and for the documentation generation system to detect and
   add it.

 - If you want the pass to become part of the default pass list, add it to
   ``ql::pmgr::Manager::from_defaults()``. Note that it should probably be
   guarded by a global option that defaults to not inserting the pass for
   backward compatibility; these are defined in
   ``ql::com::options::make_ql_options()``.

 - If you want the pass to become part of an architecture-specific default pass
   list, add it to the ``populate_backend_passes()`` method of its ``Info``
   class.

 - Actually implement and document the pass. If the implementation is complex,
   it should be put in a ``detail`` namespace within the pass namespace, with
   all (private!) header files and source files in the ``src`` directory. Any
   header file that must be public or is used elsewhere within OpenQL, for
   example one containing annotation types that other passes may want to do
   something with as well, should *not* be in ``detail``; ``detail`` is your
   private implementation, anything outside of it is public.
