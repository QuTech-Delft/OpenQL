# File:         test_rus_private.py
# Purpose:      test RUS using cQASM1.2
# Based on:     test_structure_decomposition.py

import os
import unittest
#from utils import file_compare

import openql as ql


curdir = os.path.dirname(os.path.realpath(__file__))
output_dir = os.path.join(curdir, 'test_output')


class Test_cQASM(unittest.TestCase):

    def run_test_case(self, name):
        old_wd = os.getcwd()
        try:
            os.chdir(curdir)

            in_fn = 'test_' + name + '.cq'
            out_fn = 'test_output/' + name + '_out.cq'
            gold_fn = 'golden/' + name + '_out.cq'

            ql.initialize()
            # ql.set_option('log_level', 'LOG_INFO')
            ql.set_option('log_level', 'LOG_DEBUG')

            if 0:
                # use compatibility
                # fails in new-to-old IR conversion for CC backend
                ql.set_option('write_qasm_files', 'yes')
                ql.set_option('write_report_files', 'yes')
                ql.compile(in_fn)

            if 0:
                # use compatibility
                # fails in outputting debug info
                ql.compile(
                    in_fn,
                    {
                        # 'cqasm_file': in_fn,
                        'output_prefix': 'test_output/%N.%P',
                        'debug': 'yes'
                    }
                )

            if 1:
                # use pass manager
                pl = ql.Platform("cc", "config_cc_s17_direct_iq_openql_0_10.json")
                c = pl.get_compiler()

                if 1:
                    # insert decomposer for legacy decompositions
                    # See; see https://openql.readthedocs.io/en/latest/gen/reference_passes.html#instruction-decomposer
                    c.prefix_pass(
                        'dec.Instructions',
                        'legacy',  # sets predicate key to use legacy decompositions (FIXME: TBC)
                        {
                            'output_prefix': 'test_output/%N.%P',
                            'debug': 'yes'
                        }
                    )

                # insert cQASM reader (as very first step)
                c.prefix_pass(
                    'io.cqasm.Read',
                    'reader',
                    {
                        'cqasm_file': in_fn,
                        'output_prefix': 'test_output/%N.%P',
                        'debug': 'yes'
                    }
                )



                # set scheduler options
                # sch = c.get_pass('scheduler')
                # sch.set_option('scheduler_target', 'asap')
#               c.set_option('scheduler.debug', 'yes')
#                 c.set_option('scheduler.scheduler_target', 'asap')
#                 c.set_option('scheduler.scheduler_heuristic', 'none')

                c.print_strategy()
                c.compile_with_frontend(pl)


#            self.assertTrue(file_compare(out_fn, gold_fn))

        finally:
            os.chdir(old_wd)

    def _test_rus_elements(self):
        self.run_test_case('rus_elements')

    def test_looping(self):
        self.run_test_case('looping')

    @unittest.skip("private test")
    def test_rus_private(self):
        self.run_test_case('rus_private')

if __name__ == '__main__':
    unittest.main()
