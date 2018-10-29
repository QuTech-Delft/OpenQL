import os
import sys
import unittest

class Test_QASM_asssembler(unittest.TestCase):

    def test_QASM_assembler_present(self):
        from libQasm import libQasm

lq = None

try:
    from libQasm import libQasm as lq
    assembler_present = True
except:
    assembler_present = False

def assemble(QASM_fn):
    if assembler_present:
        lq1 = lq(qasm_file_path=QASM_fn)
        failure = lq1.getParseResult()
        if failure:
            # TODO does libQASM tracks last error
            raise RuntimeError()


if __name__ == '__main__':
    unittest.main()
