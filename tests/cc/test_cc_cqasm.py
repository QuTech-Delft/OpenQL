# File:         test_cc_cqasm.py
# Purpose:      test CC using cQASM1.2
# Based on:     test_structure_decomposition.py

import os
import unittest
import pathlib
import inspect
from sys import platform
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
            ql.set_option('log_level', 'LOG_INFO')
            # ql.set_option('log_level', 'LOG_DEBUG')
            # ql.set_option('log_level', 'LOG_WARNING')

            if 0:   # extracted from PycQED. FIXME: use this instead of code below (after fixing failing tests)
                pl = ql.Platform("cc", "config_cc_s17_direct_iq_openql_0_10.json")
                c = pl.get_compiler()

                # remove default pass list (this also removes support for most *global* options as defined in
                # https://openql.readthedocs.io/en/latest/gen/reference_options.html, except for 'log_level')
                # NB: this defeats automatic backend selection by OpenQL based on key "eqasm_compiler"
                c.clear_passes()

                # cQASM reader as very first step
                c.append_pass(
                    'io.cqasm.Read',
                    'reader',
                    {
                        'cqasm_file': in_fn
                    }
                )

                # perform legacy decompositions (those defined in the "gate_decomposition" section), see:
                # - https://openql.readthedocs.io/en/latest/gen/reference_passes.html#instruction-decomposer
                # - https://openql.readthedocs.io/en/latest/gen/reference_passes.html#predicate-key
                c.append_pass(
                    'dec.Instructions',
                    'dec_legacy',
                    {
                        'predicate_key': 'name',
                        'predicate_value': 'legacy'
                    }
                )

                # perform new-style decompositions, pre-scheduling
                c.append_pass(
                    'dec.Instructions',
                    'dec_pre_sched',
                    {
                        'predicate_key': 'when',
                        'predicate_value': 'pre-sched'
                    }
                )

                # report the initial qasm
                c.append_pass(
                    'io.cqasm.Report',
                    'initial',
                    {
                        'output_suffix': '.cq',
                        'with_timing': 'no'
                    }
                )

                # add constant propagation pass
                c.append_pass(
                    'opt.ConstProp',
                    'const_prop',
                    {
                        'output_prefix': 'test_output/%N.%P',
                        'debug': 'yes'
                    }
                )

                # add dead code elimination pass
                c.append_pass(
                    'opt.DeadCodeElim',
                    'dead_code_elim',
                    {
                        'output_prefix': 'test_output/%N.%P',
                        'debug': 'yes'
                    }
                )

                # schedule
                c.append_pass(
                    'sch.ListSchedule',
                    'scheduler',
                    {
                        'resource_constraints': 'yes'
                    }
                )

                # perform new-style decompositions, post-scheduling
                c.append_pass(
                    'dec.Instructions',
                    'dec_post_sched',
                    {
                        'predicate_key': 'when',
                        'predicate_value': 'post-sched'
                    }
                )

                # report scheduled qasm
                c.append_pass(
                    'io.cqasm.Report',
                    'scheduled',
                    {
                        'output_suffix': '.cq',
                    }
                )

                # generate code using CC backend
                # NB: OpenQL >= 0.10 no longer has a CC-light backend
                c.append_pass(
                    'arch.cc.gen.VQ1Asm',
                    'cc_backend'
                )

                # set compiler pass options
                c.set_option('*.output_prefix', 'test_output/%N.%P')
                c.set_option('cc_backend.output_prefix', 'test_output/%N')
                c.set_option('scheduler.scheduler_target', 'alap')
                c.set_option('cc_backend.run_once', 'yes')  # if you want to loop, write a cqasm loop

                # compile
                c.compile_with_frontend(pl)

            if 1:
                # use pass manager
                pl = ql.Platform("cc", "config_cc_s17_direct_iq_openql_0_10.json")
                c = pl.get_compiler()

                # insert passes at front (in reverse run order)
                if 1:
                    # insert dead code elimination pass
                    c.prefix_pass(
                        'opt.DeadCodeElim',
                        'dead_code_elim',
                        {
                            'output_prefix': 'test_output/%N.%P',
                            'debug': 'yes'
                        }
                    )

                    # insert constant propagation pass
                    c.prefix_pass(
                        'opt.ConstProp',
                        'const_prop',
                        {
                            'output_prefix': 'test_output/%N.%P',
                            'debug': 'yes'
                        }
                    )


                # insert decomposer for legacy decompositions (FIXME: and new style decompositions)
                # See: see https://openql.readthedocs.io/en/latest/gen/reference_passes.html#instruction-decomposer
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
                        'output_prefix': 'test_output/%N.%P',   # FIXME: %N.%P does not work correctly for reader
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

    def run_test_case_string(self, name: str, src: str):
        pathlib.Path(curdir+"/test_"+name+".cq").write_text(inspect.cleandoc(src))
        self.run_test_case(name)

    def test_rus_elements(self):
        self.run_test_case('rus_elements')

    def test_looping(self):
        self.run_test_case('looping')

    def test_cond_gate(self):
        self.run_test_case('cond_gate')

    @unittest.skipIf(platform == "linux" or platform == "darwin", "Fails on  Ubuntu and Macos, see 'test_rnd_proc.cq'")  # FIXME: actually solve underlying issue
    def test_rnd_proc(self):
        self.run_test_case('rnd_proc')

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
