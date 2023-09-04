import openql as ql
import os
import unittest

from config import json_dir, output_dir, qasm_golden_dir
from utils import file_compare


class TestCclLongDuration(unittest.TestCase):
    @classmethod
    def setUp(cls):
        ql.initialize()

    def test_all_xy(self):
        """
        Single qubit AllXY sequence.
        Writes output files to the directory specified in openql.
        Output directory is set as an attribute to the program for convenience.

        Input pars:
            qubit_idx:      int specifying the target qubit (starting at 0)
            platf_cfg:      filename of the platform config file
            double_points:  if true repeats every element twice
        Returns:
            p:              OpenQL Program object containing
        """
        ql.set_option('output_dir', output_dir)
        config_fn = os.path.join(json_dir, 'test_cfg_CCL_long_duration.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        program = ql.Program("AllXYLongDuration", platform, platform.get_qubit_number())

        all_xy = [['i', 'i'], ['rx180', 'ry180'], ['ry180', 'rx180']]
        # ,
        # ['rx180', 'ry180'], ['ry180', 'rx180'],
        # ['rx90', 'i'], ['ry90', 'i'], ['rx90', 'ry90'],
        # ['ry90', 'rx90'], ['rx90', 'ry180'], ['ry90', 'rx180'],
        # ['rx180', 'ry90'], ['ry180', 'rx90'], ['rx90', 'rx180'],
        # ['rx180', 'rx90'], ['ry90', 'ry180'], ['ry180', 'ry90'],
        # ['rx180', 'i'], ['ry180', 'i'], ['rx90', 'rx90'],
        # ['ry90', 'ry90']]

        qubit_idx = 0
        for i, xy in enumerate(all_xy):
            k = ql.Kernel("AllXY_" + str(i), platform, platform.get_qubit_number())
            k.prepz(qubit_idx)
            k.gate(xy[0], [qubit_idx])
            k.gate(xy[1], [qubit_idx])
            k.measure(qubit_idx)
            program.add_kernel(k)

        program.compile()

        gold_fn = os.path.join(qasm_golden_dir, program.name + '_last.qasm')
        qisa_fn = os.path.join(output_dir, program.name + '_last.qasm')

        self.assertTrue(file_compare(qisa_fn, gold_fn))


if __name__ == '__main__':
    unittest.main()
