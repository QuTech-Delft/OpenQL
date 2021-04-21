The openql module
=================

To use OpenQL from Python, you need to install the :code:`qutechopenql` module using :code:`pip`, and then

.. code-block:: python3

   import openql as ql

.. note::
   It used to be necessary to use :code:`import openql.openql as ql`. This is still supported for backward compatibility.

The typical usage pattern for OpenQL is as follows:

 - set some global options with :func:`set_option() <openql.set_option>`;
 - build a :class:`Platform <openql.Platform>`;
 - build a :class:`Program <openql.Program>` using the platform;
 - build one or more :class:`Kernel <openql.Kernel>` s using the platform, and add them to the program with :meth:`add_kernel() <openql.Program.add_kernel>`;
 - compile the program with :meth:`compile() <openql.Program.compile>`.

For more advanced usage of the OpenQL compiler, the default compilation strategy might not be good enough, or the global options may be too restrictive for what you want. For this reason, the :class:`Compiler <openql.Compiler>` interface was recently added. The easiest way to make use of it is through :meth:`Platform.get_compiler() <openql.Platform.get_compiler>` or :meth:`Program.get_compiler() <openql.Program.get_compiler>`; this returns a reference that allows you to change the default compilation strategy or set options for particular passes. Once you do this, however, any changes made to global options will cease to have an effect on that particular Platform/Program/Compiler triplet; you *must* use :meth:`Compiler.set_option() <openql.Compiler.set_option>` and friends from that point onwards. Note that the names of the options in this interface have been revised compared to the global options, so you can't just replace a global ``set_option()`` with a ``Compiler.set_option()`` without a bit of work.

.. automodule:: openql
   :noindex:


   .. rubric:: Functions

   .. autosummary::

      initialize
      ensure_initialized
      get_version
      set_option
      get_option
      print_options

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


.. automodule:: openql
   :members:
