.. _program:

Program
=======

In the OpenQL programming model, one first creates the platform object and then with it the program object.
After that, one creates kernels with gates and adds these kernels to the program.
Finally, one compiles the program and executes it.
At any time, options can be set and got.

We saw an example of this in :ref:`creating_your_first_program`.
Here it is again but then with everything glued together:

.. code:: python

    from openql import openql as ql

    platform = ql.Platform("myPlatform", "hardware_config_cc_light.json")

    nqubits = 3

    p = ql.Program("aProgram", platform, nqubits)

    k = ql.Kernel("aKernel", platform, nqubits)

    for i in range(nqubits):
        k.gate('prepz', [i])

    k.gate('x', [0])
    k.gate('h', [1])
    k.gate('cz', [2, 0])
    k.gate('measure', [0])
    k.gate('measure', [1])

    p.add_kernel(k)

    p.compile()


Platform creation takes a name (to use in information messages) and the name of the platform configuration file.
The latter is used to initialize the platform attributes, e.g. to create custom gates.

A program is created by specifying a name, the platform, and the numbers of quantum and classical registers.
The name can be used as seed to create output file names and is used in information messages.

The main structural attribute of a program is its vector of kernels.
This vector is in the simplest form initialized by adding kernels one by one to it.
The order of execution is then the order of the kernels in the vector.
But there are also program APIs to create control flow between kernels
such as if/then, if/then/else, do/while and for.
These take one or more kernels representing the then-part, the else-part, or the loop-body,
and add special kernels around them to represent the control flow.
These latter APIs also take the particular branch condition
or the number of iterations as parameter.
See :ref:`kernel` for an overview of these APIs and see :ref:`classical_gate_attributes_in_the_internal_representation` for a definition of the control flow internal representation.

The gates of a kernel's circuit are always executed sequentially.
At the end of a circuit, control passes on to the next kernel in the program's vector of kernels.

After having completed adding kernels, the program has been completely specified.
It is represented by a vector of kernels, each with a circuit.
And in this form, the program is compiled by invoking its ``p.compile()`` method.

In the ``p.compile()`` method,
the platform independent compiler passes and then the platform dependent compiler passes
are called one by one in the order specified by the OpenQL compiler's internals.
After compilation, the ``p.compile()`` method returns, with the internal representation still available.
Compilation will have resulted in the creation of several external representations,
to be used by e.g. simulation, assembly/execution or human inspection.

[API TBD]
