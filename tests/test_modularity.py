from openql import openql as ql
import unittest
import os

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')

class Test_modularity(unittest.TestCase):

  @classmethod
  def setUpClass(self):
      ql.set_option('output_dir', output_dir)
      ql.set_option('optimize', 'no')
      ql.set_option('scheduler', 'ASAP')
      ql.set_option('log_level', 'LOG_DEBUG')
      ql.set_option('unique_output', 'no')
      ql.set_option('write_qasm_files', 'no')
      ql.set_option('write_report_files', 'no')
      ql.set_option('mapper', 'minextendrc')

  def test_modularity(self):
      self.setUpClass()
      config_fn = os.path.join(curdir, 'hwcfg_cc_light_modular.json')

      c = ql.Compiler("testCompiler")

      c.add_pass_alias("Writer", "outputIR") 
      c.add_pass("Reader") 
      c.add_pass("RotationOptimizer")
      c.add_pass("DecomposeToffoli")
      c.add_pass_alias("CliffordOptimize", "clifford_prescheduler")
      c.add_pass("Scheduler")
#      c.add_pass_alias("Writer", "outputIR") 
#      c.add_pass("Reader") #TODO: reader cannot read scheduled IR!
      c.add_pass_alias("CliffordOptimize", "clifford_postscheduler")
      c.add_pass_alias("Writer","scheduledqasmwriter")

## From here CC-light backed passes start; not called though to show how only front-end can be used
      #c.add_pass("CCLPrepCodeGeneration") #CCLPrepCodeGeneration
      #c.add_pass("CCLDecomposePreSchedule") #CCLDecomposePreSchedule
      #c.add_pass_alias("WriteQuantumSim", "write_quantumsim_script_unmapped") #WriteQuantumSimPass
      #c.add_pass_alias("CliffordOptimize", "clifford_premapper") #CliffordOptimizePass
      #c.add_pass("Map") #MapPass
      #c.add_pass_alias("CliffordOptimize", "clifford_postmapper") #CliffordOptimizePass
      #c.add_pass("RCSchedule") #RCSchedulePass
      #c.add_pass("LatencyCompensation") #LatencyCompensationPass
      #c.add_pass("InsertBufferDelays") #InsertBufferDelaysPass
      #c.add_pass("CCLDecomposePostSchedule") #CCLDecomposePostSchedulePass
      #c.add_pass_alias("WriteQuantumSim", "write_quantumsim_script_mapped") #WriteQuantumSimPass
      #c.add_pass("QisaCodeGeneration") # QisaCodeGenerationPass

      c.set_pass_option("ALL", "skip", "no");
      c.set_pass_option("Reader", "write_qasm_files", "no")
      c.set_pass_option("RotationOptimizer", "write_qasm_files", "no")
      c.set_pass_option("outputIR", "write_qasm_files", "yes");
      c.set_pass_option("scheduledqasmwriter", "write_qasm_files", "yes");
      c.set_pass_option("ALL", "write_report_files", "no");

      nqubits = 3 

      platform  = ql.Platform('starmon', config_fn)

      p = ql.Program("testProgram", platform, nqubits, 0)

      k = ql.Kernel("aKernel", platform, nqubits, 0)

      for i in range(nqubits):
          k.gate('prep_z', [i])

      k.gate('x', [0])
      k.gate('x', [0])
      k.gate('x', [0])
      k.gate('h', [1])
      k.gate('cz', [2, 0])
      #k.gate('measure', [0]) Fix measure gate printing from cqasm_reader
      #k.gate('measure', [1])

      p.add_kernel(k)

      c.compile(p)

if __name__ == '__main__':
    unittest.main()
