# File:         test_cc_cqasm.py
# Purpose:      test CC using cQASM1.2
# Based on:     test_structure_decomposition.py

import inspect
import openql as ql
import os
import pathlib
import unittest

from config import cq_dir, json_dir, output_dir


class TestCqasm(unittest.TestCase):
    @staticmethod
    def run_test_case(name):
        old_wd = os.getcwd()
        try:
            cur_dir = os.path.dirname(os.path.realpath(__file__))
            os.chdir(cur_dir)

            in_fn = os.path.join(cq_dir, 'test_' + name + '.cq')

            ql.initialize()
            ql.set_option('log_level', 'LOG_INFO')
            # ql.set_option('log_level', 'LOG_DEBUG')
            # ql.set_option('log_level', 'LOG_WARNING')

            if 1:
                # use pass manager
                pl = ql.Platform("cc", os.path.join(json_dir, "config_cc_s17_direct_iq_openql_0_10.json"))
                c = pl.get_compiler()

                # insert passes at front (in reverse run order)
                if 1:
                    # insert dead code elimination pass
                    c.prefix_pass(
                        'opt.DeadCodeElim',
                        'dead_code_elim',
                        {
                            'output_prefix': output_dir + '/%N.%P',
                            'debug': 'yes'
                        }
                    )

                    # insert constant propagation pass
                    c.prefix_pass(
                        'opt.ConstProp',
                        'const_prop',
                        {
                            'output_prefix': output_dir + '/%N.%P',
                            'debug': 'yes'
                        }
                    )

                # insert decomposer for legacy decompositions (FIXME: and new style decompositions)
                # See: see https://openql.readthedocs.io/en/latest/gen/reference_passes.html#instruction-decomposer
                c.prefix_pass(
                    'dec.Instructions',
                    'legacy',  # sets predicate key to use legacy decompositions (FIXME: TBC)
                    {
                        'output_prefix': output_dir + '/%N.%P',
                        'debug': 'yes'
                    }
                )

                # insert cQASM reader (as very first step)
                c.prefix_pass(
                    'io.cqasm.Read',
                    'reader',
                    {
                        'cqasm_file': in_fn,
                        'output_prefix': output_dir + '/%N.%P',
                        'debug': 'yes'
                    }
                )

                # set scheduler options
                # sch = c.get_pass('scheduler')
                # sch.set_option('scheduler_target', 'asap')
                # c.set_option('scheduler.debug', 'yes')
                # c.set_option('scheduler.scheduler_target', 'asap')
                # c.set_option('scheduler.scheduler_heuristic', 'none')

                c.print_strategy()
                c.compile_with_frontend(pl)

                # self.assertTrue(file_compare(out_fn, gold_fn))

        finally:
            os.chdir(old_wd)

    def run_test_case_string(self, name: str, src: str):
        pathlib.Path(os.path.join(cq_dir, "test_" + name + ".cq")).write_text(inspect.cleandoc(src))
        self.run_test_case(name)

    def test_rus_elements(self):
        self.run_test_case('rus_elements')

    def test_looping(self):
        self.run_test_case('looping')

    def test_cond_gate(self):
        self.run_test_case('cond_gate')

    def test_const_prop(self):
        self.run_test_case('const_prop')

    # def test_from_string(self):
    #     pl = ql.Platform("cc", "config_cc_s17_direct_iq_openql_0_10.json")
    #     c = pl.get_compiler()
    #     # FIXME

    @unittest.skip("private test")
    def test_rus_private(self):
        self.run_test_case('rus_private')


if __name__ == '__main__':
    unittest.main()
