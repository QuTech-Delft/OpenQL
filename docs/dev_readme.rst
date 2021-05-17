.. _dev_readme:

Where to begin
==============

So you want to contribute to OpenQL? Or perhaps you're employed to help
maintain it? Great!

OpenQL has grown to be quite a large project, so you may be feeling a bit
overwhelmed. I know I was. I'm assuming you already know what OpenQL is when
you get here, otherwise please read through the user documentation first.
But after that, where to begin...?

First of all, you should make sure that you're able to build and test OpenQL
on your own machine. So follow the :ref:`dev_build`, and if you run into
any problems, ask an existing maintainer or open an issue.

If you'll be touching the Python API, you'll also want to follow the
instructions for building the documentation locally; there's all kinds of
generation magic from the API docstrings and documentation getters that might
fall over if you change the wrong thing, and documentation generation is not
currently tested by CI.

Once done, you'll want to get some sort of IDE configured, so you can click
through the code. I use CLion; they have free educational licenses for anyone
with a university e-mail address, and it works okay.

Before changing anything, please read through the *whole* section on
:ref:`dev_conventions` or ``CONTRIBUTING.md`` (the content is the same). This
section describes more than just what should be capitalized and whether
braces go on the same or the next line for consistency; it also goes over
the general organization of the code, how to include things to make sure
everything works everywhere, and what rules need to be followed with regards
to the documentation ``dump_*()`` functions in order for the reStructuredText
generators in ``docs/`` to keep working.

Familiarize yourself with what's available in the ``ql::utils`` namespace.
This was added as a wrapper around the C++ standard library to offer additional
runtime safety, improve type naming consistency with the non-STL types defined
by OpenQL itself, and improve debugging. Depending on how used you are to C++
programming, you'll probably either love it, hate it, or both. But please,
*please* use it anyway, to keep OpenQL's codebase consistent.

In general, please think twice if you feel the need to type ``std::`` or
include a standard library header directly. Most things are wrapped (although
it's virtually impossible to be complete).

Avoid adding new native dependencies. If you *really* need to, the build system
should be made smart enough for things to work out of the box even if the
dependency is not installed: your additions should automatically be disabled if
they can't be built, but the rest of OpenQL can. You can do this via
preprocessor macros, but be aware that you can only use those in ``src``! Files
in ``include`` are public, and can thus be built with any preprocessor macro
set when included by user C++ code. You can look at the code for unitary
decomposition, MIP-based placement, or the visualizer if you're not sure how to
work with these constraints; those pieces of code all do this.

When you've added something, don't forget to add yourself to
``CONTRIBUTORS.md``!

These were just some general pointers I came up with on a whim, so this is most
likely not complete. If you feel like something is missing, feel free to add to
this list!
