import openql as ql
import os
import unittest

from config import cq_dir, cq_golden_dir, json_dir, output_dir
from utils import file_compare


class TestStructureDecomposition(unittest.TestCase):
    def run_test_case(self, name):
        old_wd = os.getcwd()
        try:
            os.chdir(json_dir)

            in_fn = os.path.join(cq_dir, 'test_' + name + '.cq')
            out_fn = os.path.join(output_dir, name + '_out.cq')
            gold_fn = os.path.join(cq_golden_dir, name + '_out.cq')

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
