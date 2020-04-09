.. _qxplatform:

QX Platform
----------

Details of the configuration file for the QX simulator platform. [TBD]

The OpenQL compiler is able to generate a qasm file to interface to QX.
This qasm file generation is controlled by option *write_qasm_files*:

- yes:
  Qasm files are generated before and after most of the passes.

- no:
  No qasm files are generated.

The qasm files are generated in the default output directory.
The qasm file to be used for QX is named by the program name suffixed with .qasm.

Unlike in previous releases, quantumsim is not considered a platform.
It is a means to simulate a particular (hardware) platform without timing.

[TBD]
