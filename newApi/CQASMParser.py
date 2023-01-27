import typing

################### Lexing
# omitted: JSON literals, string literals, control flow (for, ifelse,...), a lot of operators, BUNDLES

debug = False

reserved_keywords = {
    'q' : 'Q',
    'if' : 'IF',
    'for' : 'FOR',
    'then' : 'THEN',
    'else' : 'ELSE',
    'while' : 'WHILE',
    'continue' : 'CONTINUE',
    'break' : 'BREAK',
    'map': 'MAP',
    'var': 'VAR',
    'set': 'SET',
    'cond': 'COND',
}
 
tokens = ['NEWLINE', 'VERSION', 'INT_LITERAL', 'IDENTIFIER', 'FLOAT_LITERAL', 'QUBITS', 'OPERATOR'] + list(reserved_keywords.values())

def t_VERSION(t):
    r'version[ \t]+(?P<versionNumber>[0-9]+(\.[0-9]+(\.[0-9]+)?)?)'
    t.value = lexer.lexmatch.group('versionNumber')
    return t

def t_QUBITS(t):
    r'qubits[ \t]+(?P<numberOfQubits>[0-9]+)'
    t.value = int(lexer.lexmatch.group('numberOfQubits'))
    return t

def t_IDENTIFIER(t):
    r'[a-zA-Z_][a-zA-Z_0-9]*'
    t.type = reserved_keywords.get(t.value,'IDENTIFIER')
    return t

literals = ['!', ',', '.', ':', '-', '[', ']', '(', ')']

def t_FLOAT_LITERAL(t):
    r'[+-]?[0-9]+\.([0-9]+)?(e[+-]?[0-9]+)?'
    t.value = float(t.value)
    return t

def t_INT_LITERAL(t):
    r'[+-]?[0-9]+'
    t.value = int(t.value)
    return t

def t_NEWLINE(t):
    r'[\n\r;]+'
    t.lexer.lineno += t.value.count('\n')
    return t

t_ignore_whitespace = r'[\ \t]+'

def t_ignore_comment(t):
    r'(/\*(.|\n)*?\*/)|([#].*\n)'
    t.lexer.lineno += t.value.count('\n')

def t_error(t):
    print("Illegal character '%s'" % t.value[0])
    exit(1)

import ply.lex as lex
lexer = lex.lex(debug=debug)

precedence = (
    # ('left', ':'),
    # ('left', ',', '['),
)

############ Definition of the abstract syntax tree

class Instruction:
    pass

class Subcircuit:
    def __init__(self, name: str, instructions: list[Instruction], iterations: int):
        self.name = name
        self.instructions = instructions
        self.iterations = iterations
    
    def __repr__(self) -> str:
        return f".{self.name}\n" + "\n".join(map(repr, self.instructions))

class Operand:
    pass

class Literal(Operand):
    def __init__(self, value):
        self.value = value # Duck-typing allows int, float, or ...
    
    def __repr__(self) -> str:
        return f"{self.value}"
        
    def __eq__(self, other):
        if type(other) is type(self):
            if (type(other.value) is type(self.value)):
                return self.value == other.value
        return False

class Variable(Operand):
    def __init__(self, name: str):
        self.name = name
    
    def __repr__(self) -> str:
        return f"{self.name}"
        
    def __eq__(self, other):
        if type(other) is type(self):
            if (type(other.name) is type(self.name)):
                return self.name == other.name
        return False

class Qubit(Operand):
    def __init__(self, index: int):
        assert index >= 0
        self.index = index
    
    def __repr__(self) -> str:
        return f"q[{self.index}]"
    
    def __eq__(self, other):
        if type(other) is type(self):
            return self.index == other.index
        return False

class Qubits(Operand):
    def __init__(self, startQubit: Qubit, endQubit: Qubit):
        self.startQubit = startQubit
        self.endQubit = endQubit
    
    def __repr__(self) -> str:
        return f"q[{self.startQubit}:{self.endQubit}]"
    
    def __eq__(self, other):
        if type(other) is type(self):
            return self.startQubit == other.startQubit and self.endQubit == other.endQubit
        return False

class ClassicalBit(Operand):
    def __init__(self, index: int):
        assert index >= 0
        self.index = index
    
    def __repr__(self) -> str:
        return f"b[{self.index}]"
    
    def __eq__(self, other):
        if type(other) is type(self):
            return self.index == other.index
        return False

class Gate(Instruction):
    def __init__(self, name: str, operands: list[Operand], controlBits: list[ClassicalBit] = []):
        self.name = name
        self.operands = operands
        self.controlBits = controlBits
    
    def __repr__(self) -> str:
        cond = "cond(" + ", ".join(map(repr, self.controlBits)) + ") " if self.controlBits else " "
        return self.name + cond + ", ".join(map(repr, self.operands))
    
    def __eq__(self, other):
        if type(other) is type(self):
            return self.name == other.name and self.operands == other.operands and self.controlBits == other.controlBits
        return False

class Mapping(Instruction):
    def __init__(self, variable: Variable, targetQubit: Qubit):
        self.variable = variable
        self.targetQubit = targetQubit
    
    def __repr__(self) -> str:
        return f"map {self.targetQubit}, {self.variable}"
    
    def __eq__(self, other):
        if type(other) is type(self):
            return self.variable == other.variable and self.targetQubit == other.targetQubit
        return False

class QuantumCircuit:
    def __init__(self, version: str, qubits: int, subcircuits: list[Subcircuit]):
        self.version = version
        self.qubits = qubits
        self.subcircuits = subcircuits

    def __repr__(self) -> str:
        return f"version {self.version}\n\nqubits {self.qubits}\n\n" + "\n\n".join(map(repr, self.subcircuits))

################ Parsing
# omitted: matrices, json, strings, functioncalls, annot and pragma, bundles!, map, variables and a lot of operators

def p_program(p):
    '''Program : OptNewline VERSION Newline QUBITS Newline StatementList OptNewline'''
    subcircuits = []
    instrAcc = []
    currentName = "default"
    currentIterations = 1
    for instr in p[6]:
        if isinstance(instr, Instruction):
            instrAcc += [instr]
        else:
            if len(instrAcc) > 0:
                subcircuits += [Subcircuit(name = currentName, instructions = instrAcc, iterations = currentIterations)]
                instrAcc = []
            currentName = instr[0]
            currentIterations = instr[1]

    if len(instrAcc) > 0:
        subcircuits += [Subcircuit(name = currentName, instructions = instrAcc, iterations = currentIterations)]
    
    p[0] = QuantumCircuit(version = p[2], qubits = p[4], subcircuits = subcircuits)

def p_newline(p):
    '''Newline : Newline NEWLINE
                | NEWLINE'''
    pass

def p_optnewline(p):
    '''OptNewline : Newline
                  | Empty'''
    pass

def p_empty(p):
    'Empty :'
    pass

def p_statementlist_add(p):
    '''StatementList : StatementList Newline Statement'''
    p[0] = p[1] + [p[3]]

def p_statementlist_single(p):
    '''StatementList : Statement'''
    p[0] = [p[1]]

def p_statement(p):
    '''Statement : Subcircuit
                 | Map
                 | Gate'''
    p[0] = p[1]

def p_subcircuit(p):
    '''Subcircuit : '.' IDENTIFIER'''
    p[0] = (p[2], 1)

def p_subcircuit_iterations(p):
    '''Subcircuit : '.' IDENTIFIER '(' INT_LITERAL ')' '''
    p[0] = (p[2], p[4])

def p_argument_qubits(p):
    '''Argument : Q '[' INT_LITERAL ':' INT_LITERAL ']' '''
    p[0] = Qubits(startQubit = Qubit(p[3]), endQubit = Qubit(p[5]))

def p_argument_qubit(p):
    '''Argument : Q '[' INT_LITERAL ']' '''
    p[0] = Qubit(index = p[3])

def p_argument_literal(p):
    '''Argument :   INT_LITERAL
                  | FLOAT_LITERAL'''
    p[0] = Literal(value = p[1])

def p_argument_variable(p):
    '''Argument :   IDENTIFIER'''
    p[0] = Variable(name = p[1])

def p_gateApplication_add(p):
    '''GateApplication : GateApplication ',' Argument'''
    p[0] = (p[1][0], p[1][1] + [p[3]])

def p_gateApplication_single(p):
    '''GateApplication : IDENTIFIER Argument'''
    p[0] = (p[1], [p[2]])

def p_instruction_gate(p):
    '''Gate : GateApplication'''
    p[0] = Gate(name = p[1][0], operands = p[1][1], controlBits = [])

def p_map(p):
    '''Map : MAP Q '[' INT_LITERAL ']' ',' IDENTIFIER '''
    p[0] = Mapping(variable = p[7], targetQubit = p[4])

# def p_instruction_conditionalgate(p):
#     '''Gate : COND '(' ArgumentList ')' GateApplication'''
#     def f(x):
#         assert isinstance(x, Literal)
#         assert isinstance(x.value, int)
#         return ClassicalBit(x.value)
    
    # controlBits = map(f, p[3])
    # p[0] = Gate(name = p[5], operands = p[6], controlBits = controlBits)

def p_error(p):
    raise Exception("Syntax error in input!")

from ply.yacc import yacc
parser = yacc(debug=debug)

# To run the lexer only
# lex.runmain(data="""version 1.0
# # this file has been automatically generated by the OpenQL compiler please do not modify it manually.
# qubits 10

# .q
#     xm45 q[0]
#     y45 q[1]
#     x90 q[2]
#     ym45 q[3]
# """)

def parseCQASMFile(filename: str, **args):
    with open(filename, 'r') as reader:
        return parser.parse(reader.read(), debug=debug, **args)

# parseCQASMFile("/shares/bulk/plehenaff/medina/basis_change_n3.qasm")