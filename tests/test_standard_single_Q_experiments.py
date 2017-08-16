import os
import filecmp
import unittest
from openql import Kernel, Program
from openql import openql as ql

rootDir = os.path.dirname(os.path.realpath(__file__))

curdir = os.path.dirname(__file__)
config_fn = os.path.join(curdir, 'test_cfg_cbox.json')
platf = ql.Platform("starmon", config_fn)


class Test_single_qubit_seqs(unittest.TestCase):
    @classmethod
    # def setUpClass(self):
    #     self.test_file_dir = join(
    #         pq.__path__[0], 'tests', 'qasm_files')
    #     self.config_fn = join(self.test_file_dir, 'config.json')
    #     self.simple_config_fn = join(self.test_file_dir, 'config_simple.json')
    #     self.qubit_name = 'q0'
    #     self.jump_to_start = ("beq r14, r14, Exp_Start " +
    #                           "\t# Jump to start ad nauseam")
    #     self.times = gen.gen_sweep_pts(start=100e-9, stop=5e-6, step=200e-9)
    #     self.clocks = np.round(self.times/5e-9).astype(int)


    def test_allxy(self):
        p = Program(pname="AllXY", nqubits=1, p=platf)

        allXY = [['i', 'i'], ['rx180', 'rx180'], ['ry180', 'ry180'],
                 ['rx180', 'ry180'], ['ry180', 'rx180'],
                 ['rx90', 'i'], ['ry90', 'i'], ['rx90', 'ry90'],
                 ['ry90', 'rx90'], ['rx90', 'ry180'], ['ry90', 'rx180'],
                 ['rx180', 'ry90'], ['ry180', 'rx90'], ['rx90', 'rx180'],
                 ['rx180', 'rx90'], ['ry90', 'ry180'], ['ry180', 'ry90'],
                 ['rx180', 'i'], ['ry180', 'i'], ['rx90', 'rx90'],
                 ['ry90', 'ry90']]

        for i, xy in enumerate(allXY):
            k = Kernel("allXY"+str(i), p=platf)
            k.prepz(0)
            k.gate(xy[0], 0)
            k.gate(xy[1], 0)
            k.measure(0)
            p.add_kernel(k)

        p.compile()


    # def test_qasm_seq_MotzoiXY(self):
    #     for q_name in ['q0', 'q1']:
    #         qasm_file = sq_qasm.two_elt_MotzoiXY(q_name)
    #         qasm_fn = qasm_file.name
    #         qumis_fn = join(self.test_file_dir,
    #                         "motzoi_{}.qumis".format(q_name))
    #         compiler = qc.QASM_QuMIS_Compiler(self.config_fn,
    #                                           verbosity_level=0)
    #         compiler.compile(qasm_fn, qumis_fn)
    #         asm = Assembler(qumis_fn)
    #         asm.convert_to_instructions()

    #         self.assertEqual(compiler.qumis_instructions[2], 'Exp_Start: ')
    #         self.assertEqual(
    #             compiler.qumis_instructions[-1], self.jump_to_start)

    # def test_qasm_seq_OffOn(self):
    #     for q_name in ['q0', 'q1']:
    #         qasm_file = sq_qasm.off_on(q_name)
    #         qasm_fn = qasm_file.name
    #         qumis_fn = join(self.test_file_dir,
    #                         "off_on_{}.qumis".format(q_name))
    #         compiler = qc.QASM_QuMIS_Compiler(self.config_fn,
    #                                           verbosity_level=0)
    #         compiler.compile(qasm_fn, qumis_fn)
    #         asm = Assembler(qumis_fn)
    #         asm.convert_to_instructions()

    #         self.assertEqual(compiler.qumis_instructions[2], 'Exp_Start: ')
    #         self.assertEqual(
    #             compiler.qumis_instructions[-1], self.jump_to_start)

    # def test_qasm_seq_ramsey(self):
    #     for q_name in ['q0', 'q1']:
    #         qasm_file = sq_qasm.Ramsey(q_name, times=self.times)
    #         qasm_fn = qasm_file.name
    #         qumis_fn = join(self.test_file_dir,
    #                         "Ramsey_{}.qumis".format(q_name))
    #         compiler = qc.QASM_QuMIS_Compiler(self.simple_config_fn,
    #                                           verbosity_level=0)
    #         compiler.compile(qasm_fn, qumis_fn)
    #         asm = Assembler(qumis_fn)
    #         asm.convert_to_instructions()

    #         self.assertEqual(compiler.qumis_instructions[2], 'Exp_Start: ')
    #         self.assertEqual(
    #             compiler.qumis_instructions[-1], self.jump_to_start)

    # def test_qasm_seq_echo(self):
    #     for q_name in ['q0', 'q1']:
    #         qasm_file = sq_qasm.echo(q_name, times=self.times)
    #         qasm_fn = qasm_file.name
    #         qumis_fn = join(self.test_file_dir,
    #                         "echo_{}.qumis".format(q_name))
    #         compiler = qc.QASM_QuMIS_Compiler(self.config_fn,
    #                                           verbosity_level=0)
    #         compiler.compile(qasm_fn, qumis_fn)
    #         asm = Assembler(qumis_fn)
    #         asm.convert_to_instructions()

    #         self.assertEqual(compiler.qumis_instructions[2], 'Exp_Start: ')
    #         self.assertEqual(
    #             compiler.qumis_instructions[-1], self.jump_to_start)

    # def test_qasm_seq_butterfly(self):
    #     for q_name in ['q0', 'q1']:
    #         qasm_file = sq_qasm.butterfly(q_name)
    #         qasm_fn = qasm_file.name
    #         qumis_fn = join(self.test_file_dir,
    #                         "butterfly_{}.qumis".format(q_name))
    #         compiler = qc.QASM_QuMIS_Compiler(self.config_fn,
    #                                           verbosity_level=0)
    #         compiler.compile(qasm_fn, qumis_fn)
    #         asm = Assembler(qumis_fn)
    #         asm.convert_to_instructions()

    #         self.assertEqual(compiler.qumis_instructions[2], 'Exp_Start: ')
    #         self.assertEqual(
    #             compiler.qumis_instructions[-1], self.jump_to_start)

    # def test_qasm_seq_randomized_benchmarking(self):
    #     ncl = [2, 4, 6, 20]
    #     nr_seeds = 10
    #     for q_name in ['q0', 'q1']:
    #         qasm_file = sq_qasm.randomized_benchmarking(q_name, ncl, nr_seeds)
    #         qasm_fn = qasm_file.name
    #         qumis_fn = join(self.test_file_dir,
    #                         "randomized_benchmarking_{}.qumis".format(q_name))
    #         compiler = qc.QASM_QuMIS_Compiler(self.config_fn,
    #                                           verbosity_level=0)
    #         compiler.compile(qasm_fn, qumis_fn)
    #         asm = Assembler(qumis_fn)
    #         asm.convert_to_instructions()

    #         self.assertEqual(compiler.qumis_instructions[2], 'Exp_Start: ')
    #         self.assertEqual(
    #             compiler.qumis_instructions[-1], self.jump_to_start)
