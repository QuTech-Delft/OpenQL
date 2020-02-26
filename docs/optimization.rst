.. _optimization:

Optimization
------------

Optimization of circuits [TBD]

Optimize
^^^^^^^^

attempts to find contigous sequences of quantum gates
that are equivalent to identity (within some small epsilon which currently is 10 to the power -4)
and then take those sequences out of the circuit;
this relies on the function of each gate to be defined in its ``mat`` field as a matrix.

Entry points
%%%%%%%%%%%%

The following entry points are supported:

- ``entry()``
  TBD

Input and output intermediate representation
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

TBD.

Options
%%%%%%%%%

The following options are supported:

- ``option``
  TBD

Function
%%%%%%%%%

TBD

Clifford optimization
^^^^^^^^^^^^^^^^^^^^^

dependency chains of one-qubit clifford gates operating on the same qubit
are replaced by equivalent sequences of primitive gates when the latter leads to a shorter execution time.
Clifford gates are recognized by their name and use is made of the property
that clifford gates form a group of 24 elements.
Clifford optimization is called before and after the mapping pass.


Entry points
%%%%%%%%%%%%

The following entry points are supported:

- ``entry()``
  TBD

Input and output intermediate representation
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

TBD.

Options
%%%%%%%%%

The following options are supported:

- ``option``
  TBD

Function
%%%%%%%%%

TBD

