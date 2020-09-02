.. _creating_your_first_program:

Creating your first Program
===========================

In the OpenQL framework,
the quantum program (including kernels and gates)
is created by API calls which are contained in a C++ or Python program.

But before this is done,
the platform object is created by an API call
that takes the name of the platform configuration file as one of its parameters.
This platform configuration file is consulted by the APIs creating the program,
kernels and gates to generate the matching internal representation of each gate.

After creating the platform, the program and kernels are created.
The program creation API takes the program name, the platform object
and the number of qubits that are used in the program as parameters.
And similarly for each kernel.
After this, each kernel can be populated with gates.
This is again done by API calls, one per gate.

After having added each kernel to the program, the program can be compiled.
This leaves several output files in the *test_output* directory.
When compiling for CC-Light which is one of the hardware platforms of OpenQL,
one will find there a *.qisa* file which then can be executed on the platform.
But one will also find there several *.qasm* files which can be simulated by e.g. QX.

Let us start creating a program.

To begin working with OpenQL, you can start up python however you like. You can open a jupyter notebook (type ``jupyter notebook`` in your terminal), open an interactive python notebook in your terminal (with ``ipython3``), or simply launch python in your terminal (by typing ``python3``).

.. _helloworld:

Hello World
-----------

In this section we will run 'Hello World' example of OpenQL. The first step is to import openql which can be done by:

.. code:: python

    from openql import openql as ql


Next, create a platform by:

.. code:: python

	platform = ql.Platform("myPlatform", config_file_name)

where, ``config_file_name`` is the name of the configuration file in JSON format
which specifies the platform, e.g. ``hardware_config_cc_light.json``. For details, refer to :ref:`platform`.
Note that you can find these files in the `tests` directory of the OpenQL repository; you should copy the
file over from there to wherever you're running Python from (same directory as the Jupyter/IPython notebook,
same directory as where you ran Python from for its terminal, or wherever you placed your Python script).

For this example we will be working on 3 qubits. So let us define a variable so that we can use it at multiple places in our code.

.. code:: python

    nqubits = 3


Create a program

.. code:: python

    p = ql.Program("aProgram", platform, nqubits)

Create a kernel

.. code:: python

    k = ql.Kernel("aKernel", platform, nqubits)

Populate this kernel using default and custom gates

.. code:: python

    for i in range(nqubits):
        k.gate('prepz', [i])

    k.gate('x', [0])
    k.gate('h', [1])
    k.gate('cz', [2, 0])
    k.gate('measure', [0])
    k.gate('measure', [1])

Add the kernel to the program

.. code:: python

    p.add_kernel(k)

Compile the program

.. code:: python

    p.compile()


This will generate the output files in the *test_output* directory.

A good place to get started with with your own programs might be to copy `examples/getting_started.py` to some folder of your choice and start modifying it. For further examples, have a look at the test programs inside the "tests" directory.

.. todo::

    discuss the generated output files


Notebooks
---------

Following Jupyter notebooks are available in the ``<OpenQL Root Dir>/examples/notebooks`` directory:

ccLightClassicalDemo.ipynb
    This notebook provides an introduction to compilation for ccLight with an emphasis on:

    - hybrid quantum/classical code generation
    - control-flow in terms of:
        - if, if-else
        - for loop
        - do-while loop
    - getting measurement results


Examples
--------

Following Jupyter notebooks are available in the ``<OpenQL Root Dir>/examples`` directory:

getting_started.py
    The Hello World example discussed in helloworld_ section.

rb_single.py
    Single qubit randomized benchmarking.


Tests
-----

Various tests are also available in the ``<OpenQL Root Dir>/tests`` directory which can also be used as examples testing various features of OpenQL.

