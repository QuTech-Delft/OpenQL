
import sys
import os
import subprocess

swig = sys.argv[1]
args = []
for arg in sys.argv[2:]:
    if arg.startswith('--FIX,'):
        inc_dirs = arg[6:].split(',SEP,')
        for inc_dir in inc_dirs:
            args.append('-I' + inc_dir)
    else:
        args.append(arg)

cmdline = [swig] + args

if 'VERBOSE' in os.environ:
    print('Fixed swig command line: ' + ' '.join(cmdline))

sys.exit(subprocess.run(cmdline).returncode)
