.. _compiler:

Compiler
=========

To compile a program, the user needs to configure a compiler first. Until version 0.8, this program compilation was done using a monolithic hard-coded sequence of compiler passes inside the program itself when ``program.compile()`` function was called. This is the legacy operation mode, which is currently described in the :ref:`Program` documentation page. However, starting with version 0.8.0.dev1, the programer has the ability to configure its own pass sequence using the :ref:`Compiler API <compiler_api>`. 

There are two options on how to configure a compiler. The first and the most straightforward is to define a compiler object giving it as the second parameter the name of a json configuration, similar of how the :ref:`Platform` is defined. The following code line shows an example of a such initialization:

.. code:: python

    ... different other program initializations ...

    c = ql.Compiler("testCompiler", "cc_compiler_cfg.json")
    
    ..... # definition of Platform and Program p .....
    
    c.compile(p)

In the above code, the ``cc_compiler_cfg.json`` compiler configuration file is used. This can be found in the `.\\test' folder within the OpenQL installation directory and has the following structure:

.. code-block:: html
    :linenos:
    
    {
      "CompilerPasses": 
      [
        {
            "passName" : "Writer", 
            "passAlias": "initialqasmwriter"
            "options": 
            [
                {
                    "optionName" : "eqasm_compiler_name",
                    "optionValue": "eqasm_backend_cc"
                },
                ....
            ]
        },
        ...
      ]
    }

Furthermore, an additional option to configure a compiler is to use the ``compiler::add_pass()`` method to manually load compiler passes inside the program itself. To illustrate this interface, consider the following example:

.. code:: python

    from openql import openql as ql

    c = ql.Compiler("testCompiler")

    c.add_pass_alias("Writer", "outputIR") 
    c.add_pass("Reader") 
    c.add_pass("RotationOptimizer")
    c.add_pass("DecomposeToffoli")
    c.add_pass_alias("CliffordOptimize", "clifford_prescheduler")
    c.add_pass("Scheduler")
    c.add_pass_alias("CliffordOptimize", "clifford_postscheduler")
    c.add_pass_alias("Writer","scheduledqasmwriter")

    c.set_pass_option("ALL", "skip", "no");
    c.set_pass_option("Reader", "write_qasm_files", "no")
    c.set_pass_option("RotationOptimizer", "write_qasm_files", "no")
    c.set_pass_option("outputIR", "write_qasm_files", "yes");
    c.set_pass_option("scheduledqasmwriter", "write_qasm_files", "yes");
    c.set_pass_option("ALL", "write_report_files", "no");

    ..... # definition of Platform and Program p .....
    
    c.compile(p)
    

:Note: The code for the platform and the program creation as described earlier (for more information on that, please see :ref:`creating_your_first_program`) has been removed for clarity purposes. 

The example code shows that we can add a pass under its real name, which should be the exact pass name as defined in the compiler (for a complete list available pass names, please consult :ref:`compiler_passes`), or under an alias name to be defined by the OpenQL user. This last name can be any string and should be used to set pass specific options. This options setting is shown last, where current pass option choices represent either the "ALL" target or a given pass name (either its alias or its real name). Curently, only the <write_qasm_files>, <write_report_files>, and <skip> options are implemented for individual passes. The other options should be accessed through the global option settings of the program. 

Finally, to create and use a new compiler pass, the developer would need to implement three steps:

1) Inherit from the AbstractPass class and implement the following function

   .. code:: c

      virtual void runOnProgram(ql::quantum_program *program)
 
 
2) Register the pass by giving it a pass name in 
 
   .. code:: c

      AbstractPass* PassManager::createPass(std::string passName, std::string aliasName)
 
 
3) Add it in a custom compiler configuration using the :ref:`Compiler API <compiler_api>`


Currently, the following passes are available in the compiler class and can be enabled by using the following pass identifiers to map to the existing passes.  
    
+--------------------------+------------------------------------------------------+
| Pass Identifier          | Compiler Pass                                        |
+==========================+======================================================+
| Reader                   | Program Reading (currently cQASMReader)              |
+--------------------------+------------------------------------------------------+
| Writer                   | Qasm Printer                                         |
+--------------------------+------------------------------------------------------+
| RotationOptimizer        | Optimizer                                            |
+--------------------------+------------------------------------------------------+
| DecomposeToffoli         | Decompose Toffoli                                    |
+--------------------------+------------------------------------------------------+
| Scheduler                | Scheduling                                           |
+--------------------------+------------------------------------------------------+
| BackendCompiler          | Composite pass calling either CC or CC-Light passes  |
+--------------------------+------------------------------------------------------+
| ReportStatistics         | Report Statistics                                    |
+--------------------------+------------------------------------------------------+
| CCLPrepCodeGeneration    | CC-Light dependent code generation preparation       |
+--------------------------+------------------------------------------------------+
| CCLDecomposePreSchedule  | Decomposition before scheduling (CC-Light dependent) |
+--------------------------+------------------------------------------------------+
| WriteQuantumSim          | Print QuantumSim program                             |
+--------------------------+------------------------------------------------------+
| CliffordOptimize         | Clifford Optimization                                |
+--------------------------+------------------------------------------------------+
| Map                      | Mapping                                              |
+--------------------------+------------------------------------------------------+
| RCSchedule               | Resource Constraint Scheduling                       |
+--------------------------+------------------------------------------------------+
| LatencyCompensation      | Latency Compensation                                 |
+--------------------------+------------------------------------------------------+
| InsertBufferDelays       | Insert Buffer Delays                                 |
+--------------------------+------------------------------------------------------+
| CCLDecomposePostSchedule | Decomposition before scheduling (CC-Light dependent) |
+--------------------------+------------------------------------------------------+
| QisaCodeGeneration       | QISA generation (CC-Light dependent)                 |
+--------------------------+------------------------------------------------------+

 
