
#pragma once

#include <string>
#include <cstddef>
#include <istream>
#include <sstream>
#include <map>
#include <set>
#include <algorithm>

#include "qisa_parser.tab.hh"

// Tell Flex the lexer's prototype ...
# define YY_DECL \
    QISA::QISA_Parser::symbol_type yylex (QISA::QISA_Driver& driver)
// ... and declare it for the parser's sake.
YY_DECL;

// In the core of 3.1.0, the MSb of the mask works for the trigger 7.
// However, in the new core the MSb works for trigger 1.
//
// Define REV_TRIGGER_MASK in order to keep the input assembly compatible with the old version and reverse the
// mask bits.
#define REV_TRIGGER_MASK


// Due to some timing issues within the current processor implementation, a certain number of NOPS have to be
// inserted after a branch.
// Define ADD_NOPS_AFTER_BRANCH to the number of these extra NOPS if you want them.
// This number can be zero or positive.
#define ADD_NOPS_AFTER_BRANCH 5


#ifdef _WIN32
#define DllExport __declspec(dllexport)
#else
#define DllExport
#endif

namespace QISA
{

class QISA_Driver
{
public: // -- types

    // Defines the type of a target-control pair.
    typedef std::pair<uint8_t, uint8_t>   TargetControlPair;

public: // -- constants

  // Register kind.
  // Used to denote which set of registers are being used.
  enum RegisterKind
  {
    Q_REGISTER,
    R_REGISTER,
    S_REGISTER,
    T_REGISTER
  };

  // Branch conditions.
  // This is the encoding of the different branch conditions.
  enum BranchCondition
  {
    COND_ALWAYS = 0x0,
    COND_NEVER  = 0x1,
    COND_EQ     = 0x2,
    COND_NE     = 0x3,
    COND_LT     = 0xc,
    COND_LE     = 0xe,
    COND_GT     = 0xf,
    COND_GE     = 0xd,
    COND_LTU    = 0x8,
    COND_LEU    = 0xa,
    COND_GTU    = 0xb,
    COND_GEU    = 0x9
  };

public:

  // Currently, instructions are encoded in 32 bits.
  typedef uint32_t qisa_instruction_type;

  DllExport QISA_Driver();

  DllExport virtual
  ~QISA_Driver()
  {}

  // Handling the scanner.
  bool
  scanBegin (); // Note: implementation in qisa_lexer.l
  void
  scanEnd (); // Note: implementation in qisa_lexer.l

  DllExport void
  enableScannerTracing(bool enabled);

  /**
   * Run the parser on the given file.
   *
   * @param[in] filename File to parse.
   *
   * @return True on success, false on failure.
   */
  DllExport bool
  parse(const std::string& filename);

  /**
   * Disassemble the given file.
   *
   * @param filename
   * @return True on success, false on failure.
   */
  DllExport bool
  disassemble(const std::string& filename);

  /**
   * @return The last generated error message.
   */
  DllExport std::string
  getLastErrorMessage();

  /** @return The version of this assembler. */
  DllExport static std::string
  getVersion();


  DllExport void
  enableParserTracing(bool enabled);

  /**
   * Change the verbosity of the assembler.
   * This determines whether or not informational messages are shown while the assembler decodes its input
   * instructions.
   *
   * @param[in] verbose Specifies the verbosity of the assembler.
   */
  DllExport void
  setVerbose(bool verbose);

  /**
   * Retrieve the generated code as a list of strings that contain the hex values of the encoded
   * instructions.
   *
   * @return The generated instructions, one decoded instruction per line.
   */
  DllExport std::string
  getInstructionsAsHexStrings();

  /**
   * Retrieve the disassembled instructions as a multi-line string.
   * @return The disassembly output: one (or more, in case of quantum) disassembled instruction per line.
   */
  DllExport std::string
  getDisassemblyOutput();

  /**
   * Save binary assembled or textual disassembled instructions to the given output stream.
   *
   * @param[in] outputFileStream Already opened file stream opened in which to store the generated output.
   *
   * @return True on success, false on failure.
   */
  DllExport bool
  save(std::ofstream& outputFileStream);

  /**
   * Save binary assembled or textual disassembled instructions to an output file with the given name.
   *
   * @param[in] outputFileName Name of the file in which to store the generated output.
   *
   * @return True on success, false on failure.
   */
  DllExport bool
  save(const std::string& outputFileName);

  // Error reporting.
  void
  error(const QISA::location& l, const std::string& m);

  void
  error(const std::string& m);


  // Symbol table management functions

  // Add an integer symbol
  void
  add_symbol(const std::string& symbol_name,
             const QISA::location& symbol_name_loc,
             int64_t symbol_value,
             const QISA::location& symbol_value_loc);

  bool
  get_symbol(const std::string& symbol_name,
             const QISA::location& symbol_name_loc,
             int64_t& imm_val);

  // Add a string valued symbol (not used yet).
  void
  add_symbol(const std::string& symbol_name,
             const QISA::location& symbol_name_loc,
             const std::string&  symbol_value,
             const QISA::location& symbol_value_loc);


  bool
  get_symbol(const std::string& symbol_name,
             const QISA::location& symbol_name_loc,
             std::string& imm_val);


  // Add a register definition.
  // This is used to give a register a meaningful name.
  bool
  add_register_definition(const std::string& register_name,
                          const QISA::location& register_name_loc,
                          uint8_t reg_nr,
                          const QISA::location& reg_nr_loc,
                          RegisterKind register_kind);

  /**
   * Get the register index corresponding to a given alias.
   * @param register_name     Alias to be searched for.
   * @param register_name_loc Location in the source code that refers to this alias.
   * @param register_kind     Kind of register that is needed.
   * @param result            Resulting register index
   *
   * @return True on success, false on failure.
   */
  bool
  get_register_nr(const std::string& register_name,
                  const QISA::location& register_name_loc,
                  RegisterKind register_kind,
                  uint8_t& result);

  void
  add_label(const std::string& label_name,
            const QISA::location& label_name_loc);

  /**
   * Get the address (program counter) of a label, or an offset from the current program counter to that
   * address.
   *
   * @param[in] label_name     Name of the label being looked for.
   * @param[in] label_name_loc Location of that label within the input source file.
   * @param[in] get_offset     True if an offset to the current program counter should be returned, false if
   *                           the program counter associated with the requested label should be returned.
   *
   * @return Requested address or offset.
   *         If a label has not defined yet in the source file, std::numeric_limits<int64_t>::min() is
   *         returned to indicate that fact.
   */
  int64_t
  get_label_address(const std::string& label_name,
                    const QISA::location& label_name_loc,
                    bool get_offset);

  /**
   *
   * @param qubit_address Value of the qubit address.
   * @param loc Location of the given qubit address in the source file.
   * @return True if qubit_address is valid, false if it isn't.
   */
  bool
  validate_qubit_address(uint8_t qubit_address,
                        const QISA::location& loc);

  bool
  validate_s_mask(const std::vector<uint8_t>& s_mask,
                  const location& s_mask_loc);

  bool
  validate_t_mask(const std::vector<TargetControlPair>& t_mask,
                  const location& t_mask_loc);



  /**
   *
   * @param target_control_pair Values of the given target-control pair
   * @param target_control_pair_loc Location of the affected target-control pair in the source file.
   * @return True on success, false on failure.
   */
  bool
  validate_target_control_pair(const TargetControlPair& target_control_pair,
                               const location& target_control_pair_loc);

  /**
   * Validate a given bundle separator
   *
   * @param bs_val Bundle separator.
   *               This Specifies the time to wait (in number of quantum cycles) until
   *               the next instruction bundle is executed.
   *               The valid range is [0 - 7].
   * @param bs_loc Location of the bundle separator in the source file.
   *
   * @return True on success, false on failure.
   */
  bool
  validate_bundle_separator(uint8_t bs_val,
                            const location& bs_loc);

  /**
   * Parse a Quantum instruction that doesn't take any parameters.
   *
   * @param inst_name Name of the instruction.
   * @param inst_loc Location of the instruction in the source file.
   *
   * @return Shared pointer to a QInstruction, or empty pointer if an error occurred.
   */
  QInstructionPtr
  get_q_instr_arg_none(const std::string& inst_name,
                       const QISA::location& inst_loc);

  /**
   * Parse a quantum instruction that accepts an S register as a parameter.
   *
   * @param inst_name Name of the instruction.
   * @param inst_loc Location of the instruction in the source file.
   * @param st Indicates the register involved.
   * @param st_loc Location of the register specification in the source file.
   * @param is_conditional Denotes if this instruction is conditional or not.
   *
   * @return Shared pointer to a QInstruction, or empty pointer if an error occurred.
   */
  QInstructionPtr
  get_q_instr_arg_st(const std::string& inst_name,
                     const QISA::location& inst_loc,
                     uint8_t st,
                     const QISA::location& st_loc,
                     bool is_conditional);

  /**
   * Parse a quantum instruction that accepts either a T register as a parameter.
   *
   * @param inst_name Name of the instruction.
   * @param inst_loc Location of the instruction in the source file.
   * @param tt Indicates the register involved.
   * @param tt_loc Location of the register specification in the source file.
   *
   * @return Shared pointer to a QInstruction, or empty pointer if an error occurred.
   */
  QInstructionPtr
  get_q_instr_arg_tt(const std::string& inst_name,
                     const QISA::location& inst_loc,
                     uint8_t tt,
                     const QISA::location& tt_loc);

  /**
   * Parse a bundle of quantum instructions and generate the necessary instruction codes.
   *
   * @param bs_val Bundle separator.
   *               This Specifies the time to wait (in number of quantum cycles) until
   *               the next instruction bundle is executed. This has a maximum of
   *               7 quantum cycles.
   *               If it is zero, this bundle will be appended to the previous bundle.
   * @param bs_loc Location of the wait specification in the source file.
   * @param bundle List of quantum instructions that form a bundle.
   * @param bundle_loc Location of this list in the source file.
   *
   * @return True on success, false on failure.
   */
  bool generate_q_bundle(uint8_t bs_val,
                         const QISA::location& bs_loc,
                         const BundledQInstructions& bundle,
                         const QISA::location& bundle_loc);

  /**
   * Encode a given quantum instruction, given its opcode and parameters.
   * @param q_inst Contains the opcode and possible parameters.
   * @return Encoded instruction.
   */
  uint64_t
  encode_q_instr(const std::shared_ptr<QInstruction>& q_inst);


  /**
   * Decode a given quantum instruction, given its binary.
   * Return the disassembled instruction in q_inst_str.
   * @param q_inst Encoded quantum instruction.
   * @param[out] q_inst_str
   * @return True if the instruction was decoded correctly, false if not.
   */
  bool
  decode_q_instr(uint64_t q_inst, std::string& q_inst_str);


  // Assembly generation functions.
  bool
  generate_NOP(const QISA::location& inst_loc);

  bool
  generate_STOP(const QISA::location& inst_loc);

  bool
  generate_XXX_rd_rs_rt(const std::string& inst_name,
                        const QISA::location& inst_loc,
                        uint8_t rd,
                        const QISA::location& rd_loc,
                        uint8_t rs,
                        const QISA::location& rs_loc,
                        uint8_t rt,
                        const QISA::location& rt_loc);

  bool
  generate_NOT(const QISA::location& inst_loc,
               uint8_t rd,
               const QISA::location& rd_loc,
               uint8_t rt,
               const QISA::location& rt_loc);

  bool
  generate_CMP(const QISA::location& inst_loc,
               uint8_t rs,
               const QISA::location& rs_loc,
               uint8_t rt,
               const QISA::location& rt_loc);

  /**
   * Generate the branch instruction
   * @param inst_loc Location of the mnemonic.
   * @param cond     Condition on which to branch.
   * @param cond_loc Location of the condition specification.
   * @param addr     Offset from the current program counter to which to branch.
   * @param addr_loc Location of the offset specification.
   * @param is_alias If true, this branch was due to an alias, which means that we
   *                 have to correct for an additional implicit CMP instruction.
   *                 If false, don't apply this correction.
   * @return
   */
  bool
  generate_BR(const QISA::location& inst_loc,
              uint8_t cond,
              const QISA::location& cond_loc,
              int64_t addr,
              const QISA::location& addr_loc,
              bool is_alias = false);


  bool
  generate_LDI(const location& inst_loc,
               uint8_t rd,
               const location& rd_loc,
               int64_t imm,
               const location& imm_loc);

  bool
  generate_LDUI(const location& inst_loc,
                uint8_t rd,
                const location& rd_loc,
                int64_t imm,
                const location& imm_loc);

  bool
  generate_FBR(const location& inst_loc,
               uint8_t cond,
               const location& cond_loc,
               uint8_t rd,
               const location& rd_loc);

  bool
  generate_FMR(const location& inst_loc,
               uint8_t cond,
               const location& cond_loc,
               uint8_t qs,
               const location& qs_loc);


  bool
  generate_SMIS(const location& inst_loc,
                uint8_t sd,
                const location& sd_loc,
                const std::vector<uint8_t>& s_mask,
                const location& s_mask_loc);

  /* smis sd, imm  (NOTE: Alternative representation.) */
  bool
  generate_SMIS(const location& inst_loc,
                uint8_t sd,
                const location& sd_loc,
                int64_t imm,
                const location& imm_loc);


  bool
  generate_SMIT(const location& inst_loc,
                uint8_t td,
                const location& td_loc,
                const std::vector<TargetControlPair>& t_mask,
                const location& t_mask_loc);

  /* smis sd, imm  (NOTE: Alternative representation.) */
  bool
  generate_SMIT(const location& inst_loc,
                uint8_t td,
                const location& td_loc,
                int64_t imm,
                const location& imm_loc);



  bool
  generate_QWAIT(const location& inst_loc,
                 int64_t imm,
                 const location& imm_loc);

  bool
  generate_QWAITR(const QISA::location& inst_loc,
                  uint8_t rs,
                  const QISA::location& rs_loc);



  /***********
   * ALIASES *
   ***********/

  /* SHL1  rd, rs        */
  bool
  generate_SHL1(const QISA::location& inst_loc,
                uint8_t rd,
                const QISA::location& rd_loc,
                uint8_t rs,
                const QISA::location& rs_loc);

  /* NAND  rd, rs, rt    */
  bool
  generate_NAND(const QISA::location& inst_loc,
                uint8_t rd,
                const QISA::location& rd_loc,
                uint8_t rs,
                const QISA::location& rs_loc,
                uint8_t rt,
                const QISA::location& rt_loc);

  /* NOR   rd, rs, rt    */
  bool
  generate_NOR(const QISA::location& inst_loc,
               uint8_t rd,
               const QISA::location& rd_loc,
               uint8_t rs,
               const QISA::location& rs_loc,
               uint8_t rt,
               const QISA::location& rt_loc);

  /* XNOR  rd, rs, rt    */
  bool
  generate_XNOR(const QISA::location& inst_loc,
                uint8_t rd,
                const QISA::location& rd_loc,
                uint8_t rs,
                const QISA::location& rs_loc,
                uint8_t rt,
                const QISA::location& rt_loc);

  /* BRA   addr          */
  bool
  generate_BRA(const QISA::location& inst_loc,
               int64_t addr,
               const QISA::location& addr_loc);

  /* BRN   addr          */
  bool
  generate_BRN(const QISA::location& inst_loc,
               int64_t addr,
               const QISA::location& addr_loc);

  /* BEQ   rs, rt, addr  */
  /* BNE   rs, rt, addr  */
  /* BLT   rs, rt, addr  */
  /* BLE   rs, rt, addr  */
  /* BGT   rs, rt, addr  */
  /* BGE   rs, rt, addr  */
  /* BLTU  rs, rt, addr  */
  /* BLEU  rs, rt, addr  */
  /* BGTU  rs, rt, addr  */
  /* BGEU  rs, rt, addr  */
  bool
  generate_BR_COND(const QISA::location& inst_loc,
                   uint8_t rs,
                   const QISA::location& rs_loc,
                   uint8_t rt,
                   const QISA::location& rt_loc,
                   int64_t addr,
                   const QISA::location& addr_loc,
                   BranchCondition cond);


  /* MOV   rd, rs        */
  bool
  generate_MOV(const QISA::location& inst_loc,
               uint8_t rd,
               const QISA::location& rd_loc,
               uint8_t rs,
               const QISA::location& rs_loc);

  /* MULT2 rd, rs        */
  bool
  generate_MULT2(const QISA::location& inst_loc,
                 uint8_t rd,
                 const QISA::location& rd_loc,
                 uint8_t rs,
                 const QISA::location& rs_loc);



  //! Reset the state that is maintained between quantum bundle specifications.
  void
  reset_q_state();

  // The name of the file being parsed.
  // Used later to pass the file name to the location tracker.
  // NOTE: This is a public member due to the way that the bison parser wants to use it.
  std::string _filename;


  private: // -- functions


  /**
   * Lookup the last error location in the source file and return its contents.
   * @return The source line affected by the las error.
   */
  std::string
  getErrorSourceLine();


  /**
   * Used to set the opcodes of the instructions.
   * The code for this is defined in an automatically generated file.
   */
  void setOpcodes();

  /**
   * Get an opcode for a given instruction name.
   *
   * @param[in] instruction_name Name for which an opcode has to be found.
   * @param[in] instruction_name_loc Location of the given instruction in the input source file.
   * @param[in] opcode Returned opcode.
   *
   * @return True is opcode was found, false if not.
   */
  bool
  get_opcode(const std::string& instruction_name,
             const QISA::location& instruction_name_loc,
             int& opcode);

  /**
   * Assemble the LUI instructions given the previously checked parameters, and add it to the list of
   * instructions.
   * This function is used from within two separate 'generate_' instructions: generate_LUI and generate_MOV.
   *
   * @param[in] rt  Register number in which the 'imm' value must be stored.
   * @param[in] pos Part of the register that must be stored. 0 is most significant byte, 3 is least
   *                significant byte.
   * @param[in] imm Value to be stored.
   *
   */
  void
  generate_LUI_unchecked(uint8_t rt,
                         int64_t pos,
                         int64_t imm);


  // Used by the two flavors of generate_SMIS.
  // All arguments are supposed to have been checked.
  void
  unsafe_generate_SMIS(int opcode,
                       uint8_t sd,
                       int64_t s_mask_bits);

  // Used by the two flavors of generate_SMIT.
  // All arguments are supposed to have been checked.
  void
  unsafe_generate_SMIT(int opcode,
                       uint8_t td,
                       int64_t t_mask_bits);


  /**
   * Check if the given register number is acceptable to be used to adress a 'Q' register.
   * If not acceptable, the function issues an error message before returning false.
   *
   * @param[in] reg_nr Register number to check.
   * @param[in] register_nr_loc Location of the given register number in the input source file.
   *
   * @return True if acceptable, false if not.
   */
  bool
  checkQRegisterNumber(uint8_t reg_nr, const QISA::location& register_nr_loc);

  /**
   * Check if the given register number is acceptable to be used to adress a 'R' register.
   * If not acceptable, the function issues an error message before returning false.
   *
   * @param[in] reg_nr Register number to check.
   * @param[in] register_nr_loc Location of the given register number in the input source file.
   *
   * @return True if acceptable, false if not.
   */
  bool
  checkRRegisterNumber(uint8_t reg_nr, const QISA::location& register_nr_loc);

  /**
   * Check if the given register number is acceptable to be used to adress a 'S' register.
   * If not acceptable, the function issues an error message before returning false.
   *
   * @param[in] reg_nr Register number to check.
   * @param[in] register_nr_loc Location of the given register number in the input source file.
   *
   * @return True if acceptable, false if not.
   */
  bool
  checkSRegisterNumber(uint8_t reg_nr, const QISA::location& register_nr_loc);

  /**
   * Check if the given register number is acceptable to be used to adress a 'T' register.
   * If not acceptable, the function issues an error message before returning false.
   *
   * @param[in] reg_nr Register number to check.
   * @param[in] register_nr_loc Location of the given register number in the input source file.
   *
   * @return True if acceptable, false if not.
   */
  bool
  checkTRegisterNumber(uint8_t reg_nr, const QISA::location& register_nr_loc);

  /**
   * Check if the given register number is acceptable.
   * If not acceptable, the function issues an error message before returning false.
   *
   * @param[in] reg_nr Register number to check.
   * @param[in] register_nr_loc Location of the given register number in the input source file.
   * @param[in] register_kind Kind of registers to check against.
   *
   * @return True if acceptable, false if not.
   */
  bool
  checkRegisterNumber(uint8_t reg_nr, const QISA::location& register_nr_loc, RegisterKind register_kind);

  /**
   * Check if the given (signed) value is within the range [minVal,maxVal].
   * If it is not, the function issues an error message before returning false.
   *
   * @param[in] val Value to check.
   * @param[in] minVal Minimum acceptable value.
   * @param[in] maxVal Maximum acceptable value.
   * @param[in] val_name Name that corresponds to this value.
   * @param[in] val_loc Location of the given value in the input source file.
   *
   * @return True if the value lies within range, false if not.
   */
  bool
  checkValueRange(int64_t val, int64_t minVal, int64_t maxVal,
                  const std::string& val_name, const QISA::location& val_loc);

  std::string
  get_s_mask_str(const std::vector<uint8_t>& s_mask);

  /**
   * Return the s_mask corresponding to the given encoded value.
   * @param s_mask_bits Binary encoded s_mask.
   * @return The corresponding s_mask.
   */
  std::vector<uint8_t>
  bits2s_mask(int64_t s_mask_bits);


  /**
   * Return the t_mask corresponding to the given encoded value.
   * @param t_mask_bits Binary encoded s_mask.
   * @return The corresponding t_mask.
   */
  std::vector<TargetControlPair>
  bits2t_mask(int64_t t_mask_bits);



  std::string
  get_tc_pair_str(const TargetControlPair& tc_pair);

  std::string
  get_t_mask_str(const std::vector<TargetControlPair>& t_mask);

  /**
   * Process all instructions that used labels that were (supposed to be) defined afterwards.
   *
   * @return True on success, false on failure.
   */
  bool
  processDeferredInstructions();


  /**
   * Reverse the bits in the given src.
   *
   * @tparam SrcType  Type of 'src' and the result.
   *                  Only unsigned integral types are supported.
   * @tparam nrOfBits Width of 'src' in number of bits.
   *
   * @param[in] src Input bits to be reversed.
   *
   * @return Reversed bits
   *
   * @note
   * Based on code from:
   * http://graphics.stanford.edu/%7Eseander/bithacks.html#BitReverseObvious
   */
  template<typename SrcType, int nrOfBits>
  SrcType reverseBits(const SrcType src);

  /*****************************************************************/
  /* Comparator for case-insensitive comparison in STL associative */
  /* containers.                                                   */
  /* source: https://stackoverflow.com/a/1801913                   */
  /*****************************************************************/
  struct ci_less : std::binary_function<std::string, std::string, bool>
  {
    // case-independent (ci) compare_less binary function
    struct nocase_compare : public std::binary_function<unsigned char,unsigned char,bool>
    {
      bool operator() (const unsigned char& c1, const unsigned char& c2) const {
          return tolower (c1) < tolower (c2);
      }
    };
    bool operator() (const std::string & s1, const std::string & s2) const {
      return std::lexicographical_compare
        (s1.begin (), s1.end (),   // source range
        s2.begin (), s2.end (),   // dest range
        nocase_compare ());  // comparison
    }
  };


  std::string
  getHex(int val, int nDigits);

  bool
  disassembleInstruction(qisa_instruction_type inst);

  bool
  disassembleClassicInstruction(qisa_instruction_type inst);

  bool
  disassembleQuantumInstruction(qisa_instruction_type inst);

  /**
   * Post-process the disassembly steps to add labels.
   * By doing this after all branch destinations are known, we can issue labels
   * with steadily increasing numbers.
   * In this part we'll also add the instruction counter (as text).
   */
  void
  postProcessDisassembly();

  /**
   * Save binary assembled instructions to the given output stream.
   *
   * @param[in] outputFileStream Already opened file stream opened in which to store the generated output.
   *
   * @return True on success, false on failure.
   */
  bool
  saveAssembly(std::ofstream& outputFileStream);

  /**
   * Save binary assembled instructions to an output file with the given name.
   *
   * @param[in] outputFileName Name of the file in which to store the generated output.
   *
   * @return True on success, false on failure.
   */
  bool
  saveAssembly(const std::string& outputFileName);


  /**
   * Save textual disassembled instructions to the given output stream.
   *
   * @param[in] outputFileStream Already opened file stream opened in which to store the generated output.
   *
   * @return True on success, false on failure.
   */
  bool
  saveDisassembly(std::ofstream& outputFileStream);

  /**
   * Save textual disassembled instructions to an output file with the given name.
   *
   * @param[in] outputFileName Name of the file in which to store the generated output.
   *
   * @return True on success, false on failure.
   */
  bool
  saveDisassembly(const std::string& outputFileName);

private: // -- constants

  //---------------------------------------------------------
  // Classic instructions
  //---------------------------------------------------------

  // Defines various bit offsets into the instruction word.
  enum BitOffsets
  {
    OPCODE_OFFSET  = 25,

    RD_OFFSET      = 20,
    RS_OFFSET      = 15,
    RT_OFFSET      = 10,

    SD_OFFSET      = 20,
    TD_OFFSET      = 19,

    ADDR_OFFSET    = 4,

  };

  // Defines various widths of opcode and operands by specifying a '1' for every bit in use.
  enum OperandWidths
  {
    OPCODE_MASK    = 0x00003f, //  6 bits

    RS_MASK        = 0x00001f, //  5 bits
    RT_MASK        = 0x00001f, //  5 bits
    RD_MASK        = 0x00001f, //  5 bits

    ADDR_MASK      = 0x1fffff, // 21 bits
    COND_MASK      = 0x00000f, //  4 bits

    IMM20_MASK     = 0x0fffff, // 20 bits
    U_IMM15_MASK   = 0x007fff, // 15 bits

    QS_MASK        = 0x000007, //  3 bits

    SD_MASK        = 0x00001f, //  5 bits
    TD_MASK        = 0x00003f, //  6 bits

    S_MASK_MASK    = 0x00007f, //  7 bits
    T_MASK_MASK    = 0x00ffff, // 16 bits

    U_IMM20_MASK   = 0x0fffff  // 20 bits
  };


  //---------------------------------------------------------
  // Quantum instructions
  //---------------------------------------------------------

  // Defines various bit offsets into the double instruction
  // instruction VLIW.
  enum DblInstr_BitOffsets
  {
    VLIW_INST_0_OFFSET         = 3,
    VLIW_INST_1_OFFSET         = 17,

    Q_INST_ST_COND_OFFSET      = 5,
    Q_INST_OPCODE_OFFSET       = 6,
    DBL_INST_FORMAT_BIT_OFFSET = 31
  };

  // Defines various widths of opcode and operands by specifying a '1' for every bit in use.
  enum DblInstr_OperandWidths
  {
    VLIW_Q_INST_MASK           = 0x003fff, // Whole instruction, 14 bits
    BS_MASK                    = 0x000007, // Bundle separator, 3 bits
    Q_INST_OPCODE_MASK         = 0x0000ff, // 8 bits
    Q_INST_SD_MASK             = 0x00001f, // 5 bits
    Q_INST_TD_MASK             = 0x00003f, // 6 bits
  };


private: // -- variables
  // Whether lexer traces should be generated.
  bool _traceScanning;

  // Whether parser traces should be generated.
  bool _traceParsing;

  // Specifies the verbosity of the assembler.
  bool _verbose;

  // Total number of registers available in processor, per kind of register.
  int _nrOfRegisters[4];

  // 'Name' of a register, per kind of register
  char _registerName[4];

  // Total number of addressable qubits in the processor.
  int _totalNrOfQubits;

  // Maximum value to specify as bundle separator.
  // This is the number of quantum cycles (20 ns) between quantum instruction bundles.
  int _max_bs_val;

  // Contains a mapping of all valid control pairs to their
  // respective bit index in the t_mask.
  std::map<TargetControlPair, uint8_t> _valid_target_control_pairs;

  // Other way around, to go from bit number to tc_pair.
  std::map<uint8_t, TargetControlPair> _bit2tc_pair;

  // List of assembled instructions.
  std::vector<qisa_instruction_type> _instructions;

  // List of disassembled instructions.
  std::stringstream _disassemblyOutput;

  // Used while disassembling, to keep track of the current instruction within the input file.
  size_t _disassemblyInstructionCounter;

  // Used to check whether a bundle specification of 0 is legal or not.
  // This one is used while disassembling.
  bool _disassemblyStartedQuantumBundle;


  // Used to check whether a bundle specification of 0 is legal or not.
  // This one is used while assembling.
  bool _assemblyStartedQuantumBundle;


  // Used to keep track of which instruction should have a label,
  // according to branch destinations.
  // The map maps a branch destination to the branch instruction(s) that use it.
  std::map<uint64_t, std::vector<uint64_t> > _disassemblyLabels;

  // Prefix used to denote a label in the disassembly.
  // (This will be followed by a number.)
  static const char* DISASSEMBLY_LABEL_PREFIX;

  // Opcodes for the instructions.
  // They are defined elsewhere.
  static std::map<std::string, int> _opcodes;

  // Reverse lookup of the _opcodes map, for the classic instructions.
  static std::map<int, std::string> _classicOpcode2instName;

  // Reverse lookup of the _opcodes map, for the quantum instructions.
  static std::map<int, std::string> _quantumOpcode2instName;

  // The following two are used for disassembly of quantum instructions.

  // Contains the opcodes for the quantum instructions specifying an st argument.
  static std::set<int> _q_inst_arg_st;

  // Contains the opcodes for the quantum instructions specifying a tt argument.
  static std::set<int> _q_inst_arg_tt;

  // Label to 'address' map.
  // This 'address' is in instruction units, not in byte units.
  std::map<std::string, uint64_t, ci_less> _labels;

  // Aliases for registers, one map per kind of register.
  std::map<std::string, uint8_t, ci_less> _registerAliases[4];

  // Integer valued symbols.
  std::map<std::string, int64_t, ci_less> _intSymbols;

  // Symbols that represent strings.
  std::map<std::string, std::string, ci_less> _strSymbols;

  // Names of the known branch conditions.
  // Used for pretty printing.
  std::map<uint8_t, std::string> _branchConditionNames;

  // Structure to fill if a non-defined label is encountered.
  // It might be defined later on...
  struct DeferredLabelUse
  {
    // Kind of instruction it concerns.
    QISA::QISA_Parser::token_type instruction;

    // True if the label should be used as an offset, or as an actual program counter value.
    bool is_offset;

    // If true, this branch was due to an alias, which means that we have to correct for an
    // additional implicit CMP instruction.
    // If false, don't apply this correction.
    bool is_alias;

    // Name of the label that was being used.
    std::string label_name;

    // Location in the assembly input file.
    QISA::location label_name_loc;

    // Where the instruction resides in the 'program' (_instructions).
    uint64_t programCounter;
  };

  // Needed to be able to use a location in a std::map.
  struct LocationCompare {
    bool
    operator() (const QISA::location& lhs, const QISA::location& rhs) const
    {
      if (lhs.begin == rhs.begin)
      {
        return ((lhs.end.line < rhs.end.line) ||
                ((lhs.end.line == rhs.end.line) && (lhs.end.column < rhs.end.column)));
      }
      else
      {
        return ((lhs.begin.line < rhs.begin.line) ||
                ((lhs.begin.line == rhs.begin.line) && (lhs.begin.column < rhs.begin.column)));
      }
    }
  };

  // Used to resolve labels that are used before declaration.
  // NOTE: Currently only one label use per instruction is supported.
  //       The key is the location on which the label is used.
  std::map<location, DeferredLabelUse, LocationCompare> _deferredInstructions;

  // Used to redirect error messages to.
  std::ostringstream _errorStream;

  enum LastDriverAction {
    DRIVER_ACTION_NONE,
    DRIVER_ACTION_PARSE,
    DRIVER_ACTION_DISASSEMBLE
  };

  // Used to know which type of generated output must be saved to file if save() is called.
  LastDriverAction _lastDriverAction;

  // Records the location of the last error.
  location _errorLoc;
};

/**
 * Reverse the bits in the given src.
 *
 * @tparam SrcType  Type of 'src' and the result.
 *                  Only unsigned integral types are supported.
 * @tparam nrOfBits Width of 'src' in number of bits.
 *
 * @param[in] src Input bits to be reversed.
 *
 * @return Reversed bits
 *
 * @note
 * Based on code from:
 * http://graphics.stanford.edu/%7Eseander/bithacks.html#BitReverseObvious
 */
template<typename SrcType, int nrOfBits>
SrcType QISA_Driver::reverseBits(const SrcType src)
{
  SrcType srcBits = src & ((1LL << nrOfBits) - 1);
  SrcType revBits = src & 1; // Get (only) the LSB

  int nShiftsLeft = nrOfBits - 1; // Extra shift needed at end.

  for (srcBits >>= 1; srcBits; srcBits >>= 1)
  {
    revBits <<= 1;
    revBits |= srcBits & 1;
    nShiftsLeft--;
  }

  // Shift when src's highest bits are zero
  revBits <<= nShiftsLeft;

  return revBits;
}



} /* end namespace QISA */

