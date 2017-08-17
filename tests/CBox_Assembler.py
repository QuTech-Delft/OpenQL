import string
import logging


def is_number(s):
    try:
        float(s)
        return True
    except ValueError:
        return False


def RepresentsInt(s):
    try:
        int(s)
        return True
    except ValueError:
        return False


def get_bin(x, n):
    '''
    Return the 2's complement of the integer number $x$
    for a given bitwidth $n$.
    '''
    if (not is_number(x)):
        raise ValueError('get_bin: parameter {} is not a number.'.format(x))

    return '{0:{fill}{width}b}'.format((int(x) + 2**n) % 2**n,
                                       fill='0', width=n)


def bin_to_hex(bin_val, hex_width):
    if (not is_number(bin_val)):
        raise ValueError('bin_to_hex: parameter' + str(bin_val) +
                         'is not a number.')

    return format(int(bin_val, 2), 'x').zfill(hex_width)


def dec_to_bin_w4(x):
    if (not is_number(x)):
        raise ValueError('Parameter is not a number.')

    return '{0:04b}'.format(x)


def dec_to_bin_w8(x):
    if (not is_number(x)):
        raise ValueError('Parameter is not a number.')

    return '{0:08b}'.format(x)


class Assembler():

    def __init__(self, asm_filename):
        self.asmfilename = asm_filename
        # Control if a nop is added after each label.
        self.add_nop_after_label = False

        # Control the nops added after each branch instruction.
        self.add_nop_after_branch = True
        self.number_of_nops_appended = 5

        self.tfp = None

    InstOpCode = {'add':     '000000',
                  'sub':     '000000',
                  'beq':     '000100',
                  'bne':     '000101',
                  'addi':    '001000',
                  'lui':     '001111',
                  'waitreg': '000000',
                  'pulse':   '000000',
                  'measure': '000000',
                  'wait':    '110000',
                  'trigger': '101000'}

    InstfunctCode = {'add':     '100000',
                     'sub':     '100010',
                     'waitreg': '010000',
                     'pulse':   '000001',
                     'measure': '000010'}

    MAX_WAIT_TIME = 2**15 - 1
    MIN_WAIT_TIME = 1

    def get_reg_num(self, Register):
        '''
        It gets the register number from the input string.

        @param Register : the input string represents a register
        @return stat : 4-bit binary string representing the register number
        '''
        if (not is_number(Register.strip('r'))):
            raise ValueError("Register format is not correct.")

        reg_num = int(Register.strip('r'))

        if (reg_num < 0 or reg_num > 15):
            raise ValueError("Register number is out of range.")

        return dec_to_bin_w4(reg_num)

    def get_lui_pos(self, pos):

        if (not is_number(pos)):
            raise ValueError("Position input is not a number.")

        val_pos = int(pos)

        if (val_pos < 0 or val_pos > 3):
            raise ValueError("Position input is out of range ({0,1,2,3}\
                              expected).")

        elif (val_pos == 0):
            position = '0001'
        elif (val_pos == 1):
            position = '0010'
        elif (val_pos == 2):
            position = '0100'
        else:
            position = '1000'

        return position

    # lui rt, pos, byte_data
    def LuiFormat(self, args):
        Register, pos, putByte = args

        try:
            opCode = self.InstOpCode['lui']
            FDC = '100'
            rt = self.get_reg_num(Register)
            position = self.get_lui_pos(pos)
            imm8 = dec_to_bin_w8(int(putByte))
            return opCode + FDC + rt + rt + position + '000' + imm8

        except ValueError as detail:
            raise ValueError('Lui instruction format error:{}'.format(
                             detail.args))

    # add rd, rs, rt
    def AddFormat(self, args):
        dst_reg, src_reg1, src_reg2 = args

        try:
            opCode = self.InstOpCode['add']
            FDC = '100'
            rd = self.get_reg_num(dst_reg)
            rs = self.get_reg_num(src_reg1)
            rt = self.get_reg_num(src_reg2)
            shamt = '00000'
            funct = self.InstfunctCode['add']
            return opCode + FDC + rs + rt + rd + shamt + funct

        except ValueError as detail:
            raise ValueError('Add instruction format error: {}'.format(
                             detail.args))

    # sub rd, rs, rt
    def SubFormat(self, args):
        dst_reg, src_reg1, src_reg2 = args

        try:
            opCode = self.InstOpCode['sub']
            FDC = '100'
            rd = self.get_reg_num(dst_reg)
            rs = self.get_reg_num(src_reg1)
            rt = self.get_reg_num(src_reg2)
            shamt = '00000'
            funct = self.InstfunctCode['sub']
            return opCode + FDC + rs + rt + rd + shamt + funct

        except ValueError as detail:
            raise ValueError('Sub instruction format error: {}'.format(
                             detail.args))

    # beq rs, rt, off
    def BeqFormat(self, args):
        src_reg1, src_reg2, offset15 = args
        try:
            opCode = self.InstOpCode['beq']
            FDC = '100'
            rs = self.get_reg_num(src_reg1)
            rt = self.get_reg_num(src_reg2)
            imm15 = get_bin(offset15, 15)
            return opCode + FDC + rs + rt + imm15

        except ValueError as detail:
            raise ValueError('Beq instruction format error: {}'.format(
                             detail.args))

    # bne rs, rt, off
    def BneFormat(self, args):
        src_reg1, src_reg2, offset15 = args
        try:
            opCode = self.InstOpCode['bne']
            FDC = '100'
            rs = self.get_reg_num(src_reg1)
            rt = self.get_reg_num(src_reg2)
            immValue = get_bin(offset15, 15)
            return opCode + FDC + rs + rt + immValue

        except ValueError as detail:
            raise ValueError('Bne instruction format error: {}'.format(
                             detail.args))

    # addi rt, rs, imm
    def AddiFormat(self, args):
        dst_reg, src_reg1, imm15 = args
        try:
            opCode = self.InstOpCode['addi']
            FDC = '100'
            rs = self.get_reg_num(src_reg1)
            rt = self.get_reg_num(dst_reg)
            immValue = get_bin(imm15, 15)
            return opCode + FDC + rs + rt + immValue

        except ValueError as detail:
            raise ValueError('Addi instruction format error: {}'.format(
                             detail.args))

    # waitreg rs
    def WaitRegFormat(self, args):
        src_reg, = args
        try:
            opCode = self.InstOpCode['waitreg']
            FDC = '001'
            rs = self.get_reg_num(src_reg)
            zero13 = '0000000000000'
            funct = self.InstfunctCode['waitreg']
            return opCode + FDC + rs + zero13 + funct

        except ValueError as detail:
            raise ValueError('WaitReg instruction format error: {}'.format(
                detail.args))

    # pulse AWG0, AWG1, AWG2
    def PulseFormat(self, args):
        awg0, awg1, awg2 = args
        try:
            opCode = self.InstOpCode['pulse']
            FDC = '001'
            shamt = '00000'
            funct = self.InstfunctCode['pulse']
            return opCode + FDC + awg0 + awg1 + awg2 + shamt + funct

        except ValueError as detail:
            raise ValueError('Pulse instruction format error: {}'.format(
                detail.args))

    # measure
    def MeasureFormat(self, args):
        assert(len(args) == 0)
        try:
            opCode = self.InstOpCode['measure']
            FDC = '011'
            zero4 = '0000'
            rt = '0000'
            zero9 = '000000000'
            funct = self.InstfunctCode['measure']
            return opCode + FDC + zero4 + rt + zero9 + funct

        except ValueError as detail:
            raise ValueError('Measure instruction format error: {}'.format(
                detail.args))

    # wait imm
    def WaitFormat(self, args):
        imm15, = args

        if not RepresentsInt(imm15):
            raise ValueError(
                "Waiting time {} is not an integer.".format(imm15))

        if (int(imm15) < self.MIN_WAIT_TIME or int(imm15) > self.MAX_WAIT_TIME):
            raise ValueError(
                "Waiting time {} out of range: 1 ~ 32767.".format(imm15))

        try:
            opCode = self.InstOpCode['wait']
            FDC = '001'
            zero8 = '00000000'
            immValue = get_bin(imm15, 15)
            return opCode + FDC + zero8 + immValue

        except ValueError as detail:
            raise ValueError('Wait instruction format error: {}'.format(
                             detail.args))

    # trigger mask, duration
    def TriggerFormat(self, args):
        mask, imm11 = args

        if len(mask) != 7:
            raise ValueError('The mask "{}" should be 7 bits. \
                              With the MSb indicating marker 1, \
                              and the LSb indicating marker 7.'.format(mask))
        for b in mask:
            if (b != '0' and b != '1'):
                raise ValueError(
                    'The mask "{}" should only contain 1 or 0.'.format(mask))

        # In the core of 3.1.0, the MSb works for the trigger 7.
        # Reverse the string so that the MSb works for trigger 1.
        mask = mask[::-1]

        mask = "00000" + mask   # The mask should be 12-bit wide.

        if int(imm11) < 0 or int(imm11) > 2047:
            raise ValueError("the value of the duration time is out of range \
                              (accepted: integer in 0~2047).")
        try:
            opCode = self.InstOpCode['trigger']
            FDC = '001'
            immValue = get_bin(imm11, 12)       # TODO: Check Range
            return opCode + FDC + mask + immValue[1:]

        except ValueError as detail:
            raise ValueError('Trigger instruction format error: {}'.format(
                             detail.args))

    def NopFormat(self, args):
        assert(len(args) == 0)
        return "00000000000000000000000000000000"

    inst_translation_func = {
        'add':      AddFormat,
        'sub':      SubFormat,
        'beq':      BeqFormat,
        'bne':      BneFormat,
        'addi':     AddiFormat,
        'lui':      LuiFormat,
        'waitreg':  WaitRegFormat,
        'pulse':    PulseFormat,
        'measure':  MeasureFormat,
        'wait':     WaitFormat,
        'trigger':  TriggerFormat,
        'nop':      NopFormat
    }

    @classmethod
    def remove_comment(self, line):
        line = line.split('#', 1)[0]  # remove anything after '#' symbole
        line = line.strip(' \t\n\r')  # remove whitespace
        return line

    @classmethod
    def split_line_elements(self, line):
        head, sep, tail = line.partition(':')
        if (sep == ":"):
            label = head.strip()
            instr = tail.strip()
        else:
            label = ''
            instr = head.strip()

        label_instr = self.get_instruction_elements(instr)
        if len(label_instr) == 0:
            label_instr.append('')
        label_instr.insert(0, label)

        return label_instr

    @classmethod
    def get_instruction_elements(self, pureInstruction):
        return [rawEle.strip(string.punctuation.translate(
                {ord('-'): None})) for rawEle in pureInstruction.split()]

    def get_valid_lines(self):
        '''
        Read all lines with valid instructions or labels.
        Convert every string into lower case.
        '''
        try:
            Asm_File = open(self.asmfilename, 'r', encoding="utf-8")
            logging.info("open file " + self.asmfilename + " successfully.")
        except:
            raise OSError('\tError: Fail to open file ' +
                          self.asmfilename + ".")

        self.valid_lines = []
        for line in Asm_File:
            line = self.remove_comment(line)
            if (len(line) == 0):  # skip empty line and comment
                continue
            self.valid_lines.append(line.lower())

        Asm_File.close()

    def assemble(self, verbose=False):
        # label, name, param[0], param[1] ...
        self.label_instrs = []

        self.get_valid_lines()
        self.convert_line_to_ele_array()
        self.add_end_file_loop()
        self.remove_wait_zero()
        self.insert_nops()
        self.decompose()
        self.split_long_wait()
        self.merge_consecutive_wait()
        if verbose:
            self.print_label_instrs()
        self.get_label_addr()
        self.cal_branch_offset()

    def add_end_file_loop(self):
        # Append the following instructions at the end of the file
        # It loops forever and the marker 7 will always be high
        self.label_instrs.append(['end_of_file_loop', 'wait', '1000'])
        self.label_instrs.append(['', 'trigger', '0000001', '1000'])
        self.label_instrs.append(['', 'beq', 'r0', 'r0', 'end_of_file_loop'])

        self.align_labels()

    def split_long_wait(self):
        '''
        This function merges two consecutive WAIT instructions if the sum
        of the waiting time of both instructions is not larger than
        MAX_WAIT_TIME.
        '''
        i = 0
        while i < len(self.label_instrs):
            label_instr = self.label_instrs[i]

            # I only care the WAIT instruction here.
            if (label_instr[1] != 'wait'):
                i = i + 1
                continue
            remain_wait_time = int(label_instr[2])
            if remain_wait_time < self.MAX_WAIT_TIME:
                i += 1
            while (remain_wait_time > self.MAX_WAIT_TIME):
                self.label_instrs[i][2] = str(self.MAX_WAIT_TIME)
                remain_wait_time -= self.MAX_WAIT_TIME
                new_wait_instr = ['', 'wait', str(remain_wait_time)]
                self.label_instrs.insert(i+1, new_wait_instr)
                i += 1

    def merge_consecutive_wait(self):
        '''
        This function merges two consecutive WAIT instructions if the sum
        of the waiting time of both instructions is not larger than
        MAX_WAIT_TIME.
        '''
        i = 0
        while i < len(self.label_instrs):
            label_instr = self.label_instrs[i]

            if (i >= len(self.label_instrs)-1):
                break
            next_label_instr = self.label_instrs[i+1]

            # I only care the WAIT instruction here.
            if (label_instr[1] != 'wait'):
                i = i + 1
                continue

            # If the next instruction is a target of a jump instruction,
            # then, we should not merge the wait instruction.
            if next_label_instr[0] != '':
                i = i + 1
                continue

            # if the next instruction is not wait, then no merge.
            if (next_label_instr[1] != 'wait'):
                i = i + 1
                continue

            # OK, current and next instructions are both WAIT.
            # No worry about the labels.
            # If the sum waiting time is within the valid range, merge them.
            if (int(label_instr[2]) +
                    int(next_label_instr[2]) < self.MAX_WAIT_TIME):
                label_instr[2] = str(
                    int(label_instr[2]) + int(next_label_instr[2]))
                self.label_instrs[i] = label_instr
                self.label_instrs.pop(i+1)
            else:
                i = i + 1

    def cal_branch_offset(self):
        '''
        Calculate the offset used in branch instructions.
        '''
        for index, label_instr in enumerate(self.label_instrs):
            # print("instruction:", label_instr)
            if (label_instr[1] == 'beq' or label_instr[1] == 'bne'):
                # print("instruction found:", label_instr)
                if label_instr[4] not in self.label_addr_dict:
                    raise ValueError("{}: Target label {} not found.".format(
                        label_instr[0], label_instr[4]))
                offset = self.label_addr_dict[label_instr[4]] - (index + 1)
                label_instr[4] = offset
                self.label_instrs[index] = label_instr

    def decompose(self):
        '''
        Decompose emulated instruction into atomic instructions.
        E.g., mov -> 4 lui instructions
        '''
        for i, label_instr in enumerate(self.label_instrs):
            if (label_instr[1] == 'mov'):
                label = label_instr[0]

                Register = label_instr[2]

                imm32 = label_instr[3]
                bit32 = get_bin(imm32, 32)
                putByte0 = int(bit32[24:32], 2)
                putByte1 = int(bit32[16:24], 2)
                putByte2 = int(bit32[8:16], 2)
                putByte3 = int(bit32[0:8], 2)

                lui0 = [label, 'lui', Register, 0, putByte0]
                lui1 = ['', 'lui', Register, 1, putByte1]
                lui2 = ['', 'lui', Register, 2, putByte2]
                lui3 = ['', 'lui', Register, 3, putByte3]
                self.label_instrs[i] = lui0
                self.label_instrs.insert(i+1, lui1)
                self.label_instrs.insert(i+2, lui2)
                self.label_instrs.insert(i+3, lui3)

        self.align_labels()

    def convert_line_to_ele_array(self):
        self.label_instrs = []
        for line in self.valid_lines:
            self.label_instrs.append(self.split_line_elements(line))

        self.align_labels()

    def print_label_instrs(self):
        for label_instr in self.label_instrs:
            print(label_instr)

    def align_labels(self):
        '''
        Align the label with corresponding instruction, so that
        every line is occupied by one instruction.
        '''
        for i, label_instr in enumerate(self.label_instrs):

            if label_instr[0] == '' and label_instr[1] == '':
                # remove empty line
                self.label_instrs.pop(i)
                continue

            if label_instr[0] != '' and label_instr[1] == '':
                # A label without an instruction
                if (i == len(self.label_instrs) - 1):
                    # last instruction. Add a nop for this label
                    self.label_instrs[i] = [label_instr[0], 'nop']
                else:
                    # not last instruction.
                    next_label_instr = self.label_instrs[i + 1]
                    if (next_label_instr[0] != ''):
                        # next instruction has a lable, throw away the current
                        # one
                        self.label_instrs.pop(i)
                    else:
                        # next instruction has no tag, merge the current label
                        # with next instruction
                        next_label_instr[0] = label_instr[0]
                        self.label_instrs[i + 1] = next_label_instr
                        self.label_instrs.pop(i)

        self.get_label_addr()

    def get_label_addr(self):
        self.label_addr_dict = {}
        for i, label_instr in enumerate(self.label_instrs):
            label = label_instr[0]

            if (len(label) == 0):
                continue

            if (label in self.label_addr_dict):
                raise ValueError(
                    "Redefintion of the label {} in the QuMIS file {}".format(
                        label, self.asmfilename))

            self.label_addr_dict[label] = i

    def insert_nops(self):
        self.align_labels()
        if (self.add_nop_after_label):
            self.insert_nop_after_label()

        if (self.add_nop_after_branch):
            self.insert_nop_after_branch()
        self.align_labels()

    def insert_nop_after_label(self):
        for (index, label_instr) in enumerate(self.label_instrs):
            if (label_instr[0] == ''):
                continue
            if (label_instr[1] == 'nop'):
                continue

            label = label_instr[0]
            self.label_instrs[index] = [label, 'nop']
            label_instr[0] = ''
            self.label_instrs.insert(index + 1, label_instr)

    def insert_nop_after_branch(self):
        max_addr = len(self.label_instrs)
        addr = 0
        while (addr < max_addr):
            if (self.label_instrs[addr][1] == 'beq' or
                    self.label_instrs[addr][1] == 'bne'):

                nop_instr = ['', 'nop']
                for i in range(self.number_of_nops_appended):
                    self.label_instrs.insert(addr + 1, nop_instr)
                addr = addr + self.number_of_nops_appended
                max_addr = len(self.label_instrs)
            addr = addr + 1

    def remove_wait_zero(self):
        for i, label_instr in enumerate(self.label_instrs):
            if (label_instr[1] == 'wait' and label_instr[2] == '0'):
                label_instr = [label_instr[0], '']
                self.label_instrs[i] = label_instr

        self.align_labels()

    def convert_to_instructions(self, verbose=False):
        '''
        The main function that performs the translation from the QuMIS file
        into binary instructions.
        '''
        self.assemble(verbose=verbose)
        self.instructions = []

        for label_instr in self.label_instrs:
            elements = label_instr[1:]
            translate_function = self.inst_translation_func[
                elements[0].lower()]
            self.instructions.append(
                int(translate_function(self, elements[1:]), 2))

        return self.instructions

    def getTextInstructions(self):
        '''
        Show the final instructions after expanding the mov instruction
        and appending nop instruction after labels and beq/bne.
        '''
        self.label_instrs = []
        self.get_valid_lines()
        self.convert_line_to_ele_array()
        self.add_end_file_loop()
        self.remove_wait_zero()
        self.insert_nops()
        self.decompose()
        self.split_long_wait()
        self.merge_consecutive_wait()
        self.get_label_addr()
        self.text_instructions = []
        for label_instr in self.label_instrs:
            if (label_instr[0] != ''):
                label_instr[0] = label_instr[0] + ':'

            text_instruction = ""
            for i, ele in enumerate(label_instr):
                if (i == 0):
                    text_instruction = ele
                elif (i == 1):
                    text_instruction = text_instruction + ele
                else:
                    text_instruction = text_instruction + ', {}'.format(ele)

            self.text_instructions.append(text_instruction)
        return self.text_instructions
