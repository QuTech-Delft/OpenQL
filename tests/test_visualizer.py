from openql import openql as ql
import unittest
import os

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')

class Test_visualizer(unittest.TestCase):

  @classmethod
  def setUpClass(self):
      ql.set_option('output_dir', output_dir)
      ql.set_option('optimize', 'no')
      ql.set_option('scheduler', 'ASAP')
      ql.set_option('log_level', 'LOG_INFO')
      ql.set_option('unique_output', 'yes')
      ql.set_option('write_qasm_files', 'no')
      ql.set_option('write_report_files', 'no')


  def test_modularity(self):
      config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')

      c = ql.Compiler("testCompiler")

    #NOTE: there is also a Reader pass in OpenQL; however, this is disabled for regression tests because this requires an external qasm file to read in. Currently, limited support in setting this file, only possible now is to give program name the name of an existing qasm file in test_output/ folder. TODO: this should be more flexible by implementing a pass input file option.
    #c.add_pass("Reader");

#NOTE: Below pass sequence emulates the same behavior as the current 0.8.0 release. Future work will split the Backend into smaller passes 
      c.add_pass("Writer");
      c.add_pass("RotationOptimizer");
      c.add_pass("DecomposeToffoli");
      c.add_pass("Scheduler");
      #c.add_pass("ReportStatistics");
      #c.add_pass("Writer");
      c.add_pass("BackendCompiler");
      c.add_pass("Visualizer");
      #c.add_pass("Writer");

#TODO: The backend compiler will eventually be split into smaller backend passes that should be instantiated here in the same way.
      c.set_pass_option("BackendCompiler", "eqasm_compiler_name", "cc_light_compiler"); 

#TODO: currently, all passes require the platform, that is why the options is set for ALL passes. However, this should change and be set only for backend passes
      c.set_pass_option("ALL", "skip", "no");
      c.set_pass_option("ALL", "write_qasm_files", "yes");
      c.set_pass_option("ALL", "write_report_files", "yes");
      #c.set_pass_option("ALL", "hwconfig", config_fn);

#TODO: this needs to be removed! and set as option for a simulator backend or read from platform configuration file
      nqubits = 3 

      platform  = ql.Platform('starmon', config_fn)

      p = ql.Program("testProgram", platform, nqubits, 0)

      k = ql.Kernel("aKernel", platform, nqubits, 0)

#TODO: this should become an initialization of the platform, an implicit pass that initializes the data segment!
      for i in range(nqubits):
          k.gate('prepz', [i])

      k.gate('x', [0])
      k.gate('x', [0])
      k.gate('x', [0])
      k.gate('h', [1])
      k.gate('cz', [2, 0])
      k.gate('measure', [0])
      k.gate('measure', [1])

      p.add_kernel(k)

      c.compile(p)

if __name__ == '__main__':
    unittest.main()
