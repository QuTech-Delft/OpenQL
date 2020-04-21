.. _decomposition:

Decomposition
-------------

Decomposition of gates [TBD]

Control decomposition
^^^^^^^^^^^^^^^^^^^^^


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


Unitary decomposition
^^^^^^^^^^^^^^^^^^^^^
Unitary decomposion allows a developer of quantum algorithms to specify a quantum gate as a unitary matrix, which is then split into a circuit consisting of ``ry``, ``rz`` and ``cnot`` gates. 

To use it, define a ``Unitary`` with a name and a  (complex) list containing all the values in the unitary matrix in order from the top left to the bottom right.  The matrix needs to be unitary to be a valid quantum gate, otherwise an error will be raised by the compilation step.

+--------------+----------------------------+---------------------------+---------------------------------------+
| Name         | operands                   | C++ type                  | example                               |
+==============+============================+===========================+=======================================+
| Unitary      | name                       | string                    | "U_name"                              |
|              +----------------------------+---------------------------+---------------------------------------+
|              | unitary matrix             | vector<complex<double>>   | [0.5+0.5j,0.5-0.5j,0.5-0.5j,0.5+0.5j] |         
+--------------+----------------------------+---------------------------+---------------------------------------+

The unitary is first decomposed, by calling the ``.decompose()`` function on it. Only then can it be added to the kernel as a normal gate to the number of qubits corresponding to the unitary matrix size. This looks like:

.. code::

    u1 = ql.Unitary("U_name", [0.5+0.5j,0.5-0.5j,0.5-0.5j,0.5+0.5j])
    u1.decompose()
    k.gate(u1, [0])

Which generates this circuit:

.. code::

    rz q[0], -1.570796
    ry q[0], -1.570796
    rz q[0], 1.570796

The circuit generated might also have different angles, though not different gates, and result in the same effect on the qubits, this is because a matrix can have multiple valid decompositions. 

For a two-qubit unitary gate or matrix, it looks like:

.. code::

    list_matrix = [1, 0	      , 0       , 0, 
                   0, 0.5+0.5j, 0.5-0.5j, 0,
                   0, 0.5-0.5j, 0.5+0.5j, 0,
                   0, 0       , 0       , 1]
    u1 = ql.Unitary("U_name", list_matrix)
    u1.decompose()
    k.gate(u1, [0,1])

This generates a circuit of 24 gates of which 6 ``cnots``, spanning qubits 0 and 1. The rest are ``ry`` and ``rz`` gates on both qubits, which looks like this:

.. code::

    rz q[0], -0.785398
    ry q[0], -1.570796
    rz q[0], -3.926991
    rz q[1], -0.785398
    cnot q[0],q[1]
    rz q[1], 1.570796
    cnot q[0],q[1]
    rz q[0], 2.356194
    ry q[0], -1.570796
    rz q[0], -3.926991
    ry q[1], 0.785398
    cnot q[0],q[1]
    ry q[1], 0.785398
    cnot q[0],q[1]
    rz q[0], -0.000000
    ry q[0], -1.570796
    rz q[0], 3.926991
    rz q[1], 0.785398
    cnot q[0],q[1]
    rz q[1], -1.570796
    cnot q[0],q[1]
    rz q[0], 3.926991
    ry q[0], -1.570796
    rz q[0], -2.356194

The unitary gate has no limit in how many qubits it can apply to. But the matrix size for an n-qubit gate scales as 2^n*2^n, which means the number of elements in the matrix scales with 4^n. This is also the scaling rate of the execution time of the decomposition algorithm and of the number of gates generated in the circuit. Caution is advised for decomposing large matrices both for compilation time and for the size of the resulting quantum circuit.

More detailed information can be found at http://resolver.tudelft.nl/uuid:9c60d13d-4f42-4d8b-bc23-5de92d7b9600 

..
    Decomposition before scheduling
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^   
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

Decomposition before scheduling
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


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


Decomposition after scheduling
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


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


Decompose_toffoli
^^^^^^^^^^^^^^^^^

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

