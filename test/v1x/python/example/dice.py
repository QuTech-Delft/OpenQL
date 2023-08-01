import matplotlib.pyplot as plt
import openql as ql
import os
import qxelarator

from config import output_dir
from functools import reduce


ql.set_option('output_dir', output_dir)
ql.set_option('log_level', 'LOG_INFO')

num_qubits = 3


def dice_compile():
    print('compiling 8-face dice program by openql')

    platform = ql.Platform('myPlatform', 'none')
    p = ql.Program('dice', platform, num_qubits)
    k = ql.Kernel('aKernel', platform, num_qubits)

    for q in range(num_qubits):
        k.gate('h', [q])

    for q in range(num_qubits): 
        k.gate('measure', [q])

    p.add_kernel(k)
    p.compile()


def plot_histogram(dice_faces):
    plt.hist(dice_faces, bins=8, color='#0504aa', alpha=0.7, rwidth=0.85)
    plt.grid(axis='y', alpha=0.75)
    plt.xlabel('Dice Face', fontsize=15)
    plt.ylabel('Frequency', fontsize=15)
    plt.xticks(fontsize=15)
    plt.yticks(fontsize=15)
    plt.ylabel('Frequency', fontsize=15)
    plt.title('Histogram', fontsize=15)
    plt.show()
    plt.savefig('hist.png')


def dice_execute_single_shot():
    print('executing 8-face dice program on qxelarator')
    qx = qxelarator.QX()
    qx.set(os.path.join(output_dir, 'dice.qasm'))
    qx.execute()
    res = [int(qx.get_measurement_outcome(q)) for q in range(num_qubits)]
    dice_face = reduce(lambda x, y: 2*x+y, res, 0) + 1
    print('Dice face : {}'.format(dice_face))


def dice_execute_multi_shot():
    print('executing 8-face dice program on qxelarator')
    qx = qxelarator.QX()
    qx.set(os.path.join(output_dir, 'dice.qasm'))
    dice_faces = []
    num_tests = 100
    for i in range(num_tests):
        qx.execute()
        res = [int(qx.get_measurement_outcome(q)) for q in range(num_qubits)]
        dice_face = reduce(lambda x, y: 2*x+y, res, 0) + 1
        dice_faces.append(dice_face)

    plot_histogram(dice_faces)


if __name__ == '__main__':
    dice_compile()
    dice_execute_single_shot()
    dice_execute_multi_shot()
