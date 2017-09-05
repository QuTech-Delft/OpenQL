# This program is used to generate parts of the lexer and parser, so that
# the assembler knows the opcodes of the classic and quantum instructions.
# the file './qisa_instructions.def' is used as a source.
# See that file for information on how to specify the necessary fields.
import os.path
import string


# Parse command line arguments.
import argparse
parser = argparse.ArgumentParser(
    description="Generate C++/Parser/Lexer from qisa instructions def file and template input files.")
parser.add_argument('-d', '--def-file', metavar='DEF_FILE',
                    dest='def_file',
                    required=True,
                    help="QISA instructions definition file")

parser.add_argument('-co', '--cpp-output', metavar='CPP_OUTPUT_FILE',
                    dest='cpp_output',
                    required=True,
                    help="Destination cpp output file.")

parser.add_argument('-lt', '--lexer-template', metavar='LEXER_TEMPLATE_FILE',
                    dest='lexer_template',
                    required=True,
                    help="Input template file for generating the lexer")
parser.add_argument('-lo', '--lexer-output', metavar='LEXER_OUTPUT_FILE',
                    dest='lexer_output',
                    required=True,
                    help="Destination lexer file")

parser.add_argument('-pt', '--parser-template', metavar='PARSER_TEMPLATE_FILE',
                    dest='parser_template',
                    required=True,
                    help="Input template file for generating the parser")
parser.add_argument('-po', '--parser-output', metavar='PARSER_OUTPUT_FILE',
                    dest='parser_output',
                    required=True,
                    help="Destination parser file")


args = parser.parse_args()

# A classic opcode has 6 bits.
max_c_opcode = 2**6 - 1

# A quantum opcode has 8 bits.
max_q_opcode = 2**8 - 1

def_opcode={}
def_q_arg_none={}
def_q_arg_st={}
def_q_arg_tt={}


used_c_opcodes = []
used_q_opcodes = []

exec(open(args.def_file).read())

# For the purpose of handling the opcodes and the lexer output, we merge
# all quantum instruction definitions in one dictionary.
q_all = def_q_arg_none.copy()
q_all.update(def_q_arg_st)
q_all.update(def_q_arg_tt)

# ---------------------
# Handle the cpp output
# ---------------------

encountered_error = False

try:
    with open(args.cpp_output, 'w') as fd:
        print('/*' + ('*' * 76) + '*/', file=fd)
        print('/* ' + 'Automatically generated, do not edit.'.ljust(75) + '*/', file=fd)
        print('/*' + ('*' * 76) + '*/\n\n', file=fd)
        print('namespace QISA {\n', file=fd)
        print('void', file=fd)
        print('QISA_Driver::setOpcodes()', file=fd)
        print('{', file=fd)

        print('', file=fd)
        print('  // Opcodes for the Classic Instructions (Single Instruction Format)', file=fd)
        for inst,opc in sorted(def_opcode.items(), key=lambda x: x[1]):
            if ((opc < 0) or
                (opc > max_c_opcode)):
                print("Opcode for '{0}' ({1}) is out of range. Acceptable range = [0,{2}]".format(inst, opc, max_c_opcode))
                encountered_error = True
                break
            if opc in used_c_opcodes:
                print("Opcode for '{0}' ({1}) has already been used.".format(inst, opc))
                encountered_error = True
                break
            used_c_opcodes.append(opc)

            print('  _opcodes["{0}"]'.format(inst.upper()).ljust(40) +
                  "= {0:#04x};".format(opc), file=fd)

        print('', file=fd)
        print('  // Reverse lookup of above', file=fd)

        for inst,opc in sorted(def_opcode.items(), key=lambda x: x[1]):
            print('  _classicOpcode2instName[{0:#04x}]'.format(opc).ljust(40) +
                  '= "{0}";'.format(inst.upper()), file=fd)

        print('', file=fd)
        print('  // Opcodes for the Quantum Instructions (Double Instruction Format)', file=fd)

        if not encountered_error:
            for inst,opc in sorted(q_all.items(), key=lambda x: x[1]):
                if ((opc < 0) or
                    (opc > max_q_opcode)):
                    print("Opcode for '{0}' ({1}) is out of range. Acceptable range = [0,{2}]".format(inst, opc, max_q_opcode))
                    encountered_error = True
                    break
                if opc in used_q_opcodes:
                    print("Opcode for '{0}' ({1}) has already been used.".format(inst, opc))
                    encountered_error = True
                    break
                used_q_opcodes.append(opc)

                print('  _opcodes["{0}"]'.format(inst.upper()).ljust(40) +
                      "= {0:#04x};".format(opc), file=fd)

        if not encountered_error:
            print('', file=fd)
            print('  // Reverse lookup of above', file=fd)

            for inst,opc in sorted(q_all.items(), key=lambda x: x[1]):
                print('  _quantumOpcode2instName[{0:#04x}]'.format(opc).ljust(40) +
                      '= "{0}";'.format(inst.upper()), file=fd)

        if not encountered_error:
            print('', file=fd)
            print('  // Contains the opcodes for the quantum instructions specifying an st argument.', file=fd)

            for opc in sorted(def_q_arg_st.values()):
                print('  _q_inst_arg_st.insert({0:#04x});'.format(opc), file=fd)

            print('', file=fd)
            print('  // Contains the opcodes for the quantum instructions specifying a tt argument.', file=fd)

            for opc in sorted(def_q_arg_tt.values()):
                print('  _q_inst_arg_tt.insert({0:#04x});'.format(opc), file=fd)

        if not encountered_error:
            print('}\n', file=fd)
            print('} // namespace QISA', file=fd)

except Exception as e:
    print("Exception occured while writing to file '{0}': {1}".format(args.cpp_output, e))
    error_encountered = True

# Remove the generated file upon error
if encountered_error:
    try:
        os.remove(args.cpp_output)
    except:
        pass
    sys.exit(1)  # Exit with code 1 to indicate failure.

# ---------------------
# Handle the lexer output
# ---------------------

lexer_output_str = ''

for inst in sorted(q_all.keys()):
    inst_str = '"{0}"'.format(inst.upper())
    lexer_output_str += (inst_str.ljust(15) +
                         '{{ return QISA::QISA_Parser::make_{0}(loc); }}\n'.format(inst.upper()))

try:
    # open the template file
    with open(args.lexer_template) as template_file:

        # read it into a template object
        template_src = string.Template(template_file.read())

        # do the substitution
        new_lexer_src = template_src.safe_substitute({'template_input_quantum_instructions':lexer_output_str})

except Exception as e:
    print("Exception occured while reading file '{0}': {1}".format(args.lexer_template, e))
    sys.exit(1)  # Exit with code 1 to indicate failure.


# Write the new contents
try:
    with open(args.lexer_output, 'w') as fd:
        print(new_lexer_src, file=fd, end='')

except Exception as e:
    print("Exception occured while writing to file '{0}': {1}".format(args.lexer_output, e))
    error_encountered = True

if encountered_error:
    try:
        os.remove(args.lexer_output)
    except:
        pass
    sys.exit(1)  # Return 1 to indicate failure.

# ---------------------
# Handle the parser output
# ---------------------

# Add the tokens.
parser_tokens_str = ''

for inst in sorted(q_all.keys()):
    parser_tokens_str += '%token {0}\n'.format(inst.upper())

# Now define the quantum instruction grammars.

parser_q_arg_none_str = None
cnt = 0
if def_q_arg_none:
    parser_q_arg_none_str = 'q_instr_name_arg_none\n'
    for inst in sorted(def_q_arg_none.keys()):
        sep = ':' if cnt == 0 else '|'
        parser_q_arg_none_str += '  {0} {1:<10} {{ $$ = "{1}"; }}\n'.format(sep, inst.upper())
        cnt += 1
    parser_q_arg_none_str += "  ;\n"

parser_q_arg_st_str = None
cnt = 0
if def_q_arg_st:
    parser_q_arg_st_str = 'q_instr_name_arg_st\n'
    for inst in sorted(def_q_arg_st.keys()):
        sep = ':' if cnt == 0 else '|'
        parser_q_arg_st_str += '  {0} {1:<10} {{ $$ = "{1}"; }}\n'.format(sep, inst.upper())
        cnt += 1
    parser_q_arg_st_str += "  ;\n"

parser_q_arg_tt_str = None
cnt = 0
if def_q_arg_tt:
    parser_q_arg_tt_str = 'q_instr_name_arg_tt\n'
    for inst in sorted(def_q_arg_tt.keys()):
        sep = ':' if cnt == 0 else '|'
        parser_q_arg_tt_str += '  {0} {1:<10} {{ $$ = "{1}"; }}\n'.format(sep, inst.upper())
        cnt += 1
    parser_q_arg_tt_str += "  ;\n"

# Create a substitution map, that will be used to substitute variables in
# the parser template.
template_substitution_map = {'template_parser_quantum_tokens' : parser_tokens_str,
                             'template_parser_q_instr_arg_none' : parser_q_arg_none_str,
                             'template_parser_q_instr_arg_st' : parser_q_arg_st_str,
                             'template_parser_q_instr_arg_tt' : parser_q_arg_tt_str}

try:
    # open the template file
    with open(args.parser_template) as template_file:

        # read it into a template object
        template_src = string.Template(template_file.read())

        # do the substitution
        new_parser_src = template_src.safe_substitute(template_substitution_map)

except Exception as e:
    print("Exception occured while reading file '{0}': {1}".format(args.lexer_template, e))
    sys.exit(1)  # Exit with code 1 to indicate failure.


# Write the new contents
try:
    with open(args.parser_output, 'w') as fd:
        print(new_parser_src, file=fd, end='')

except Exception as e:
    print("Exception occured while writing to file '{0}': {1}".format(args.parser_output, e))
    error_encountered = True

if encountered_error:
    try:
        os.remove(args.parser_output)
    except:
        pass
    sys.exit(1)  # Return 1 to indicate failure.




# // For parser.
# %token               ADD

# | ADD reg COMMA reg COMMA reg { if (!driver.generate_Q_INSTR($2, @2, $4, @4, $6, @6)) { YYERROR;} }

# instruction
#   : NEWLINE
#   | definition    NEWLINE
#   | register_decl NEWLINE
#   | label_decl    NEWLINE
#   | statement     NEWLINE

# print("Would store this in file: {}".format(args.dest_file))
