.. _dev_resources:

Resources
=========

In OpenQL, the term "resources" is reserved for *scheduling* resources.
Conceptually, a resource models a physical thing that prevents two instructions
from executing simultaneously due to resource contention, but ultimately it can
be anything that prevents an instruction from starting in a particular cycle.

Implementation
--------------

Resources can be used by various passes that somehow relate to scheduling, so
they are not an OpenQL-wide thing. Because of how fundamental they are, they
received their own namespaces and management logic, not unlike the pass manager
and pass namespaces. In fact, much of what applies for passes also applies for
resources.

Resource classes
^^^^^^^^^^^^^^^^

Resource instances are represented by a class that (ultimately) derives from
``ql::rmgr::resource_types::Base``. Unlike their pass equivalent, resource
classes normally derive from this directly. Like passes, there is no
associated object for a resource type; rather, the C++ type itself is used for
the resource type, so in principle you only have to implement one class to
define a new resource type.

Resource classes override the following methods to define their functionality:

 - ``on_dump_docs(...)``: a
   `dump function <conventions.html#runtime-documentation-and-dump-functions>`_
   that writes the documentation for the resource type. This must not depend on
   its JSON configuration; it is only called on "virgin" objects (unfortunately,
   there is no such thing as overriding static methods in C++).
 - ``on_dump_config(...)``: a dump function that prints the configuration of
   the resource in a user-friendly way.
 - ``on_dump_state(...)``: a dump function that prints the current state of the
   resource, mid-scheduling.
 - ``get_friendly_name()``: used by the documentation generation logic to get
   a user-friendly name for the resource, to use as section header.
 - ``on_initialize()``: used to initialize the state of the resource for a
   particular scheduling direction.
 - ``on_gate()``: used to test or update the state during scheduling. Its
   arguments include a gate reference, the desired start cycle for the gate,
   and whether to update the state to reflect that the given gate has actually
   been scheduled in that cycle.

The semantics of ``on_gate()`` should be as follows:

 - If the given gate can *not* be scheduled in the given cycle and ``commit``
   is false, return false.
 - If the given gate can *not* be scheduled in the given cycle and ``commit``
   is true, throw an exception.
 - If the given gate *can* be scheduled in the given cycle and ``commit``
   is false, return true.
 - If the given gate *can* be scheduled in the given cycle and ``commit``
   is true, update the resource state accordingly and return true.

Resource class instances must be copy-constructable, in such a way that the
actual state of the resource is appropriately copied. This is to allow
recursive-descent scheduling algorithms that try different solutions to be
created with them. Ideally, configuration data should *not* be copied to save
space and increase performance of the copy operation; it's advisable to store
configuration data in a separate structure and embed it into the resource class
via a ``utils::Ptr<>``.

Depending on the direction argument passed to ``on_initialize()``, resources
may assume that cycle numbers are non-decreasing or non-increasing. This often
allows old state information to be deleted, thus further reducing the overhead
of recursive-descent algorithms.

The resource factory
^^^^^^^^^^^^^^^^^^^^

For a resource type to be usable within the platform configuration file, its
class must be registered with the resource factory (``ql::rmgr::Factory``).
Registration happens in the default constructor of the class. For example,
this constructor currently contains the following line, among others like it:

.. code-block:: c++

    register_resource<resource::qubit::Resource>("Qubit");

The template argument (typedefs to) the resource class, while the string
argument defines its externally-usable type name.

.. note::

    The C++ namespace path and externally-usable type name path should be kept
    in sync! Please avoid using differing naming conventions for the two. If
    needed for backward compatibility, different aliases can be made for the
    same resource type, but the complement of the C++ name should also be
    usable as a resource type within the platform.

.. note::

    The capitalization of the resource types is chosen such to be as familiar
    as possible to Python users: the last entry represents a class, while the
    remaining period-separated entries represent module names. In C++ it works
    the same, except that resources have their own namespace in addition, so
    you end up with ``...::name::Resource`` rather than ``...::Name``. Note
    however that the CC-light resource types use lowercase names for backward
    compatibility.

After default-construction, the ``Factory`` object will be "configured" by the
resource manager. During configuration, aliases are added for the 
architecture-specific resources of the selected architecture, preventing the
user from having to explicitly prefix these resources using
``arch.<arch-name>.``. This mechanism also allows an architecture to override
the implementation of a generic resource if it needs to, without breaking
backward compatibility, as architecture-specific resources take precedence over
generic resources when these aliases are created. Aliases may also be generated
for "dnu" (do-not-use) resources that are explicitly requested by the user.

The resource manager
^^^^^^^^^^^^^^^^^^^^

Resource instances are bundled together by the resource manager
(``ql::rmgr::Manager``), stored as part of the platform structures. The
resource manager is not much more than a wrapper around a list of resources,
the JSON loading logic to create this list, and boilerplate code for
documentation generation stuff. Once the list of resources is complete,
schedulers can use its ``build()`` method to construct a clean
``ql::rmgr::State`` object.

The state object
^^^^^^^^^^^^^^^^

The state object, like the resource manager, is just a wrapper around a list
of resources. It differs in where it's used: the state object tracks the
state of all the resources during scheduling, while the resource manager is
conceptually just a builder for that state. The state object presents the
following interface to scheduling algorithms:

 - ``available()``: used to query whether a gate can be scheduled in a
   particular cycle (it calls ``on_gate()`` for all resources with commit
   set to false, returning true only if all resources returned true);
 - ``reserve()``: used to update the state to reflect that the given gate
   is scheduled in the given cycle; and
 - the copy constructor/copy assignment operator: to allow a state to be
   copied for recursive-descent scheduling algorithms.

Adding a new resource
---------------------

Having read the above, adding a new resource should be a fairly straightforward
process. Nevertheless, here's a checklist that should handle the common cases.

 - Figure out what you want to call the resource, keeping in mind the naming
   conventions.

 - Create a source file for the resource corresponding to the resource type you
   settled on in ``src/ql/resource``, and an accompanying header file in
   ``include/ql/resource``. It's probably easiest to copypaste the contents
   from an existing resource.

 - Implement the documentation generation functions. If you can't be bothered
   to put anything useful there until you're done with the implementation yet
   then that's on you, but at least put a one-liner placeholder there. Don't
   just copypaste the documentation of another pass!

 - Put an appropriate placeholders in ``on_initialize()`` and ``on_gate()``,
   such as ``QL_ICE("not yet implemented")``.

 - Register your resource with the resource factory by adding it to its default
   constructor.

 - At this point, you should have everything needed for the user to be able to
   add the resource to the platform configuration file, and for the
   documentation generation system to detect and add it.

 - Actually implement and document the resource. Unlike for passes, any state
   tracked by the resource must be part of the resource object, and the
   implementation of the resource is usually (at least for the existing
   resources) not so complicated that it warrants its own detail namespace or
   even types and functions outside of the resource class, but of course you're
   free to make one anyway as per the naming conventions if you prefer.
