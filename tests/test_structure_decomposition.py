import os
from utils import file_compare
import unittest
from openql import openql as ql

curdir = os.path.dirname(os.path.realpath(__file__))
output_dir = os.path.join(curdir, 'test_output')

class Test_structure_decomposition(unittest.TestCase):

    def run_test_case(self, name):
        old_wd = os.getcwd()
        try:
            os.chdir(curdir)

            in_fn = 'test_' + name + '.cq'
            out_fn = 'test_output/' + name + '_out.cq'
            gold_fn = 'golden/' + name + '_out.cq'

            ql.compile(in_fn)

            self.assertTrue(file_compare(out_fn, gold_fn))

        finally:
            os.chdir(old_wd)

    def test_structure_decomposition_goto(self):
        self.run_test_case('structure_decomposition_goto')

    def test_structure_decomposition_if_else(self):
        self.run_test_case('structure_decomposition_if_else')

    def test_structure_decomposition_foreach(self):
        self.run_test_case('structure_decomposition_foreach')

    def test_structure_decomposition_for(self):
        self.run_test_case('structure_decomposition_for')

    def test_structure_decomposition_while(self):
        self.run_test_case('structure_decomposition_while')

    def test_structure_decomposition_repeat_until(self):
        self.run_test_case('structure_decomposition_repeat_until')

if __name__ == '__main__':
    unittest.main()
