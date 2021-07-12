.. _dev_options:

Options (of various degrees)
============================

There are a number of ways to make a piece of code execute conditionally in
OpenQL, global- and pass options being the most obvious ones. But there are
more, with different use cases, so let's go over them.

Pass options
------------

Wherever possible, pass options are what should be used to configure things
related to passes.

Global options
--------------

Global options are primarily a remnant from before we had passes. They should
now only be used for backward compatibility, and for things that cannot be
turned into pass options due to needing to be available outside the context of
a pass. If you really need them, they are defined in ``src/ql/com/options.cc``.

Platform JSON configuration file options
----------------------------------------

Anything that has access to the IR can read arbitrary JSON from the platform
configuration data structure; the raw JSON is always maintained, and
unrecognized keys are silently ignored (somewhat by design). So, whenever you
have an optional bit of code that's logically related to the platform and that
has access to the platform, this is what you should use. Usually, though, it
makes more sense to use a pass option.

Preprocessor-based options
--------------------------

The C preprocessor can be used to set options at compile-time. There are
various ways to do this (beyond just ``#define``) in the context of CMake,
which are preferred to the usual C++ way, because they don't require a user
to modify code in order to change the option.

.. note::

    All preprocessor macros should use the ``QL_`` prefix, to avoid name
    clashes with libraries we're depending on, or (if it ever happens) the
    codebase of software depending on OpenQL within C++.

CMake ``target_compile_definitions``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

These are useful for pieces of code that shouldn't even be attempted to be
compiled when the option is off. However, the associated preprocessor directive
**must not** be used in public header files! After all, if OpenQL would be
installed and a user would link against it, they wouldn't use the CMakeLists
file at all, and thus you'd end up with arbitrary discrepancies in the header
files! When nevertheless used, they should be added at the bottom of the
"OpenQL library target" section of the main ``CMakeLists.txt`` file.

Whether the macro is defined or now would normally be controlled via a CMake
option/cache variable, created with the ``option()`` function. These directives
should be placed at the top of the main ``CMakeLists.txt`` file, in the
"Configuration options" section.

CMake ``configure_file`` options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When the above is not possible because the definition is also needed in public
header files, you can instead use ``ql/config.h``, again in conjunction with an
``option()`` directive. To do so:

 - add a ``#cmakedefine`` directive to ``src/ql/config.h.template``;
 - make sure that the name of the preprocessor directive exists as a yes/no
   CMake variable when ``config.h`` is created (also in the "OpenQL library
   target" section of the main ``CMakeLists.txt`` file); and
 - ensure that ``ql/config.h`` is included in all files that use the
   definition.

The latter is of course very important, otherwise you'll get the same kind of
discrepancies that you would for ``target_compile_definitions``. Basically,
this just shifts that problem. Choose the one that's more convenient.

``#define`` in some header file somewhere
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can do this if you want, but it should only be done for things that rarely
change. After all, you can't change the option without changing sources.
