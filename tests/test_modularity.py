from openql import openql as ql
import unittest
import os

curdir = os.path.dirname(os.path.realpath(__file__))
output_dir = os.path.join(curdir, 'test_output')

class Test_modularity(unittest.TestCase):

  @classmethod
  def setUp(self):
      ql.initialize()
      ql.set_option('output_dir', output_dir)
      ql.set_option('optimize', 'no')
      ql.set_option('scheduler', 'ASAP')
      ql.set_option('log_level', 'LOG_NOTHING')
      ql.set_option('unique_output', 'no')
      ql.set_option('write_qasm_files', 'no')
      ql.set_option('write_report_files', 'no')
      ql.set_option('mapper', 'minextendrc')

  def test_modularity(self):
      pconfig_fn = os.path.join(curdir, 'hwcfg_cc_light_modular.json')
      cconfig_fn = os.path.join(curdir, 'compiler_modular.json')

      nqubits = 3 

      platform  = ql.Platform('starmon', pconfig_fn)

      c = ql.Compiler("testCompiler", cconfig_fn)
      
      p = ql.Program("testProgram", platform, nqubits, 0)

      k = ql.Kernel("aKernel", platform, nqubits, 0)

      for i in range(nqubits):
          k.gate('prep_z', [i])

      k.gate('x', [0])
      k.gate('x', [0])
      k.gate('x', [0])
      k.gate('h', [1])
      k.gate('cz', [2, 0])
#      k.gate('measure', [0]) Fix measure gate printing from cqasm_reader
      k.gate('measure', [1])

      p.add_kernel(k)

      c.compile(p)

if __name__ == '__main__':
    unittest.main()
