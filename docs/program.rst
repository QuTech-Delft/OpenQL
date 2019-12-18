Program
=======

In the OpenQL programming model, one first creates the platform object and then with it the program object.
After that, one creates kernels with gates and adds these kernels to the program.
Finally, one compiles the program and executes it.
At any time, options can be set and got.

Platform creation takes a name (to use in information messages) and the name of the platform configuration file.
The latter is used to initialize the platform attributes, e.g. to create custom gates.

A program is created by specifying a name, the platform, and the numbers of quantum and classical registers.
The latter defaults to 0.
The name can be used as seed to create output file names and is used in information messages.

The main structural attribute of a program is its vector of kernels.
This vector is in the simplest form initialized by adding kernels one by one to it.
The order of execution is then the order of the kernels in the vector.
But there are also program APIs to create control flow between kernels such as if/then, if/then/else, do/while and for.
These take one or more kernels such as the then-part or the loop-body, add sequential control gates to the involved kernels to represent the desired control flow,
and add the resulting kernels to the vector of kernels of the program;
these APIs also take the particular condition or number of iterations as parameter.

Internally in a kernel's circuit, one never finds a control gate:
the gates of a kernel's circuit are always executed sequentially.
During execution, control flow change can only occur at the end of a circuit, after its last gate;
the latter is the only gate of a circuit which can be a control gate.
When executing a control gate,
control passes on to another kernel, which can be the next one in the vector or a particular named one.

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
