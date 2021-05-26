import openql as ql
import os
import unittest
from utils import file_compare
import tempfile

curdir = os.path.dirname(os.path.realpath(__file__))
output_dir = os.path.join(curdir, 'test_output')

class test_compiler_api(unittest.TestCase):

    def setUp(self):
        ql.initialize()
        self.maxDiff = None

    def test_compiler_api(self):
        c = ql.Compiler()
        p = c.append_pass('io.cqasm.Report')
        self.assertEqual(p.get_name(), 'io_cqasm_report')
        self.assertEqual(p.get_type(), 'io.cqasm.Report')
        self.assertEqual(p.get_option('debug'), 'no')
        self.assertEqual(p.dump_pass_documentation().strip(), """
This pass writes the current program out as a cQASM file.

* Options *

  * `output_prefix` *
    Must be any string, default `%N.%P`. Format string for the prefix used for all
    output products. `%n` is substituted with the user-specified name of the
    program. `%N` is substituted with the optionally uniquified name of the program.
    `%p` is substituted with the local name of the pass within its group. `%P` is
    substituted with the fully-qualified name of the pass, using periods as
    hierarchy separators (guaranteed unique). `%U` is substituted with the
    fully-qualified name of the pass, using underscores as hierarchy separators.
    This may not be completely unique,`%D` is substituted with the fully-qualified
    name of the pass, using slashes as hierarchy separators. Any directories that
    don't exist will be created as soon as an output file is written.

  * `debug` *
    Must be one of `no`, `yes`, `stats`, `qasm`, or `both`, default `no`. May be
    used to implicitly surround this pass with cQASM/report file output printers, to
    aid in debugging. Set to `no` to disable this functionality or to `yes` to write
    a cQASM file before and after that includes statistics as comments. The filename
    is built using the output_prefix option, using suffix `_debug_[in|out].cq`. The
    option values `stats`, `cqasm`, and `both` are used for backward compatibility
    with the `write_qasm_files` and `write_report_files` global options; for `stats`
    and `both` a statistics report file is written with suffix `_[in|out].report`,
    and for `qasm` and `both` a cQASM file is written (without stats in the
    comments) with suffix `_[in|out].qasm`.

  * `output_suffix` *
    Must be any string, default `.cq`. Suffix to use for the output filename.

  * `with_statistics` *
    Must be `yes` or `no`, default `no`. Whether to include the current statistics
    for each kernel and the complete program in the generated comments.
""".strip())
        self.assertEqual(p.dump_options(False).strip(), """
output_prefix: %N.%P
debug: no
output_suffix: .cq
with_statistics: no
""".strip())
        self.assertEqual(p.dump_options(True).strip(), 'no options to dump')
        with self.assertRaisesRegex(RuntimeError, '^unknown option: does not exist'):
            p.get_option('does not exist')
        self.assertEqual(c.get_option('io_cqasm_report.debug'), 'no')
        with self.assertRaisesRegex(RuntimeError, '^unknown option: does not exist'):
            c.get_option('io_cqasm_report.does not exist')
        with self.assertRaisesRegex(RuntimeError, '^unknown option: does not exist'):
            c.get_option('does not exist')
        with self.assertRaisesRegex(RuntimeError, '^no sub-pass with name "does not exist" in root'):
            c.get_option('does not exist.debug')
        p = c.prefix_pass('io.cqasm.Report')
        self.assertEqual(p.get_name(), 'io_cqasm_report_1')
        p = c.insert_pass_after('io_cqasm_report_1', 'io.cqasm.Report')
        self.assertEqual(p.get_name(), 'io_cqasm_report_2')
        p = c.insert_pass_before('io_cqasm_report_2', 'io.cqasm.Read', 'bla', {'debug': 'yes'})
        self.assertEqual(p.get_name(), 'bla')
        with self.assertRaisesRegex(RuntimeError, '^duplicate pass name "bla"'):
            c.append_pass('io.cqasm.Report', 'bla')
        self.assertEqual(p.dump_options(True).strip(), 'debug: yes')
        self.assertTrue(c.does_pass_exist('bla'))
        p = c.get_pass('bla')
        self.assertEqual(p.get_name(), 'bla')
        self.assertEqual(p.get_option('debug'), 'yes')
        self.assertFalse(c.does_pass_exist('fgsfds'))
        with self.assertRaisesRegex(RuntimeError, '^no pass with name "fgsfds" exists'):
            c.get_pass('fgsfds')
        self.assertEqual(
            list(map(ql.Pass.get_name, c.get_passes())),
            ['io_cqasm_report_1', 'bla', 'io_cqasm_report_2', 'io_cqasm_report']
        )
        self.assertEqual(
            list(map(ql.Pass.get_name, c.get_passes_by_type('io.cqasm.Report'))),
            ['io_cqasm_report_1', 'io_cqasm_report_2', 'io_cqasm_report']
        )
        self.assertEqual(c.dump_strategy().strip(), """
- io_cqasm_report_1: io.cqasm.Report
   |- no options to dump

- bla: io.cqasm.Read
   |- debug: yes

- io_cqasm_report_2: io.cqasm.Report
   |- no options to dump

- io_cqasm_report: io.cqasm.Report
   |- no options to dump
""".strip())
        self.assertEqual(c.set_option('io_cqasm_report_*.debug', 'no'), 2)
        with self.assertRaisesRegex(RuntimeError, '^pattern fgdsfds did not match any sub-passes of root'):
            c.set_option('fgdsfds.debug', 'no')
        self.assertEqual(c.set_option('fgdsfds.debug', 'no', False), 0)
        self.assertEqual(c.set_option('fgdsfds.does-not-exist', 'no', False), 0)
        self.assertEqual(c.dump_strategy().strip(), """
- io_cqasm_report_1: io.cqasm.Report
   |- debug: no

- bla: io.cqasm.Read
   |- debug: yes

- io_cqasm_report_2: io.cqasm.Report
   |- debug: no

- io_cqasm_report: io.cqasm.Report
   |- no options to dump
""".strip())
        self.assertEqual(c.get_num_passes(), 4)
        c.remove_pass('bla')
        self.assertEqual(c.get_num_passes(), 3)
        with self.assertRaisesRegex(RuntimeError, '^pass with name "bla" not found'):
            c.remove_pass('bla')
        c.clear_passes()
        self.assertEqual(c.get_num_passes(), 0)

    def test_compiler_json_1(self):
        with tempfile.TemporaryDirectory() as d:
            fname = os.path.join(d, 'test.json')
            with open(fname, 'w') as f:
                f.write('{"passes":[]}')
            self.assertEqual(ql.Compiler('compiler', fname).dump_strategy().strip(), '')

    def test_compiler_json_2(self):
        with tempfile.TemporaryDirectory() as d:
            fname = os.path.join(d, 'test.json')
            with open(fname, 'w') as f:
                f.write("""
                {
                    "pass-options": {
                        "debug": "yes"
                    },
                    "passes": [
                        {
                            "type": "io.cqasm.Report"
                        },
                        {
                            "type": "io.cqasm.Report",
                            "name": "hello",
                            "options": {
                                "debug": "no"
                            }
                        }
                    ]
                }
                """)
            self.assertEqual(ql.Compiler('compiler', fname).dump_strategy().strip(), """
- io_cqasm_report: io.cqasm.Report
   |- debug: yes

- hello: io.cqasm.Report
   |- debug: no
""".strip())

    def test_compiler_json_3(self):
        with tempfile.TemporaryDirectory() as d:
            fname = os.path.join(d, 'test.json')
            with open(fname, 'w') as f:
                f.write("""
                {
                    "architecture": "cc",
                    "passes": [
                        {
                            "type": "gen.VQ1Asm"
                        }
                    ]
                }
                """)
            self.assertEqual(ql.Compiler('compiler', fname).dump_strategy().strip(), """
- gen_vq1_asm: arch.cc.gen.VQ1Asm
   |- no options to dump
""".strip())

    def test_compiler_json_4(self):
        with tempfile.TemporaryDirectory() as d:
            with open(os.path.join(d, 'compiler.json'), 'w') as f:
                f.write("""
                {
                    "architecture": "none",
                    "passes": [
                        {
                            "type": "io.cqasm.Report"
                        }
                    ]
                }
                """)
            fname = os.path.join(d, 'test.json')
            with open(fname, 'w') as f:
                f.write("""
                {
                    "eqasm_compiler": "compiler.json",
                    "hardware_settings": {
                        "qubit_number": 7,
                        "cycle_time": 20
                    },
                    "instructions": {}
                }
                """)
            self.assertEqual(ql.Platform('compiler', fname).get_compiler().dump_strategy().strip(), """
- io_cqasm_report: io.cqasm.Report
   |- no options to dump
""".strip())

    def test_compiler_json_5(self):
        with tempfile.TemporaryDirectory() as d:
            compiler_fname = os.path.join(d, 'compiler.json')
            with open(compiler_fname, 'w') as f:
                f.write("""
                {
                    "architecture": "none",
                    "passes": [
                        {
                            "type": "io.cqasm.Report"
                        }
                    ]
                }
                """)
            fname = os.path.join(d, 'test.json')
            with open(fname, 'w') as f:
                f.write("""
                {
                    "hardware_settings": {
                        "qubit_number": 7,
                        "cycle_time": 20
                    },
                    "instructions": {}
                }
                """)
            self.assertEqual(ql.Platform('compiler', fname, compiler_fname).get_compiler().dump_strategy().strip(), """
- io_cqasm_report: io.cqasm.Report
   |- no options to dump
""".strip())

    def test_compiler_json_6(self):
        with tempfile.TemporaryDirectory() as d:
            fname = os.path.join(d, 'test.json')
            with open(fname, 'w') as f:
                f.write("""
                {
                    "eqasm_compiler": {
                        "architecture": "none",
                        "passes": [
                            {
                                "type": "io.cqasm.Report"
                            }
                        ]
                    },
                    "hardware_settings": {
                        "qubit_number": 7,
                        "cycle_time": 20
                    },
                    "instructions": {}
                }
                """)
            self.assertEqual(ql.Platform('compiler', fname).get_compiler().dump_strategy().strip(), """
- io_cqasm_report: io.cqasm.Report
   |- no options to dump
""".strip())


if __name__ == '__main__':
    # ql.set_option('log_level', 'LOG_DEBUG')
    unittest.main()

