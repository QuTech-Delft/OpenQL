.. _creating_your_first_program:

Creating your first program
===========================

In the OpenQL framework, the quantum :class:`Program <openql.Program>`, its
:class:`Kernel <openql.Kernel>`\ s, and its :meth:`gate <openql.Kernel.gate()>`\ s
are created using API calls contained in a C++ or Python 3 program. You then
run this program in order to compile the quantum program. In the manual, we'll
use Python exclusively, but
:ref:`the API is largely identical in C++ <ref_cpp>`.

You can set up Python however you like; via a Jupyter/IPython notebook, a
Python file created in a text editor that you then run, various Python IDEs
like IDLE, and so on. Just make sure that OpenQL is actually installed for the
interpreter that your preferred environment uses.

The very first step is to import the OpenQL module:

.. code-block:: python

    import openql as ql

.. note::

    In versions before 0.9, you had to use the more verbose
    ``from openql import openql as ql`` syntax. This is still supported for
    backward compatibility, but is deprecated.

Next, you should call the
:func:`initialize() <openql.initialize()>` function:

.. code-block:: python

    ql.initialize()

This function ensures that OpenQL is (re)initialized to its default
configuration. This is especially important in the context of a test suite or
IPython notebook, where one might want to do multiple compilation runs in a
single Python instance. For backward compatibility, OpenQL will automatically
call this function when you first use a dependent API function, warning you as
it does.

After initialization, you may want to change some of OpenQL's global
:ref:`options <ref_options>`. One of the important ones is ``output_dir``,
which is used to specify which directory the compiler's output will be placed
in. If you don't set it, OpenQL will default to outputting to a ``test_output``
directory within the current working directory. Another important one is
``log_level``, which sets the verbosity of OpenQL's logging; if you don't set
that one, you won't see any log messages, at least until something has already
gone horribly wrong.

.. code-block:: python

    ql.set_option('output_dir', 'output')
    ql.set_option('log_level', 'LOG_INFO')

.. note::

    OpenQL will automatically recursively create directories whenever it tries
    to write a file. Thus, you don't have to manually create the output
    directory.

.. note::

    As of version 0.9, you can also opt to use the more powerful, but slightly
    more complicated :class:`Compiler <openql.Compiler>` API to set options
    and manipulate the compilation strategy. Since version 0.9, almost all of
    the global options have no effect other than manipulating the default
    compilation strategy and pass options. You can obtain a reference to the
    :class:`Compiler <openql.Compiler>` object used by a
    :class:`Platform <openql.Platform>` or :class:`Program <openql.Program>`
    using the :meth:`get_compiler() <openql.Platform.get_compiler()>` method,
    or you can construct a :class:`Compiler <openql.Compiler>` manually and
    use its :meth:`compile() <openql.Compiler.compile()>` function to compile
    the program (rather than ``program.compile()``).

Before you can start building a quantum program, you must create a
:class:`Platform <openql.Platform>` object. One of its constructor parameters
is either the name of the :ref:`architecture<ref_architectures>` you want to
compile for, a reference to a
:ref:`platform configuration file<ref_platform_configuration>`, or (via
the :meth:`from_json() <openql.Platform.from_json()>` constructor) a JSON
object specified by way of Python dictionaries, lists, strings, integers, and
booleans with the same structure as the platform configuration file. The
platform configuration is consulted by the APIs creating the program, kernels,
and gates, to generate the matching internal representation of each gate.

For now, let's use the "none" architecture:

.. code-block:: python

    platform = ql.Platform('my_platform', 'none')

This is a basic architecture that is most useful for simulating with QX. The
first argument is only used to identify the platform in error messages; you can
set it to whatever you like.

After creating the platform, the :class:`program <openql.Program>` and its
:class:`Kernel <openql.Kernel>`\ s may be created. The program and kernel
constructors take the program/kernel name, the associated platform, and the
number of qubits used in it as parameters. We'll use 3 in this example:

.. code-block:: python

    nqubits = 3
    program = ql.Program('my_program', platform, nqubits)
    kernel = ql.Kernel('my_kernel', platform, nqubits)

When needed, the number of used CRegs (classical integer registers) and BRegs
(bit registers) used by the program/kernel must also be specified, but we don't
use these for now.

Again, the first argument is just a name. However, unlike for the platform, the
name is actually used. Specifically, the program name is used as a prefix for
the output files, and the kernel names are used in various places where a
unique name is needed (thus, they must actually be unique).

Once you have a kernel, you can add gates to it:

.. code:: python

    for i in range(nqubits):
        kernel.prepz(i)

    kernel.x(0)
    kernel.h(1)
    kernel.cz(2, 0)
    kernel.measure(0)
    kernel.measure(1)

Most gates have a shorthand function, as used above. However, some
architecture-specific gates might not, or might need additional arguments. For
these cases, the :meth:`gate() <openql.Kernel.gate()>` method can be used.

.. note::

    You can only add gates that are registered via the instruction set
    definition in the platform configuration structure, or for which a
    decomposition rule exists. If a gate doesn't exist there, you will
    immediately get an exception. In future versions, this exception may be
    delayed to when you call :meth:`compile() <openql.Program.compile()>`.

When you're done adding gates to a kernel, you can add the kernel to the
program using :meth:`add_kernel() <openql.Program.add_kernel()>`:

.. code:: python

    program.add_kernel(kernel)

.. note::

    The number of qubits, CRegs, and BRegs used by a kernel must be less than
    or equal to the number used by the program, and the number for the program
    must be less than or equal to what the number available in the platform.
    Also, a kernel can only be added to a program when the kernel and program
    were constructed using the same platform.

Finally, when you have completed the program, you can compile it using the
:meth:`compile() <openql.Program.compile()` function:

.. code:: python

    program.compile()

Here's the completed program, taken from ``examples/simple.py``:

.. literalinclude:: ../../examples/simple.py
   :language: python3

When you run this file with Python, two files will be generated in the
``output`` directory: ``my_program.qasm`` and ``my_program_scheduled.qasm``.
The first look like this:

.. literalinclude:: ../gen/my_program.qasm

This file is generated by a cQASM writer pass before anything else is done.
As you can see, it contains exactly what was generated by the Python program,
but in
`cQASM 1.0 <https://libqasm.readthedocs.io/en/latest/cq1-structure.html>`_
format.

The second is a little more interesting:

.. literalinclude:: ../gen/my_program_scheduled.qasm

It is generated after basic ALAP (as late as possible) scheduling, using the
(rather arbitrary) instruction durations specified in the default platform
configuration file for the "none" architecture.

Depending on the architecture and compiler configuration, different output
files may be generated. The above only applies because of the default pass
list doe the "none" architecture: a cQASM writer, followed by a scheduler,
followed by another cQASM writer. This is fully configurable.
