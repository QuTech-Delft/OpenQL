import os 
from cffi import FFI
from sys import platform

rootDir = os.path.dirname(os.path.realpath(__file__))
myclibDir = os.path.join(rootDir, ".." , "cbuild", "openql")

if platform == "linux" or platform == "linux2":
    myclib = os.path.join(myclibDir, "libopenql.so")

elif platform == "darwin":
    print('OS X not yet tested!')

elif platform == "win32":
    myclib = os.path.join(myclibDir, "openql.dll")

else:
    print('Unknown platform !!!')

ffi = FFI()

# Describe the data type and function prototype to cffi.
ffi.cdef('''
int init();
int schedule();
int compile();
''')

lib = ffi.dlopen(myclib)
# print('Loaded lib {0}'.format(lib))

def init():
    print('init() via cffi')
    ret = lib.init()

def schedule():
    print('schedule() via cffi')
    ret = lib.schedule()

def compile():
    print('compile() via cffi')
    ret = lib.compile()
