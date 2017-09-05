### Test files for the QISA Assembler

This directory contains some valid and some invalid input files that are used to test the QISA Assembler.

* `test_assembly.qisa`       Contains most of the classic and quantum instructions that should all assemble without errors.
* `test_s_mask.qisa`         Contains SMIS instructions using different input formats for the s_mask, but producing
  all the same encoded instructions.
* `test_t_mask.qisa`        Contains SMIT instructions using different input formats for the t_mask, but producing
  all the same encoded instructions.
* `test_wrong_s_mask.qisa`   Contains a SMIS instruction that uses a wrong s_mask specification.
  The assembler should complain about a duplicate entry.
* `test_wrong_t_mask_1.qisa` Contains a SMIS instruction that uses a wrong t_mask specification.
  The assembler should complain about a qubit that is used in more than one target-control pair.
* `test_wrong_t_mask_2.qisa` Contains a SMIS instruction that uses a wrong t_mask specification in another
  input format.
  The assembler should complain about a another qubit that is used in more than one target-control pair.
