import os
import sys
import unittest

lq = None

try:
    from libQasm import libQasm as lq
    assembler_present = True
except:
    assembler_present = False

def assemble(QASM_fn):
    if assembler_present:
        lq1 = lq()
        lq1.parse_file(qasm_file_path=QASM_fn)
        failure = lq1.getParseResult()
        if failure:
            # TODO does libQASM tracks last error
            raise RuntimeError()


if __name__ == '__main__':
    unittest.main()
