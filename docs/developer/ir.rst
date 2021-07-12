.. _dev_ir:

Intermediate representation
===========================

The IR is the most important data structure of any compiler. It must be able to
encapsulate any program that can be compiled, during any compilation stage. The
compiler passes can then be defined to just be transformations on this IR
structure.

Compilers sometimes have multiple IR structures that they switch between at
certain points in the compilation process. This is not the case for OpenQL:
there is only one IR structure that all passes must operate on, though it's
of course legal for a pass to temporarily build its own private structures,
which may be especially important for complex code generation passes.
Noteworthy for OpenQL however, is that the platform (a.k.a. target in many
other compilers) is part of the IR structure in OpenQL: it shouldn't be
modified by any passes, but it lives in the same tree structure for reasons
we'll get back to in a few paragraphs.

The old IR
----------

Despite only having a single logical IR, OpenQL is currently schizophrenic
about its IR in its own way: at some point a new IR (``ql::ir``) was developed
to replace the old one (now in ``ql::ir::compat``, previously ``ql::ir`` and
``ql::plat``), but not all passes have been converted to use the new IR yet.
Thus, for any pass that hasn't been converted yet, the pass management logic
first converts the new IR to the old one, then runs the legacy pass
implementation, and then converts back to the new IR. This process is usually
transparant, but there are a few gotchas:

 - kernel names may change in some cases due to name uniquification logic;
 - annotations places on nodes other than the program, a kernel, or a gate will
   be lost;
 - any new-to-old conversion implicitly runs all legacy gate decompositions; and
 - obviously, any features supported by the new IR but not by the old IR will
   result in compatibility errors during the new-to-old conversion.

Unfortunately, even when all passes have been converted, the old IR cannot be
removed, because it is highly intertwined with the Python/C++ APIs for program
construction (primarily, adding gates to a kernel immediately applies
decomposition rules, which have a bunch of oddities that a reimplementation
probably wouldn't be able to mimic exactly). Thus, as long as we need to
maintain compatibility with old OpenQL programs, we'll be stuck with it.
However, the intention is to use the old IR exclusively within the API in the
future, run the (only) old-to-new conversion in
``program.compile()``/``compiler.compile()`` just before calling the pass
manager, and remove the new-to-old conversion logic.

The platform construction logic is similarly difficult, partly because the old
platform construction logic needs to be maintained because of the above, and
partly because the platform construction logic is very intertwined with the
old IR and operations on it. Thus, the platform side of the old-to-new
conversion will also remain relevant. In fact, this conversion is currently
where platform features only supported in the new IR are parsed; there is
unfortunately no saner place to do this.

If you're not already familier with the old IR and you're not here to work on
or upgrade legacy code, it's probably best to ignore the existence of the old
IR as much as possible. If you nevertheless need to learn more about the old
IR, you can:

 - try to make sense of the classes and functions in ``ql::ir::compat``;
 - try to make sense of the "old pages" section in these docs, if it still
   exists when you're reading this; or
 - read the documentation of an older version of OpenQL (prior to PR #405, so
   commit ``7f2e2bb`` or earlier).

For the rest of this section, we'll ignore the existence of the old IR and its
conversions to and from the new IR.

The new IR
----------

Describing a tree structure in C++ requires a lot of boilerplate code and
repetition. To avoid some of this, the C++ classes for the IR are generated
using `tree-gen <https://tree-gen.readthedocs.io/>`_. This tool was developed
specifically for OpenQL and libqasm, but doesn't depend on OpenQL (or libqasm)
in any way, so it is written and documented in a generic way. This also means
that its quirks are out of scope for this documentation; nevertheless, it is
vital that you understand how ``tree-gen`` tree structures conceptually work
before trying to understand OpenQL's IR structure. To prevent you from having
to jump back and forth *too* much, here are a few things that might not be
immediately apparent.

 - The tree definition file format was a bit rushed, and in part because of
   that its structure might be unintuitive when you're not used to it yet.
   Most importantly, the ``{}``-based structure of the tree definition file
   does *not* correspond to the tree structure being described, but rather to
   the class inheritance tree of the nodes that can be used *within* the
   described tree. Also, while the node names are written in lower_case in
   the tree description file, they are converted to TitleCase for the C++
   class names (this is simply because ``tree-gen`` needs both forms, and
   it's easier to convert from lower_case to TitleCase than the other way
   around).

 - There is no well-defined root node in a ``tree-gen`` tree, and (somewhat
   equivalently) ``tree-gen`` nodes do not know who their parent is. For this
   reason, most functions operating on OpenQL's IR take the root node of the
   IR (``ql::ir::Root``, or more typically ``ql::ir::Ref``, which typedefs to
   ``ql::utils::One<ql::ir::Root>``) as their first argument. This also means
   that a node can be reused in multiple trees. But be careful: nodes are
   almost always stored and passed by means of (ultimately) a
   ``std::shared_ptr<>`` reference with mutable content, so if a node ends up
   being shared, changing it in one tree will also effectively change it in the
   other tree. You can use the generated ``copy()`` and ``clone()`` methods to
   respectively make a shallow or deep copy of a node if need be.

 - Besides the usual DAG edges in a tree graph, ``tree-gen`` trees support
   so-called "link" edges as well. A tree node can have any number of links
   pointing to it from anywhere else in the tree, even if this forms a loop.
   Links are useful to for example implement a variable reference node, or
   to refer to a data type node within the referenced variable definition node.
   This avoids having to keep track of unique names everywhere, and avoids a
   map lookup. Essentially, the "name" of a linked node is the pointer to its
   data structure. The tree consistency check ensures that a node is only used
   once in a tree, thus ensuring uniqueness of its pointer (and preventing a
   lot of other mayhem due to accidental reference reuse). It also ensures that
   all links in a tree actually link to nodes that are reachable from the root
   node via non-link edges: this is the primary reason why the platform
   description has to be part of the IR tree.

 - There are six "edge types" for connecting nodes together: Maybe, One, Any,
   Many, OptLink, and Link. These correspond to zero or one edge, exactly one
   edge, zero or more edges, one or more edges, zero or one link, and exactly
   one link. Note however that it's actually possible for One, Link, and Many
   to represent zero edges; doing that would merely make the tree consistency
   check fail. This is useful while building a tree or operating on it.

 - Tree nodes, or anything else that implements the ``Annotatable`` type
   defined by ``tree-gen``'s support library (``ql::utils::tree`` in OpenQL),
   can be "annotated" with zero or one of literally any C++ type. You can think
   of them like a fancy, type-safe ``void*`` field in every node. This allows
   passes to attach information to nodes temporarily, allows storage of
   metadata that doesn't really belong in the tree explicitly, and so on.
   Keep in mind, however, that annotations are stored by reference, and cannot
   be copied unless you already know which annotation types exist (just like a
   ``void*``). Also, annotations are not copied by the ``copy()`` and
   ``clone()`` methods; if you intend to copy annotation *references* you must
   use ``copy_annotations()`` in addition to copying/cloning the node, and if
   you really need to copy an annotation by value you must use
   ``copy_annotation<AnnotationType>()``.

With ``tree-gen``-specific stuff out of the way, the IR definition itself
should be rather straight-forward based on its tree definition file. So,
instead of duplicating documentation in a way that will inevitably desync with
the implementation, here's the contents of that file (``src/ql/ir/ir.tree``).

.. literalinclude:: ../../src/ql/ir/ir.tree
   :language: text
