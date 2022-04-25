# File:         test_cc.py
# Purpose:      test the central controller backend
# Based on:     ../test_hybrid.py, ../test_uniform_sched.py

import os
import unittest
import openql as ql
from typing import List, Tuple

rootDir = os.path.dirname(os.path.realpath(__file__))
curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')
config_fn = os.path.join(curdir, 'test_cfg_cc.json')
platform_name = 's-17'
num_qubits = 17
num_cregs = 32
num_bregs = 32
all_qubits = range(0, num_qubits)


class Test_central_controller(unittest.TestCase):

    @classmethod
    def setUp(self):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('log_level', 'LOG_WARNING')

    # NB: based on PycQED::openql_helpers.py
    def _configure_compiler(
            self,
            platform: ql.Platform,
            cqasm_src_filename: str = "",
            extra_pass_options: List[Tuple[str, str]] = None
    ) -> ql.Compiler:
        # NB: for alternative ways to configure the compiler, see
        # https://openql.readthedocs.io/en/latest/gen/reference_configuration.html#compiler-configuration

        c = platform.get_compiler()


        # remove default pass list (this also removes support for most *global* options as defined in
        # https://openql.readthedocs.io/en/latest/gen/reference_options.html, except for 'log_level')
        # NB: this defeats automatic backend selection by OpenQL based on key "eqasm_compiler"
        c.clear_passes()

        # add the passes we need
        compiling_cqasm = cqasm_src_filename != ""
        if compiling_cqasm:
            # cQASM reader as very first step
            c.append_pass(
                'io.cqasm.Read',
                'reader',
                {
                    'cqasm_file': cqasm_src_filename
                }
            )

            # decomposer for legacy decompositions (those defined in the "gate_decomposition" section)
            # FIXME: comment incorrect, also decomposes new-style definitions
            # see https://openql.readthedocs.io/en/latest/gen/reference_passes.html#instruction-decomposer
            c.append_pass(
                'dec.Instructions',
                # NB: don't change the name 'legacy', see:
                # - https://openql.readthedocs.io/en/latest/gen/reference_passes.html#instruction-decomposer
                # - https://openql.readthedocs.io/en/latest/gen/reference_passes.html#predicate-key
                'legacy',
            )
        else:  # FIXME: experimental. Also decompose API input to allow use of new style decompositions
            c.append_pass(
                'dec.Instructions',
                # NB: don't change the name 'legacy', see:
                # - https://openql.readthedocs.io/en/latest/gen/reference_passes.html#instruction-decomposer
                # - https://openql.readthedocs.io/en/latest/gen/reference_passes.html#predicate-key
                'legacy',
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

        # schedule
        c.append_pass(
            'sch.ListSchedule',
            'scheduler',
            {
                'resource_constraints': 'yes'
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
        # c.set_option('*.output_prefix', f'{OqlProgram.output_dir}/%N.%P')
        # if self._arch == 'CC':
        #     c.set_option('cc_backend.output_prefix', f'{OqlProgram.output_dir}/%N')
        c.set_option('scheduler.scheduler_target', 'alap')
        if compiling_cqasm:
            c.set_option('cc_backend.run_once', 'yes')  # if you want to loop, write a cqasm loop

        # finally, set user pass options
        if extra_pass_options is not None:
            for opt, val in extra_pass_options:
                c.set_option(opt, val)

        # log.debug("\n" + c.dump_strategy())
        return c

    def test_gate_decomposition_cz(self):
        platform = ql.Platform(platform_name, os.path.join(curdir, 'config_cc_s17_direct_iq_openql_0_10.json'))

        p = ql.Program('test_gate_decomposition_cz', platform, 17, num_cregs, num_bregs)
        k = ql.Kernel('kernel_0', platform, 17, num_cregs, num_bregs)

        k.gate("cz", [8, 10])
        k.gate("cz", [10, 8])
        k.gate("cz", [8, 11])
        k.gate("cz", [11, 9])
        k.gate("cz", [11, 14])
        k.gate("cz", [14, 11])
        k.gate("cz", [10, 14])
        k.gate("cz", [14, 10])
        k.gate("cz", [9, 11])
        k.gate("cz", [11, 9])
        k.gate("cz", [9, 12])
        k.gate("cz", [12, 9])
        # k.gate("cz", [11, 15])
        # k.gate("cz", [15, 11])
        k.gate("cz", [12, 15])
        k.gate("cz", [15, 12])

        p.add_kernel(k)

        # compile, with new style gate decompositions
        c = self._configure_compiler(platform)
        c.compile(p)

    def test_native_instructions(self):
        platform = ql.Platform(platform_name, os.path.join(curdir, 'config_cc_s17_direct_iq_openql_0_10.json'))

        p = ql.Program('test_native_instructions', platform, 17, num_cregs, num_bregs)
        k = ql.Kernel('kernel_0', platform, 17, num_cregs, num_bregs)

        k.gate("prepz", [0])
        k.gate("i", [0])
        k.gate("rx180", [0])
        k.gate("ry180", [0])
        k.gate("rx90", [0])
        k.gate("ry90", [0])
        k.gate("rxm90", [0])
        k.gate("rym90", [0])

        k.gate("measure", [0])

        p.add_kernel(k)
        p.compile()

    def test_special_instructions(self):
        platform = ql.Platform(platform_name, os.path.join(curdir, 'config_cc_s17_direct_iq_openql_0_10.json'))

        p = ql.Program('test_special_instructions', platform, 17, num_cregs, num_bregs)
        k = ql.Kernel('kernel_0', platform, 17, num_cregs, num_bregs)

        k.gate("spec", [0])
        k.gate("rx12", [0])
        k.gate("square", [0])
        k.gate("rx45", [0])

        k.gate("rx2theta", [0])
        k.gate("rxm2theta", [0])
        k.gate("rx2thetaalpha", [0])
        k.gate("rphi180", [0])
        k.gate("rphi180beta", [0])
        k.gate("rx180beta", [0])
        k.gate("rphi180beta2", [0])
        k.gate("ry90beta", [0])
        k.gate("rym90alpha", [0])
        k.gate("ry90betapi", [0])
        k.gate("rphi180alpha", [0])
        k.gate("rx90alpha", [0])
        k.gate("rx180alpha2", [0])
        k.gate("rphim2theta", [0])

        p.add_kernel(k)
        p.compile()

    def test_gate_decompositions_alias(self):
        platform = ql.Platform(platform_name, os.path.join(curdir, 'config_cc_s17_direct_iq_openql_0_10.json'))

        p = ql.Program('test_gate_decompositions_alias', platform, 17, num_cregs, num_bregs)
        k = ql.Kernel('kernel_0', platform, 17, num_cregs, num_bregs)

        k.gate("x", [0])
        k.gate("y", [0])
        k.gate("roty90", [0])
        k.gate("x180", [0])
        k.gate("y180", [0])
        k.gate("y90", [0])
        k.gate("x90", [0])
        k.gate("my90", [0])
        k.gate("mx90", [0])

        p.add_kernel(k)
        p.compile()

    # Quantum Error Correction cycle
    def test_qec(self):
        platform = ql.Platform(platform_name, config_fn)

        p = ql.Program('test_qec', platform, num_qubits, num_cregs, num_bregs)
        k = ql.Kernel('kernel_0', platform, num_qubits, num_cregs, num_bregs)

        # pipelined QEC: [
        # see: R. Versluis et al., Phys. Rev. A 8, 034021 (2017)
        # - nw, ne, sw, se] -> [n, e, w, s] because we rotate grid
        # - H -> rym90, ry90, see Fig 2 of reference
        #
        # class SurfaceCode, qubits, tiles, width, getNeighbourN, getNeighbourE, getNeighbourW, getNeighbourS, getX, getZ, getData

        # define qubit aliases:
        # FIXME: neighbours make no sense anymore
        x = 7
        xN = x-5
        xE = x+1
        xS = x+5
        xW = x-1

        z = 11
        zN = z-5
        zE = z+1
        zS = z+5
        zW = z-1

        # create classical registers
        if 0:      # FIXME: deprecated by branch condex
            rdX = ql.CReg(1)
            rdZ = ql.CReg(2)

        # X stabilizers
        k.gate("rym90", [x])
        k.gate("rym90", [xN])
        k.gate("rym90", [xE])
        k.gate("rym90", [xW])
        k.gate("rym90", [xS])
        k.barrier([])

        k.gate("cz", [x, xE])
        k.gate("cz", [x, xN])
        k.gate("cz", [x, xS])
        k.gate("cz", [x, xW])
        k.barrier([])

        k.gate("ry90", [x])
        k.gate("ry90", [xN])
        k.gate("ry90", [xE])
        k.gate("ry90", [xW])
        k.gate("ry90", [xS])
        k.barrier([])

#        k.gate("measure", [x], rdX)
        k.gate('measure', [x], 0,0.0, [0])
        k.barrier([])

        # Z stabilizers
        k.gate("rym90", [z])
        k.barrier([])

        k.gate("cz", [z, zE])
        k.gate("cz", [z, zS])
        k.gate("cz", [z, zN])
        k.gate("cz", [z, zW])
        k.barrier([])

        k.gate("ry90", [z])
        # k.gate("measure", [z], rdZ)
        k.gate('measure', [z], 0, 0.0, [1])

        p.add_kernel(k)
        p.compile()

    def test_angle(self):
        platform = ql.Platform(platform_name, config_fn)

        p = ql.Program('test_angle', platform, num_qubits, num_cregs, num_bregs)
        k = ql.Kernel('kernel_0', platform, num_qubits, num_cregs, num_bregs)

        k.gate("rx180", [6], 0, 1.2345)     # NB: Python interface lacks classical parameter

        p.add_kernel(k)
        p.compile()

    def test_qi_example(self):
        platform = ql.Platform(platform_name, os.path.join(curdir, 'config_cc_s17_direct_iq_openql_0_10.json'))

        p = ql.Program('test_qi_example', platform, 17, num_cregs, num_bregs)
        k = ql.Kernel('kernel_0', platform, 17, num_cregs, num_bregs)

        k.barrier([])
        for q in [0, 1, 2, 3, 4]:
            k.gate("prepz", [q])
        k.barrier([])

        k.gate("ry180", [0, 2])     # FIXME: "y" does not work, but gate decomposition should handle?
        # k.gate("cz", [0, 2])
        k.gate("cz", [8, 10])
        k.gate("y90", [2])
        k.barrier([])
        for q in [0, 1, 2, 3, 4]:
            k.gate("measure", [q])
        k.barrier([])

        p.add_kernel(k)

        # compile, with new style gate decompositions
        c = self._configure_compiler(platform)
        c.compile(p)

    # based on ../test_cqasm_reader.py::test_conditions
    # FIXME: uses old cqasm_reader
    def test_cqasm_conditions(self):
        cqasm_config_fn = os.path.join(curdir, 'cqasm_config_cc.json')
        platform = ql.Platform(platform_name, os.path.join(curdir, 'cc_s5_direct_iq.json'))
        number_qubits = platform.get_qubit_number()
        name = 'test_cqasm_conditions'
        program = ql.Program(name, platform, number_qubits)
        qasm_rdr = ql.cQasmReader(platform, program, cqasm_config_fn)
        qasm_str = """
            version 1.1
            
            var qa, qb: qubit
            var ca, cb: bool
            
            { measure_fb qa, ca | measure_fb qb, cb }
            # .barrier  # use subcircuit to force new kernel, and thus scheduling realm 
            # barrier [qa,qb]
            # { barrier qa | barrier qb }
            barrier
            
            # # Old hack for feedback
            # { measure qa, ca | measure qb, cb }
            # 
            # .wait_uhfqa # use subcircuit to force new kernel, and thus scheduling realm
            # { _wait_uhfqa qa | _wait_uhfqa qb }
            # .dist_dsm
            # { _dist_dsm qa, ca | _dist_dsm qb, cb }
            # .wait_dsm
            # { _wait_dsm qa | _wait_dsm qb }        
            # .test
            #             
            cond(true) x qa
            cond(false) y qa
            cond(ca) z qa
            cond(!true) x qa
            cond(!false) y qa
            cond(!ca) z qa
            cond(!!true) x qa
            cond(!!false) y qa
            cond(!!ca) z qa
            cond(ca && cb) x qa
            cond(ca && true) y qa
            cond(ca && false) z qa
            cond(true && cb) x qa
            cond(false && cb) y qa
            cond(ca || cb) z qa
            cond(ca || true) x qa
            cond(ca || false) y qa
            cond(true || cb) z qa
            cond(false || cb) x qa
            cond(ca ^^ cb) y qa
            cond(ca ^^ true) z qa
            cond(ca ^^ false) x qa
            cond(true ^^ cb) y qa
            cond(false ^^ cb) z qa
            cond(!(ca && cb)) x qa
            cond(!(ca && true)) y qa
            cond(!(ca && false)) z qa
            cond(!(true && cb)) x qa
            cond(!(false && cb)) y qa
            cond(!(ca || cb)) z qa
            cond(!(ca || true)) x qa
            cond(!(ca || false)) y qa
            cond(!(true || cb)) z qa
            cond(!(false || cb)) x qa
            cond(!(ca ^^ cb)) y qa
            cond(!(ca ^^ true)) z qa
            cond(!(ca ^^ false)) x qa
            cond(!(true ^^ cb)) y qa
            cond(!(false ^^ cb)) z qa
            """
        qasm_rdr.string2circuit(qasm_str)
        ql.set_option('log_level', 'LOG_INFO')
        program.compile()
        #self.assertTrue(file_compare(os.path.join(output_dir, name + '.qasm'), os.path.join(curdir, 'golden', name + '.qasm')))


    def test_cqasm_gate_decomposition(self):
        cqasm_config_fn = os.path.join(curdir, 'cqasm_config_cc.json')
        platform = ql.Platform(platform_name, os.path.join(curdir, 'cc_s5_direct_iq.json'))
        number_qubits = platform.get_qubit_number()
        name = 'test_cqasm_gate_decomposition'
        program = ql.Program(name, platform, number_qubits)
        qasm_rdr = ql.cQasmReader(platform, program, cqasm_config_fn)
        qasm_str = """
            version 1.1
            
            var qa, qb: qubit
            var ca, cb: bool
            
            { measure_fb qa, ca | measure_fb qb, cb }
            """
        qasm_rdr.string2circuit(qasm_str)
        ql.set_option('log_level', 'LOG_INFO')
        program.compile()

    # based on test_hybrid.py::test_do_while_nested_for()
    @unittest.skip("fails on sf_cz_sw")  # FIXME: now fails with "in repeat-until loop body: expected creg reference, but got something else"
    # additionally, we don't support if_?_break anymore
    def test_nested_rus(self):
        num_qubits = 5
        qidx = 0

        platform = ql.Platform(platform_name, os.path.join(curdir, 'config_cc_s17_direct_iq_openql_0_10.json'))
        p = ql.Program('test_nested_rus', platform, num_qubits, num_cregs, num_bregs)

        outer_program = ql.Program('outer_program', platform, num_qubits, num_cregs, num_bregs)
        outer_kernel = ql.Kernel('outer_kernel', platform, num_qubits, num_cregs)
        outer_kernel.gate("measure_fb", [qidx])
        outer_kernel.gate("if_1_break", [qidx])  # FIXME: uses incorrect label
        outer_program.add_kernel(outer_kernel)

        inner_program = ql.Program('inner_program', platform, num_qubits, num_cregs, num_bregs)
        inner_kernel = ql.Kernel('inner_kernel', platform, num_qubits, num_cregs)
        inner_kernel.gate("measure_fb", [qidx])
        inner_kernel.gate("if_0_break", [qidx])
        inner_kernel.gate("rx180", [qidx])
        inner_program.add_for(inner_kernel, 1000000) # NB: loops *kernel*

        outer_program.add_program(inner_program)

        foo = ql.CReg(0)
        p.add_do_while(outer_program, ql.Operation(foo, '==', foo)) # NB: loops *program*, CC backend interprets all conditions as true

        ql.set_option('log_level', 'LOG_INFO') # override log level
        p.compile()

    # based on DCL test program
    @unittest.skip("fails on sf_cz_sw")
    # FIXME: additionally, we don't support if_?_break anymore
    def test_nested_rus_angle_0(self):
        num_qubits = 17

        ancilla1idx = 10
        ancilla2idx = 8
        dataidx = 11

        angle = 0
        echo_delay_inner_rus = 1
        echo_delay_inner_rus_data = 1
        echo_delay_outer_rus = 1

        platform = ql.Platform(platform_name, os.path.join(curdir, 'config_cc_s17_direct_iq.json'))
        p = ql.Program('test_nested_rus_angle_0', platform, num_qubits, num_cregs, num_bregs)

        init_kernel = ql.Kernel('initKernel', platform, num_qubits, num_cregs)
        init_kernel.prepz(ancilla1idx)
        init_kernel.prepz(ancilla2idx)
        init_kernel.prepz(dataidx)
        init_kernel.gate('cw_{:02}'.format(int(angle) // 20 + 9), [dataidx])
        init_kernel.gate("wait", [], 0)
        p.add_kernel(init_kernel)

        outer_program = ql.Program('outerProgram', platform, num_qubits, num_cregs, num_bregs)
        rus_program_1 = ql.Program('rusProgram1', platform, num_qubits, num_cregs, num_bregs)
        rus_kernel_1 = ql.Kernel('rusKernel1', platform, num_qubits, num_cregs)
        rus_kernel_1.gate("rx2theta", [ancilla1idx])
        rus_kernel_1.gate("rYm90", [ancilla2idx])
        rus_kernel_1.gate("cz", [ancilla1idx, ancilla2idx])
        rus_kernel_1.gate("rXm2theta", [ancilla1idx])
        rus_kernel_1.gate("ry90beta", [ancilla2idx])
        rus_kernel_1.gate("wait", [ancilla1idx,ancilla2idx], 0)
        rus_kernel_1.gate("rphi180", [dataidx])
        rus_kernel_1.gate("measure_fb", [ancilla1idx])
        rus_kernel_1.gate("wait", [ancilla2idx], echo_delay_inner_rus)
        rus_kernel_1.gate("rphi180", [ancilla2idx])
        rus_kernel_1.gate("wait", [ancilla2idx], 1860-echo_delay_inner_rus)
        rus_kernel_1.gate("wait", [dataidx], echo_delay_inner_rus_data)
        rus_kernel_1.gate("wait", [ancilla1idx,ancilla2idx,dataidx], 0)
        rus_kernel_1.gate("if_0_break", [ancilla1idx])
        rus_kernel_1.gate("wait", [ancilla1idx,ancilla2idx,dataidx], 0)
        rus_kernel_1.gate("rx180", [ancilla1idx])
        rus_kernel_1.gate("rxm90", [ancilla2idx])
        rus_kernel_1.gate("rx180", [dataidx])
        rus_kernel_1.gate("wait", [ancilla1idx,ancilla2idx,dataidx], 0)
        rus_program_1.add_for(rus_kernel_1, 1000000)
        outer_program.add_program(rus_program_1)

        interm_program = ql.Program('intermProgram', platform, num_qubits, num_cregs, num_bregs)
        interm_kernel = ql.Kernel('intermKernel', platform, num_qubits, num_cregs)
        interm_kernel.gate("rx180", [ancilla2idx])
        interm_kernel.gate("rx180", [dataidx])
        interm_kernel.gate("rYm90", [dataidx])
        interm_kernel.gate("cz", [ancilla2idx, dataidx])
        interm_kernel.gate("rY90", [dataidx])
        interm_program.add_kernel(interm_kernel)
        outer_program.add_program(interm_program)

        rus_program_2 = ql.Program('rusProgram2', platform, num_qubits, num_cregs, num_bregs)
        rus_kernel_2 = ql.Kernel('rusKernel2', platform, num_qubits, num_cregs)
        rus_kernel_2.gate("rX2theta", [ancilla1idx])
        rus_kernel_2.gate("rYm90alpha", [ancilla2idx])
        rus_kernel_2.gate("cz", [ancilla1idx, ancilla2idx])
        rus_kernel_2.gate("rX2thetaalpha", [ancilla1idx])
        rus_kernel_2.gate("rY90betapi", [ancilla2idx])
        rus_kernel_2.gate("wait", [ancilla1idx,ancilla2idx], 0)
        rus_kernel_2.gate("rphi180beta", [dataidx])
        rus_kernel_2.gate("measure_fb", [ancilla1idx])
        rus_kernel_2.gate("wait", [ancilla2idx], echo_delay_inner_rus)
        rus_kernel_2.gate("rphi180alpha", [ancilla2idx])
        rus_kernel_2.gate("wait", [ancilla2idx], 1860-echo_delay_inner_rus)
        rus_kernel_2.gate("wait", [dataidx], echo_delay_inner_rus_data)
        rus_kernel_2.gate("wait", [ancilla1idx,ancilla2idx,dataidx], 0)
        rus_kernel_2.gate("if_0_break", [ancilla1idx])
        rus_kernel_2.gate("wait", [ancilla1idx,ancilla2idx,dataidx], 0)
        rus_kernel_2.gate("rx180", [ancilla1idx])
        rus_kernel_2.gate("rx90alpha", [ancilla2idx])
        rus_kernel_2.gate("rx180beta", [dataidx])
        rus_kernel_2.gate("wait", [ancilla1idx,ancilla2idx,dataidx], 0)
        rus_program_2.add_for(rus_kernel_2, 1000000)
        outer_program.add_program(rus_program_2)

        end_kernel = ql.Kernel('outerKernel', platform, num_qubits, num_cregs)
        end_kernel.gate("wait", [ancilla2idx,dataidx], 0)
        end_kernel.gate("rx180alpha2", [ancilla2idx])
        end_kernel.gate("measure_fb", [ancilla2idx])
        end_kernel.gate("wait", [dataidx], echo_delay_outer_rus)
        end_kernel.gate("rphi180beta2", [dataidx])
        end_kernel.gate("wait", [dataidx], 1880-echo_delay_outer_rus)
        end_kernel.gate("wait", [ancilla2idx,dataidx], 0)
        end_kernel.gate("if_0_break", [ancilla2idx])
        outer_program.add_kernel(end_kernel)

        foo = ql.CReg(0)
        p.add_do_while(outer_program, ql.Operation(foo, '==', foo))

        measure_kernel = ql.Kernel('measureKernel', platform, num_qubits, num_cregs)
        measure_kernel.gate("measure_fb", [dataidx])
        p.add_kernel(measure_kernel)

        ql.set_option('log_level', 'LOG_INFO') # override log level
        p.compile()

    def test_rc_sched_measure(self):
        platform = ql.Platform(platform_name, os.path.join(curdir, 'cc_s5_direct_iq.json'))

        p = ql.Program('test_rc_sched_measure', platform, 5, num_cregs, num_bregs)
        k = ql.Kernel('kernel_0', platform, 5, num_cregs, num_bregs)

        for q in [0, 1, 2, 3, 4]:
            k.gate("measure", [q])
        k.gate('x', [0])
        for q in [0, 1, 2, 3, 4]:
            k.gate("measure", [q])
        k.gate('x', [1])

        p.add_kernel(k)
        p.compile()

    @unittest.skip("fails with: 'Inconsistency detected in bundle contents: time travel not yet possible in this version'")  # FIXME: solve for real
    def test_rc_sched_measure_asap(self):
        platform = ql.Platform(platform_name, os.path.join(curdir, 'cc_s5_direct_iq.json'))

        p = ql.Program('test_rc_sched_measure_asap', platform, 5, num_cregs, num_bregs)
        k = ql.Kernel('kernel_0', platform, 5, num_cregs, num_bregs)

        k.gate('x', [2])
        for q in [0, 1, 2, 3, 4]:
            k.gate("measure", [q])
        k.gate('x', [0])
        for q in [0, 1, 2, 3, 4]:
            k.gate("measure", [q])
        k.gate('x', [1])

        p.add_kernel(k)
        ql.set_option('scheduler', 'ASAP')
        p.compile()

    def test_rc_sched_measure_barrier(self):
        platform = ql.Platform(platform_name, os.path.join(curdir, 'cc_s5_direct_iq.json'))

        p = ql.Program('test_rc_sched_measure_barrier', platform, 5, num_cregs, num_bregs)
        k = ql.Kernel('kernel_0', platform, 5, num_cregs, num_bregs)

        for q in [0, 1, 2, 3, 4]:
            k.gate("measure", [q])
        k.barrier([])
        k.gate('x', [0])
        k.barrier([])
        for q in [0, 1, 2, 3, 4]:
            k.gate("measure", [q])
        k.barrier([])
        k.gate('x', [1])

        p.add_kernel(k)
        p.compile()

    @unittest.skip("fails with new IR")  # FIXME: solve for real
    def test_rc_sched_cz(self):
        num_qubits = 17

        platform = ql.Platform(platform_name, os.path.join(curdir, 'config_cc_s17_direct_iq.json'))

        p = ql.Program('test_rc_sched_cz', platform, num_qubits, num_cregs, num_bregs)
        k = ql.Kernel('kernel_0', platform, num_qubits, num_cregs, num_bregs)

        # NB: requires resource to manage fluxing
        k.gate("cz", [10, 14])  # no associated park
        k.gate("cz", [9, 11])   # parks 12
        k.gate('x', [10])
        p.add_kernel(k)

        ql.set_option('log_level', 'LOG_DEBUG') # override log level
        p.compile()

    def test_rc_sched_prepz(self):
        num_qubits = 17

        platform = ql.Platform(platform_name, os.path.join(curdir, 'config_cc_s17_direct_iq.json'))

        p = ql.Program('test_rc_sched_prepz', platform, num_qubits, num_cregs, num_bregs)
        k = ql.Kernel('kernel_0', platform, num_qubits, num_cregs, num_bregs)

        k.gate('x', [2])
        k.gate('prepz', [1,2])
        k.gate('x', [1])
        p.add_kernel(k)

        ql.set_option('log_level', 'LOG_DEBUG') # override log level
        p.compile()


    # FIXME: add:
    # - qec_pipelined
    # - long program (RB)



if __name__ == '__main__':
    unittest.main()
