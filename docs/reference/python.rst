.. _ref_python:

Python API
==========

To use OpenQL from Python, you need to install the ``qutechopenql`` module
using ``pip``, and then

.. code-block:: python3

   import openql as ql

.. note::
   It used to be necessary to use ``import openql.openql as ql``. This is
   still supported for backward compatibility.

The typical usage pattern for OpenQL is as follows:

 - call :func:`initialize() <openql.initialize>` to initialize the OpenQL
   library and clean up any leftovers from compiling a previous program;
 - set some global options with :func:`set_option() <openql.set_option>`;
 - build a :class:`Platform <openql.Platform>`;
 - build a :class:`Program <openql.Program>` using the platform;
 - build one or more :class:`Kernel <openql.Kernel>` s using the platform,
   and add them to the program with
   :meth:`add_kernel() <openql.Program.add_kernel>`;
 - compile the program with :meth:`compile() <openql.Program.compile>`.

.. note::
   The :func:`initialize() <openql.initialize>` didn't use to exist. Therefore,
   for backward compatibility, it is called automatically by the constructor
   of :class:`Platform <openql.Platform>`, the constructor of
   :class:`Compiler <openql.Compiler>`, or by
   :func:`set_option() <openql.set_option>` if it has not been called yet
   within this Python interpreter.

.. warning::
   Calling :meth:`Program.compile() <openql.Program.compile>` or
   :meth:`Compiler.compile() <openql.Compiler.compile>` multiple times on the
   same program is currently *not* a supported use case: the ``compile()``
   function mutates the contents of the program as compilation progresses.
   There are currently no API methods on ``Program`` or ``Kernel`` to read
   back the compilation result, but these may be added in the future.
   Therefore, if you want to compile a program multiple times, you'll have to
   rebuild the program from scratch each time.

For more advanced usage of the OpenQL compiler, the default compilation
strategy might not be good enough, or the global options may be too restrictive
for what you want. For this reason, the :class:`Compiler <openql.Compiler>`
interface was recently added. The easiest way to make use of it is through
:meth:`Platform.get_compiler() <openql.Platform.get_compiler>` or
:meth:`Program.get_compiler() <openql.Program.get_compiler>`; this returns a
reference that allows you to change the default compilation strategy or set
options for particular passes. Once you do this, however, any changes made to
global options will cease to have an effect on that particular
Platform/Program/Compiler triplet; you *must* use
:meth:`Compiler.set_option() <openql.Compiler.set_option>` and friends from
that point onwards. Note that the names of the options in this interface have
been revised compared to the global options, so you can't just replace a
global ``set_option()`` with a ``Compiler.set_option()`` without a bit of work.

Index
-----

.. automodule:: openql
   :noindex:


   .. rubric:: Regular functions

   .. autosummary::

      initialize
      ensure_initialized
      compile
      get_version
      set_option
      get_option

   .. rubric:: Classes

   .. autosummary::

      Platform
      Program
      Kernel
      CReg
      Operation
      Unitary
      Compiler
      Pass
      cQasmReader

   .. rubric:: Documentation retrieval functions

   .. autosummary::

      print_options
      dump_options
      print_architectures
      dump_architectures
      print_passes
      dump_passes
      print_resources
      dump_resources
      print_platform_docs
      dump_platform_docs

Platform class
--------------

.. automodule:: openql
   :members: Platform

Program class
-------------

.. automodule:: openql
   :members: Program

Kernel class
------------

.. automodule:: openql
   :members: Kernel

CReg class
----------

.. automodule:: openql
   :members: CReg

Operation class
---------------

.. automodule:: openql
   :members: Operation

Unitary class
-------------

.. automodule:: openql
   :members: Unitary

Compiler class
--------------

.. automodule:: openql
   :members: Compiler

Pass class
----------

.. automodule:: openql
   :members: Pass

cQasmReader class
-----------------

.. automodule:: openql
   :members: cQasmReader

Functions and miscellaneous
---------------------------

.. automodule:: openql
   :members:
   :exclude-members: Platform Program Kernel CReg Operation Unitary Compiler Pass cQasmReader
