import openql as ql
import os
import tempfile
import unittest


class TestCompilerApi(unittest.TestCase):
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
This pass writes the current program out as a cQASM file, targeting the
given cQASM version. The writer supports cQASM versions 1.0, 1.1, and 1.2,
but note that older cQASM versions do not support everything that OpenQL
supports.

Several options are provided to control how the cQASM file is written.
These are necessary because, even within a particular cQASM version, various
dialects exist, based on instruction set, implicit register definitions,
function definitions, and so on.

Regardless of configuration, the written file assumes that the target cQASM
reader/interpreter supports the instruction- and function set as defined
(or derived from) the platform JSON description. This means that if you want
to target a cQASM reader/interpreter that only supports a subset of this
instruction/function set, or one that supports a different instruction set
entirely, you will have to ensure that all instructions have been decomposed
to the instruction set supported by the target prior to printing the cQASM
file, or write the program such that the unsupported instructions/functions
aren't used in the first place. It is also possible to embed the platform
description into the cQASM file in JSON form via a pragma instruction, but
of course the target cQASM reader/interpreter would then have to support
that instead.

The only instructions that the cQASM writer can print that are not part of
the instruction set as defined in the JSON file are pragmas, barriers, wait,
and skip instructions, but they can be disabled via options.

 - `pragma` instructions are no-op placeholder instructions with no operands
   that are used to convey metadata via annotations within the context of
   a statement. If the `with_metadata` and `with_platform` options are
   disabled, no pragmas will be printed.

 - `barrier` and `wait` instructions are used for the builtin wait
   instruction. If the `with_barriers` option is disabled, they will not be
   printed. If the option is set to `simple`, the printed syntax and
   semantics are:

    - `wait <int>`: wait for all previous instructions to complete, then
      wait `<int>` cycles, where `<int>` may be zero;
    - `barrier q[...]`: wait for all instructions operating on the qubits
      in the single-gate-multiple-qubit list to complete.

   Note that this syntax only supports barriers acting on qubits, and
   doesn't support wait instructions depending on a subset of objects.
   However, it conforms with the default cQASM 1.0 gateset as of libqasm
   0.3.1 (in 0.3 and before, `barrier` did not exist in the default
   gateset). If the option is instead set to `extended`, the syntax is:

    - `wait <int>`: wait for all previous instructions to complete, then
      wait `<int>` >= 1 cycles;
    - `wait <int>, [...]`: wait for all previous instructions operating on
      the given objects to complete, when wait `<int>` >= 1 cycles;
    - `barrier`: wait for all previous instructions to complete;
    - `barrier [...]`: wait for all previous instructions operating on the
      given objects to complete.

   This encompasses all wait instructions possible within OpenQL's IR.
   OpenQL's cQASM reader supports both notations equally.

 - `skip <int>` instructions are printed in addition to the `{}` multiline
   bundle notation to convey scheduling information: all instructions in a
   bundle start in the same cycle, the subsequent bundle or instruction
   starts in the next cycle (regardless of the duration of the instructions
   in the former bundle), and a `skip <int>` instruction may be used in
   place of `<int>` empty bundles, thus skipping `<int>` cycles. `skip`
   instructions and bundles are not printed when the `with_timing` option
   is disabled.

None of the supported cQASM versions support non-scalar variables or
registers, aside from the special-cased main qubit register and
corresponding bit register. Therefore, some tricks are needed.

 - For non-scalar registers that are expected to be implicitly defined by
   the target cQASM reader/interpreter, references are printed as a function
   call, for example `creg(2)` for the integer control register 2.

 - For non-scalar variables (including registers when
   `registers_as_variables` is set), an independent cQASM variable will be
   printed for every element of the non-scalar OpenQL object, using the name
   format `<name>_<major>_[...]_<minor>`. For example, the `creg(2)` example
   above would be printed as `creg_2` if `registers_as_variables` is set.
   Note that this notation obviously only supports literal indices, and also
   note that name conflicts may arise in contrived cases (for example, when
   a scalar variable named `creg_2` was defined in addition to a
   one-dimensional `creg` variable).

Indices start from 0 in both cases.

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
    a tree dump and a cQASM file before and after, the latter of which includes
    statistics as comments. The filename is built using the output_prefix option,
    using suffix `_debug_[in|out].ir` for the IR dump, and `_debug_[in|out].cq` for
    the cQASM file. The option values `stats`, `cqasm`, and `both` are used for
    backward compatibility with the `write_qasm_files` and `write_report_files`
    global options; for `stats` and `both` a statistics report file is written with
    suffix `_[in|out].report`, and for `qasm` and `both` a cQASM file is written
    (without stats in the comments) with suffix `_[in|out].qasm`.

  * `output_suffix` *
    Must be any string, default `.cq`. Suffix to use for the output filename.

  * `cqasm_version` *
    Must be one of `1.0`, `1.1`, or `1.2`, default `1.2`. The cQASM version to
    target.

  * `with_platform` *
    Must be `yes` or `no`, default `no`. Whether to include an annotation that
    includes the (preprocessed) JSON description of the platform.

  * `registers_as_variables` *
    Must be `yes` or `no`, default `no`. Whether to include variable declarations
    for registers. This must be enabled if the cQASM file is to be passed to a
    target that doesn't implicitly define the registers. Note that the size of the
    main qubit register is always printed for version 1.0, because it can't legally
    be omitted for that version. Also note that this is a lossy operation if the
    file is later read by OpenQL again, because register indices are lost (since
    only scalar variables are supported by cQASM).

  * `with_statistics` *
    Must be `yes` or `no`, default `no`. Whether to include the current statistics
    for each kernel and the complete program in the generated comments.

  * `with_metadata` *
    Must be `yes` or `no`, default `yes`. Whether to include metadata supported by
    the IR but not by cQASM as annotations, to allow the IR to be more accurately
    reproduced when read again via the cQASM reader pass.

  * `with_barriers` *
    Must be one of `no`, `simple`, or `extended`, default `extended`. Whether to
    include wait and barrier instructions, and if so, using which syntax (see pass
    description). These are only needed when the program will be fed to another
    compiler later on.

  * `with_timing` *
    Must be `yes` or `no`, default `yes`. Whether to include scheduling/timing
    information via bundle-and-skip notation.
""".strip())
        self.assertEqual(p.dump_options(False).strip(), """
output_prefix: %N.%P
debug: no
output_suffix: .cq
cqasm_version: 1.2
with_platform: no
registers_as_variables: no
with_statistics: no
with_metadata: yes
with_barriers: extended
with_timing: yes
""".strip())
        self.assertEqual(p.dump_options(True).strip(), 'no options to dump')
        with self.assertRaisesRegex(RuntimeError, 'unknown option: does not exist'):
            p.get_option('does not exist')
        self.assertEqual(c.get_option('io_cqasm_report.debug'), 'no')
        with self.assertRaisesRegex(RuntimeError, 'unknown option: does not exist'):
            c.get_option('io_cqasm_report.does not exist')
        with self.assertRaisesRegex(RuntimeError, 'unknown option: does not exist'):
            c.get_option('does not exist')
        with self.assertRaisesRegex(RuntimeError, 'no sub-pass with name "does not exist" in root'):
            c.get_option('does not exist.debug')
        p = c.prefix_pass('io.cqasm.Report')
        self.assertEqual(p.get_name(), 'io_cqasm_report_1')
        p = c.insert_pass_after('io_cqasm_report_1', 'io.cqasm.Report')
        self.assertEqual(p.get_name(), 'io_cqasm_report_2')
        p = c.insert_pass_before('io_cqasm_report_2', 'io.cqasm.Read', 'bla', {'debug': 'yes'})
        self.assertEqual(p.get_name(), 'bla')
        with self.assertRaisesRegex(RuntimeError, 'duplicate pass name "bla"'):
            c.append_pass('io.cqasm.Report', 'bla')
        self.assertEqual(p.dump_options(True).strip(), 'debug: yes')
        self.assertTrue(c.does_pass_exist('bla'))
        p = c.get_pass('bla')
        self.assertEqual(p.get_name(), 'bla')
        self.assertEqual(p.get_option('debug'), 'yes')
        self.assertFalse(c.does_pass_exist('fgsfds'))
        with self.assertRaisesRegex(RuntimeError, 'no pass with name "fgsfds" exists'):
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
        with self.assertRaisesRegex(RuntimeError, 'pattern fgdsfds did not match any sub-passes of root'):
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
        with self.assertRaisesRegex(RuntimeError, 'pass with name "bla" not found'):
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
            file_name = os.path.join(d, 'test.json')
            with open(file_name, 'w') as f:
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
            self.assertEqual(ql.Platform('compiler', file_name).get_compiler().dump_strategy().strip(), """
- io_cqasm_report: io.cqasm.Report
   |- no options to dump
""".strip())


if __name__ == '__main__':
    # ql.set_option('log_level', 'LOG_DEBUG')
    unittest.main()
