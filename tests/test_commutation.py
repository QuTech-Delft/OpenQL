import os
from utils import file_compare
import unittest
from openql import openql as ql

curdir = os.path.dirname(os.path.realpath(__file__))
output_dir = os.path.join(curdir, 'test_output')

# s5 would do as well
# cnot, t, z, etc. must not have been decomposed to exploit their commute properties
conffile = 'test_mapper_s7.json'

class Test_commutation(unittest.TestCase):

    def setUp(self):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('log_level', 'LOG_WARNING')

        # ASAP is easier to construct and verify the tests
        ql.set_option('scheduler', 'ASAP')

        # don't optimize since gates are used by the tests to create 'latency'
        # to enforce schedules that make a difference with/without commutation
        ql.set_option('clifford_prescheduler', 'no')
        ql.set_option('clifford_postscheduler', 'no')
        ql.set_option('clifford_premapper', 'no')
        ql.set_option('clifford_postmapper', 'no')

    def test_cnot_z_NN_commute(self):
        config_fn = os.path.join(curdir, conffile)
        platf = ql.Platform("starmon", config_fn)

        ql.set_option("scheduler_commute", 'no');
        ql.set_option("scheduler_commute_rotations", 'no');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        # commute on z operand of CNOT/T/Z/...
        # gates on q0 commute with gates on q3 and could even be in parallel
        # q3 is cnot control and thus z-commuting operand i.e. with t q3 and z q3
        # q0 is cnot target and thus x-commuting operand, of which there are none
        # so: cnot commutes with later t q3 and z q3, making t q3 and z q3 available in parallel to q0 gates,
        # while cnot has to be after t q0 and z q0, so intended result is that t q3 and z q3 get before cnot
        #
        # both commutation options are 'no' so input order must be kept
        k.gate("t", [0]);
        k.gate("z", [0]);
        k.gate("cnot", [3,0]);
        k.gate("t", [3]);
        k.gate("z", [3]);

        sweep_points = [2]

        p = ql.Program("test_cnot_z_NN_commute", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_cnot_z_2N_commute(self):
        config_fn = os.path.join(curdir, conffile)
        platf = ql.Platform("starmon", config_fn)

        ql.set_option("scheduler_commute", 'yes');
        ql.set_option("scheduler_commute_rotations", 'no');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        # commute on z operand of CNOT/T/Z/...
        # gates on q0 commute with gates on q3 and could even be in parallel
        # q3 is cnot control and thus z-commuting operand i.e. with t q3 and z q3
        # q0 is cnot target and thus x-commuting operand, of which there are none
        # so: cnot commutes with later t q3 and z q3, making t q3 and z q3 available in parallel to q0 gates,
        # while cnot has to be after t q0 and z q0, so intended result is that t q3 and z q3 get before cnot
        #
        # only cnot/cz commutation is enabled so input order must be kept
        k.gate("t", [0]);
        k.gate("z", [0]);
        k.gate("cnot", [3,0]);
        k.gate("t", [3]);
        k.gate("z", [3]);

        sweep_points = [2]

        p = ql.Program("test_cnot_z_2N_commute", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_cnot_z_2R_commute(self):
        config_fn = os.path.join(curdir, conffile)
        platf = ql.Platform("starmon", config_fn)

        ql.set_option("scheduler_commute", 'yes');
        ql.set_option("scheduler_commute_rotations", 'yes');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        # commute on z operand of CNOT/T/Z/...
        # gates on q0 commute with gates on q3 and could even be in parallel
        # q3 is cnot control and thus z-commuting operand i.e. with t q3 and z q3
        # q0 is cnot target and thus x-commuting operand, of which there are none
        # so: cnot commutes with later t q3 and z q3, making t q3 and z q3 available in parallel to q0 gates,
        # while cnot has to be after t q0 and z q0, so intended result is that t q3 and z q3 get before cnot
        #
        # also rotation commutation is enabled so intended result as above should be result
        k.gate("t", [0]);
        k.gate("z", [0]);
        k.gate("cnot", [3,0]);
        k.gate("t", [3]);
        k.gate("z", [3]);

        sweep_points = [2]

        p = ql.Program("test_cnot_z_2R_commute", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )


    def test_cnot_x_NN_commute(self):
        config_fn = os.path.join(curdir, conffile)
        platf = ql.Platform("starmon", config_fn)

        ql.set_option("scheduler_commute", 'no');
        ql.set_option("scheduler_commute_rotations", 'no');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        # commute on x operand of CNOT/X/X45/...
        # gates on q0 commute with gates on q3 and could even be in parallel
        # q3 is cnot control and thus z-commuting operand, of which there are none
        # q0 is cnot target and thus x-commuting operand, i.e. x q0 and x45 q0
        # so: cnot commutes with later x q0 and x45 q0, making x q0 and x45 q0 available in parallel to q3 gates,
        # while cnot has to be after x q3 and x45 q3, so intended result is that x q0 and x45 q0 get before cnot
        #
        # both commutation options are 'no' so input order must be kept
        k.gate("x", [3]);
        k.gate("x45", [3]);
        k.gate("cnot", [3,0]);
        k.gate("x", [0]);
        k.gate("x45", [0]);

        sweep_points = [2]

        p = ql.Program("test_cnot_x_NN_commute", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_cnot_x_2N_commute(self):
        config_fn = os.path.join(curdir, conffile)
        platf = ql.Platform("starmon", config_fn)

        ql.set_option("scheduler_commute", 'yes');
        ql.set_option("scheduler_commute_rotations", 'no');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        # commute on x operand of CNOT/X/X45/...
        # gates on q0 commute with gates on q3 and could even be in parallel
        # q3 is cnot control and thus z-commuting operand, of which there are none
        # q0 is cnot target and thus x-commuting operand, i.e. x q0 and x45 q0
        # so: cnot commutes with later x q0 and x45 q0, making x q0 and x45 q0 available in parallel to q3 gates,
        # while cnot has to be after x q3 and x45 q3, so intended result is that x q0 and x45 q0 get before cnot
        #
        # only cnot/cz commutation is enabled so input order must be kept
        k.gate("x", [3]);
        k.gate("x45", [3]);
        k.gate("cnot", [3,0]);
        k.gate("x", [0]);
        k.gate("x45", [0]);

        sweep_points = [2]

        p = ql.Program("test_cnot_x_2N_commute", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_cnot_x_2R_commute(self):
        config_fn = os.path.join(curdir, conffile)
        platf = ql.Platform("starmon", config_fn)

        ql.set_option("scheduler_commute", 'yes');
        ql.set_option("scheduler_commute_rotations", 'yes');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        # commute on x operand of CNOT/X/X45/...
        # gates on q0 commute with gates on q3 and could even be in parallel
        # q3 is cnot control and thus z-commuting operand, of which there are none
        # q0 is cnot target and thus x-commuting operand, i.e. x q0 and x45 q0
        # so: cnot commutes with later x q0 and x45 q0, making x q0 and x45 q0 available in parallel to q3 gates,
        # while cnot has to be after x q3 and x45 q3, so intended result is that x q0 and x45 q0 get before cnot
        #
        # also rotation commutation is enabled so intended result as above should be result
        k.gate("x", [3]);
        k.gate("x45", [3]);
        k.gate("cnot", [3,0]);
        k.gate("x", [0]);
        k.gate("x45", [0]);

        sweep_points = [2]

        p = ql.Program("test_cnot_x_2R_commute", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )


    def test_cnot_NN_noncommute_RAD(self):
        config_fn = os.path.join(curdir, conffile)
        platf = ql.Platform("starmon", config_fn)

        ql.set_option("scheduler_commute", 'no');
        ql.set_option("scheduler_commute_rotations", 'no');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        # the 2 cnots don't commute although a commute would shorten the circuit's latency
        # the common qubit is q3 and it is target (x position) of the first and control (z position) of the second cnot
        # this is a RAD (or ZAX) dependence between the cnots
        #
        # gates on q5 commute with cnot q0,q3 and could be in parallel
        # but cnot q3,q5 doesn't commute with first cnot and q5 gates depend on it (and don't commute)
        # so: shortening circuit is blocked by 2nd cnot not commuting with first and with gates
        #
        # commute options are both off
        # circuit will be as input
        k.gate("cnot", [0,3]);
        k.gate("cnot", [3,5]);
        k.gate("t", [5]);
        k.gate("z", [5]);

        sweep_points = [2]

        p = ql.Program("test_cnot_NN_noncommute_RAD", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_cnot_2R_noncommute_RAD(self):
        config_fn = os.path.join(curdir, conffile)
        platf = ql.Platform("starmon", config_fn)

        ql.set_option("scheduler_commute", 'yes');
        ql.set_option("scheduler_commute_rotations", 'yes');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        # the 2 cnots don't commute although a commute would shorten the circuit's latency
        # the common qubit is q3 and it is target (x position) of the first and control (z position) of the second cnot
        # this is a RAD (or ZAX) dependence between the cnots
        #
        # gates on q5 commute with cnot q0,q3 and could be in parallel
        # but cnot q3,q5 doesn't commute with first cnot and q5 gates depend on it (and don't commute)
        # so: shortening circuit is blocked by 2nd cnot not commuting with first and with gates
        #
        # commute options are both on
        # circuit will be as input
        k.gate("cnot", [0,3]);
        k.gate("cnot", [3,5]);
        k.gate("t", [5]);
        k.gate("z", [5]);

        sweep_points = [2]

        p = ql.Program("test_cnot_2R_noncommute_RAD", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_cnot_NN_noncommute_DAR(self):
        config_fn = os.path.join(curdir, conffile)
        platf = ql.Platform("starmon", config_fn)

        ql.set_option("scheduler_commute", 'no');
        ql.set_option("scheduler_commute_rotations", 'no');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        # the 2 cnots don't commute although a commute would shorten the circuit's latency
        # the common qubit is q3 and it is control (z position) of the first and target (x position) of the second cnot
        # this is a DAR (or XAZ) dependence between the cnots
        #
        # gates on q0 commute with cnot q3,q5 and could be in parallel
        # but cnot q0,q3 doesn't commute with first cnot and q0 gates depend on it (and don't commute)
        # so: shortening circuit is blocked by 2nd cnot not commuting with first and with gates
        #
        # commute options are both off
        # circuit will be as input
        k.gate("cnot", [3,5]);
        k.gate("cnot", [0,3]);
        k.gate("x45", [0]);
        k.gate("x", [0]);
        k.gate("x45", [0]);

        sweep_points = [2]

        p = ql.Program("test_cnot_NN_noncommute_DAR", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_cnot_2R_noncommute_DAR(self):
        config_fn = os.path.join(curdir, conffile)
        platf = ql.Platform("starmon", config_fn)

        ql.set_option("scheduler_commute", 'yes');
        ql.set_option("scheduler_commute_rotations", 'yes');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        # the 2 cnots don't commute although a commute would shorten the circuit's latency
        # the common qubit is q3 and it is control (z position) of the first and target (x position) of the second cnot
        # this is a DAR (or XAZ) dependence between the cnots
        #
        # gates on q0 commute with cnot q3,q5 and could be in parallel
        # but cnot q0,q3 doesn't commute with first cnot and q0 gates depend on it (and don't commute)
        # so: shortening circuit is blocked by 2nd cnot not commuting with first and with gates
        #
        # commute options are both on
        # circuit will be as input
        k.gate("cnot", [3,5]);
        k.gate("cnot", [0,3]);
        k.gate("x45", [0]);
        k.gate("x", [0]);
        k.gate("x45", [0]);

        sweep_points = [2]

        p = ql.Program("test_cnot_2R_noncommute_DAR", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_cnot_NN_controlcommute(self):
        config_fn = os.path.join(curdir, conffile)
        platf = ql.Platform("starmon", config_fn)

        ql.set_option("scheduler_commute", 'no');
        ql.set_option("scheduler_commute_rotations", 'no');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        # commute on control operand of CNOT
        # without any commutation, the order is kept
        k.gate("cnot", [3,0]);
        k.gate("cnot", [3,6]);
        k.gate("x45", [6]);
        k.gate("x", [6]);
        k.gate("cnot", [3,1]);
        k.gate("x45", [1]);
        k.gate("x", [1]);
        k.gate("x45", [1]);
        k.gate("x", [1]);

        sweep_points = [2]

        p = ql.Program("test_cnot_NN_controlcommute", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_cnot_2N_controlcommute(self):
        config_fn = os.path.join(curdir, conffile)
        platf = ql.Platform("starmon", config_fn)

        ql.set_option("scheduler_commute", 'yes');
        ql.set_option("scheduler_commute_rotations", 'no');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        # commute on control operand of CNOT
        # without rotation commutation, last one is most critical so cnot order is reversed wrt input order
        k.gate("cnot", [3,0]);
        k.gate("cnot", [3,6]);
        k.gate("x45", [6]);
        k.gate("x", [6]);
        k.gate("cnot", [3,1]);
        k.gate("x45", [1]);
        k.gate("x", [1]);
        k.gate("x45", [1]);
        k.gate("x", [1]);

        sweep_points = [2]

        p = ql.Program("test_cnot_2N_controlcommute", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_cnot_2R_controlcommute(self):
        config_fn = os.path.join(curdir, conffile)
        platf = ql.Platform("starmon", config_fn)

        ql.set_option("scheduler_commute", 'yes');
        ql.set_option("scheduler_commute_rotations", 'yes');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        # commute on control operand of CNOT
        # with all commutation, all gates commute, so any result order is ok
        k.gate("cnot", [3,0]);
        k.gate("cnot", [3,6]);
        k.gate("x45", [6]);
        k.gate("x", [6]);
        k.gate("cnot", [3,1]);
        k.gate("x45", [1]);
        k.gate("x", [1]);
        k.gate("x45", [1]);
        k.gate("x", [1]);

        sweep_points = [2]

        p = ql.Program("test_cnot_2R_controlcommute", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_cnot_NN_targetcommute(self):
        config_fn = os.path.join(curdir, conffile)
        platf = ql.Platform("starmon", config_fn)

        ql.set_option("scheduler_commute", 'no');
        ql.set_option("scheduler_commute_rotations", 'no');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        # commute on target operand of CNOT
        # without any commutation, the order is kept
        k.gate("cnot", [0,3]);
        k.gate("cnot", [6,3]);
        k.gate("t", [6]);
        k.gate("z", [6]);
        k.gate("cnot", [1,3]);
        k.gate("t", [1]);
        k.gate("z", [1]);
        k.gate("t", [1]);
        k.gate("z", [1]);

        sweep_points = [2]

        p = ql.Program("test_cnot_NN_targetcommute", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_cnot_2N_targetcommute(self):
        config_fn = os.path.join(curdir, conffile)
        platf = ql.Platform("starmon", config_fn)

        ql.set_option("scheduler_commute", 'yes');
        ql.set_option("scheduler_commute_rotations", 'no');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        # commute on target operand of CNOT
        # without rotation commutation, last one is most critical so cnot order is reversed wrt input order
        k.gate("cnot", [0,3]);
        k.gate("cnot", [6,3]);
        k.gate("t", [6]);
        k.gate("z", [6]);
        k.gate("cnot", [1,3]);
        k.gate("t", [1]);
        k.gate("z", [1]);
        k.gate("t", [1]);
        k.gate("z", [1]);

        sweep_points = [2]

        p = ql.Program("test_cnot_2N_targetcommute", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_cnot_2R_targetcommute(self):
        config_fn = os.path.join(curdir, conffile)
        platf = ql.Platform("starmon", config_fn)

        ql.set_option("scheduler_commute", 'yes');
        ql.set_option("scheduler_commute_rotations", 'yes');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        # commute on target operand of CNOT
        # with all commutation, all gates commute, so any result order is ok
        k.gate("cnot", [0,3]);
        k.gate("cnot", [6,3]);
        k.gate("t", [6]);
        k.gate("z", [6]);
        k.gate("cnot", [1,3]);
        k.gate("t", [1]);
        k.gate("z", [1]);
        k.gate("t", [1]);
        k.gate("z", [1]);

        sweep_points = [2]

        p = ql.Program("test_cnot_2R_targetcommute", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_cz_NN_anycommute(self):
        config_fn = os.path.join(curdir, conffile)
        platf = ql.Platform("starmon", config_fn)

        ql.set_option("scheduler_commute", 'no');
        ql.set_option("scheduler_commute_rotations", 'no');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        # commute on both operands of CZ
        # without any commutation, order is kept
        k.gate("cz", [0,3]);
        k.gate("cz", [3,6]);
        k.gate("t", [6]);
        k.gate("z", [6]);
        k.gate("cz", [1,3]);
        k.gate("t", [1]);
        k.gate("z", [1]);
        k.gate("t", [1]);
        k.gate("z", [1]);

        sweep_points = [2]

        p = ql.Program("test_cz_NN_anycommute", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_cz_2N_anycommute(self):
        config_fn = os.path.join(curdir, conffile)
        platf = ql.Platform("starmon", config_fn)

        ql.set_option("scheduler_commute", 'yes');
        ql.set_option("scheduler_commute_rotations", 'no');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        # commute on both operands of CZ
        # without rotation commutation, cz order is reversed wrt input because last one is most critical
        k.gate("cz", [0,3]);
        k.gate("cz", [3,6]);
        k.gate("t", [6]);
        k.gate("z", [6]);
        k.gate("cz", [1,3]);
        k.gate("t", [1]);
        k.gate("z", [1]);
        k.gate("t", [1]);
        k.gate("z", [1]);

        sweep_points = [2]

        p = ql.Program("test_cz_2N_anycommute", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_cz_2R_anycommute(self):
        config_fn = os.path.join(curdir, conffile)
        platf = ql.Platform("starmon", config_fn)

        ql.set_option("scheduler_commute", 'yes');
        ql.set_option("scheduler_commute_rotations", 'yes');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        # commute on both operands of CZ
        # with all commutation, all gates commute, so any result order is ok
        k.gate("cz", [0,3]);
        k.gate("cz", [3,6]);
        k.gate("t", [6]);
        k.gate("z", [6]);
        k.gate("cz", [1,3]);
        k.gate("t", [1]);
        k.gate("z", [1]);
        k.gate("t", [1]);
        k.gate("z", [1]);

        sweep_points = [2]

        p = ql.Program("test_cz_2R_anycommute", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_cnot_mixedcommute(self):
        config_fn = os.path.join(curdir, conffile)
        platf = ql.Platform("starmon", config_fn)

        ql.set_option("scheduler_commute", 'yes');
        ql.set_option("scheduler_commute_rotations", 'yes');

        nqubits = 7
        k = ql.Kernel("aKernel", platf, nqubits)

        # commute on mixture of operands of many CNOTs:
        # the only dependences are those reflecting that 
        # each CNOT(a,b) must be before CNOT(b,c), since b is in common but in different positions (RAD),
        # each CNOT(a,b) must be before CNOT(c,a), since a is in common but in different positions (DAR),
        # all dep chains have length 3:
        #   cnot[0,*] -> cnot[2,*] -> cnot[5,*]
        #   cnot[0,*] -> cnot[3,*] -> cnot[6,*]
        #   cnot[1,*] -> cnot[3,*] -> cnot[6,*]
        #   cnot[1,*] -> cnot[4,*] -> cnot[6,*]
        # so the end result should reflect those 3 bundles with cnots, one for each column above
        # but after rc scheduling, the middle ones because of classical control constraints, causing 4 bundles
        k.gate("cnot", [0,2]);
        k.gate("cnot", [0,3]);
        k.gate("cnot", [1,3]);
        k.gate("cnot", [1,4]);
        k.gate("cnot", [2,0]);
        k.gate("cnot", [2,5]);
        k.gate("cnot", [3,0]);
        k.gate("cnot", [3,1]);
        k.gate("cnot", [3,5]);
        k.gate("cnot", [3,6]);
        k.gate("cnot", [4,1]);
        k.gate("cnot", [4,6]);
        k.gate("cnot", [5,2]);
        k.gate("cnot", [5,3]);
        k.gate("cnot", [6,3]);
        k.gate("cnot", [6,4]);

        sweep_points = [2]

        p = ql.Program("test_cnot_mixedcommute", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

#    def test_cnot_variations(self):
#        config_fn = os.path.join(curdir, conffile)
#        platf = ql.Platform("starmon", config_fn)
#        ql.set_option("scheduler", 'ALAP');
#        ql.set_option("scheduler_commute", 'yes');
#        ql.set_option("vary_commutations", 'yes');
#
#        nqubits = 7
#        k = ql.Kernel("aKernel", platf, nqubits)
#
#        for j in range(7):
#            k.gate("x", [j])
#        # commute on mixture of operands of many CNOTs
#        # basically, each CNOT(a,b) must ultimately be before CNOT(b,a)
#        # in between, CNOT(a,b) commutes with CNOT(a,c) and CNOT(d,b)
#        # there will be several commutation sets
#        k.gate("cnot", [0,2]);
#        k.gate("cnot", [0,3]);
#        k.gate("cnot", [1,3]);
#        k.gate("cnot", [1,4]);
#        k.gate("cnot", [2,0]);
#        k.gate("cnot", [2,5]);
#        k.gate("cnot", [3,0]);
#        k.gate("cnot", [3,1]);
#        k.gate("cnot", [3,5]);
#        k.gate("cnot", [3,6]);
#        k.gate("cnot", [4,1]);
#        k.gate("cnot", [4,6]);
#        # k.gate("cnot", [5,2]);
#        # k.gate("cnot", [5,3]);
#        # k.gate("cnot", [6,3]);
#        # k.gate("cnot", [6,4]);
#        for j in range(7):
#            k.gate("x", [j])
#
#        sweep_points = [2]
#
#        p = ql.Program("test_cnot_variations", platf, nqubits)
#        p.set_sweep_points(sweep_points)
#        p.add_kernel(k)
#        p.compile()
#
#        gold_fn = curdir + '/golden/'+ p.name + '_scheduled.qasm'
#        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
#        self.assertTrue( file_compare(qasm_fn, gold_fn) )

if __name__ == '__main__':
    unittest.main()
