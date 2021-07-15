/** \file
 * API header for accessing the compiler's pass management logic.
 */

#pragma once

#include "ql/config.h"
#include "ql/pmgr/manager.h"
#include "ql/api/declarations.h"

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//         Docstrings in this file must manually be kept in sync with         //
//     compiler.i! This should be automated at some point, but isn't yet.     //
//============================================================================//

namespace ql {
namespace api {

/**
 * Wrapper for the compiler/pass manager.
 */
class Compiler {
private:
    friend class Platform;
    friend class Program;

    /**
     * The linked pass manager.
     */
    ql::pmgr::Ref pass_manager;

    /**
     * Constructor used internally to build a compiler object that belongs to
     * a platform.
     */
    explicit Compiler(const ql::pmgr::Ref &pass_manager);

public:

    /**
     * User-given name for this compiler.
     *
     * NOTE: not actually used for anything. It's only here for consistency with
     * the rest of the API objects.
     */
    std::string name;

    /**
     * Creates an empty compiler, with no specified architecture.
     */
    explicit Compiler(const std::string &name="");

    /**
     * Creates a compiler configuration from the given JSON file.
     */
    explicit Compiler(const std::string &name, const std::string &filename);

    /**
     * Creates a default compiler for the given platform.
     */
    explicit Compiler(const std::string &name, const Platform &platform);

    /**
     * Prints documentation for all available pass types, as well as the option
     * documentation for the passes.
     */
    void print_pass_types() const;

    /**
     * Returns documentation for all available pass types, as well as the option
     * documentation for the passes.
     */
    std::string dump_pass_types() const;

    /**
     * Prints the currently configured compilation strategy.
     */
    void print_strategy() const;

    /**
     * Returns the currently configured compilation strategy as a string.
     */
    std::string dump_strategy() const;

#ifdef QL_HIERARCHICAL_PASS_MANAGEMENT
    /**
     * Sets a pass option. Periods are used as hierarchy separators; the last
     * element will be the option name, and the preceding elements represent
     * pass instance names. Furthermore, wildcards may be used for the pass name
     * elements (asterisks for zero or more characters and a question mark for a
     * single character) to select multiple or all immediate sub-passes of that
     * group, and a double asterisk may be used for the element before the
     * option name to chain to set_option_recursively() instead. The return
     * value is the number of passes that were affected; passes are only
     * affected when they are selected by the option path AND have an option
     * with the specified name. If must_exist is set an exception will be thrown
     * if none of the passes were affected, otherwise 0 will be returned.
     */
#else
    /**
     * Sets a pass option. The path must consist of the target pass instance name
     * and the option name, separated by a period. The former may include * or ?
     * wildcards. If must_exist is set an exception will be thrown if none of the
     * passes were affected, otherwise 0 will be returned.
     */
#endif
    size_t set_option(
        const std::string &path,
        const std::string &value,
        bool must_exist = true
    );

#ifdef QL_HIERARCHICAL_PASS_MANAGEMENT
    /**
     * Sets an option for all passes recursively. The return value is the number
     * of passes that were affected; passes are only affected when they have an
     * option with the specified name. If must_exist is set an exception will be
     * thrown if none of the passes were affected, otherwise 0 will be returned.
     */
    size_t set_option_recursively(
        const std::string &option,
        const std::string &value,
        bool must_exist = true
    );
#endif

#ifdef QL_HIERARCHICAL_PASS_MANAGEMENT
    /**
     * Returns the current value of an option. Periods are used as hierarchy
     * separators; the last element will be the option name, and the preceding
     * elements represent pass instance names.
     */
#else
    /**
     * Returns the current value of an option. path must consist of the pass
     * instance name and the option name separated by a period.
     */
#endif
    std::string get_option(const std::string &path) const;

#ifdef QL_HIERARCHICAL_PASS_MANAGEMENT
    /**
     * Appends a pass to the end of the pass list. If type_name is empty
     * or unspecified, a generic subgroup is added. Returns a reference to the
     * constructed pass.
     */
    Pass append_pass(
        const std::string &type_name = "",
        const std::string &instance_name = "",
        const std::map<std::string, std::string> &options = {}
    );
#else
    /**
     * Appends a pass to the end of the pass list. Returns a reference to the
     * constructed pass.
     */
    Pass append_pass(
        const std::string &type_name,
        const std::string &instance_name = "",
        const std::map<std::string, std::string> &options = {}
    );
#endif

#ifdef QL_HIERARCHICAL_PASS_MANAGEMENT
    /**
     * Appends a pass to the beginning of the pass list. If type_name is empty
     * or unspecified, a generic subgroup is added. Returns a reference to the
     * constructed pass.
     */
    Pass prefix_pass(
        const std::string &type_name = "",
        const std::string &instance_name = "",
        const std::map<std::string, std::string> &options = {}
    );
#else
    /**
     * Appends a pass to the beginning of the pass list. Returns a reference to the
     * constructed pass.
     */
    Pass prefix_pass(
        const std::string &type_name,
        const std::string &instance_name = "",
        const std::map<std::string, std::string> &options = {}
    );
#endif

#ifdef QL_HIERARCHICAL_PASS_MANAGEMENT
    /**
     * Inserts a pass immediately after the target pass (named by instance). If
     * target does not exist, an exception is thrown. If type_name is empty or
     * unspecified, a generic subgroup is added. Returns a reference to the
     * constructed pass. Periods may be used in target to traverse deeper into
     * the pass hierarchy.
     */
    Pass insert_pass_after(
        const std::string &target,
        const std::string &type_name = "",
        const std::string &instance_name = "",
        const std::map<std::string, std::string> &options = {}
    );
#else
    /**
     * Inserts a pass immediately after the target pass (named by instance). If
     * target does not exist, an exception is thrown. Returns a reference to the
     * constructed pass.
     */
    Pass insert_pass_after(
        const std::string &target,
        const std::string &type_name,
        const std::string &instance_name = "",
        const std::map<std::string, std::string> &options = {}
    );
#endif

#ifdef QL_HIERARCHICAL_PASS_MANAGEMENT
    /**
     * Inserts a pass immediately before the target pass (named by instance). If
     * target does not exist, an exception is thrown. If type_name is empty or
     * unspecified, a generic subgroup is added. Returns a reference to the
     * constructed pass. Periods may be used in target to traverse deeper into
     * the pass hierarchy.
     */
    Pass insert_pass_before(
        const std::string &target,
        const std::string &type_name = "",
        const std::string &instance_name = "",
        const std::map<std::string, std::string> &options = {}
    );
#else
    /**
     * Inserts a pass immediately before the target pass (named by instance). If
     * target does not exist, an exception is thrown. Returns a reference to the
     * constructed pass.
     */
    Pass insert_pass_before(
        const std::string &target,
        const std::string &type_name,
        const std::string &instance_name = "",
        const std::map<std::string, std::string> &options = {}
    );
#endif

#ifdef QL_HIERARCHICAL_PASS_MANAGEMENT
    /**
     * Looks for the pass with the target instance name, and embeds it into a
     * newly generated group. The group will assume the name of the original
     * pass, while the original pass will be renamed as specified by sub_name.
     * Note that this ultimately does not modify the pass order. If target does
     * not exist or this pass is not a group of sub-passes, an exception is
     * thrown. Returns a reference to the constructed group. Periods may be used
     * in target to traverse deeper into the pass hierarchy.
     */
    Pass group_pass(
        const std::string &target,
        const std::string &sub_name = "main"
    );
#endif

#ifdef QL_HIERARCHICAL_PASS_MANAGEMENT
    /**
     * Like group_pass(), but groups an inclusive range of passes into a
     * group with the given name, leaving the original pass names unchanged.
     * Periods may be used in from/to to traverse deeper into the pass
     * hierarchy, but the hierarchy prefix must be the same for from and to.
     */
    Pass group_passes(
        const std::string &from,
        const std::string &to,
        const std::string &group_name
    );
#endif

#ifdef QL_HIERARCHICAL_PASS_MANAGEMENT
    /**
     * Looks for an unconditional pass group with the target instance name and
     * flattens its contained passes into its parent group. The names of the
     * passes found in the collapsed group are prefixed with name_prefix before
     * they are added to the parent group. Note that this ultimately does not
     * modify the pass order. If the target instance name does not exist or is
     * not an unconditional group, an exception is thrown. Periods may be used
     * in target to traverse deeper into the pass hierarchy.
     */
    void flatten_subgroup(
        const std::string &target,
        const std::string &name_prefix = ""
    );
#endif

#ifdef QL_HIERARCHICAL_PASS_MANAGEMENT
    /**
     * Returns a reference to the pass with the given instance name. If no such
     * pass exists, an exception is thrown. Periods may be used as hierarchy
     * separators to get nested sub-passes.
     */
#else
    /**
     * Returns a reference to the pass with the given instance name. If no such
     * pass exists, an exception is thrown.
     */
#endif
    Pass get_pass(const std::string &target) const;

#ifdef QL_HIERARCHICAL_PASS_MANAGEMENT
    /**
     * Returns whether a pass with the target instance name exists. Periods may
     * be used in target to traverse deeper into the pass hierarchy.
     */
#else
    /**
     * Returns whether a pass with the target instance name exists.
     */
#endif
    bool does_pass_exist(const std::string &target) const;

    /**
     * Returns the total number of passes in the root hierarchy.
     */
    size_t get_num_passes() const;

    /**
     * Returns a vector with references to all passes in the root hierarchy.
     */
    std::vector<Pass> get_passes() const;

    /**
     * Returns an indexable list of references to all passes with the given
     * type within the root hierarchy.
     */
    std::vector<Pass> get_passes_by_type(const std::string &target) const;

    /**
     * Removes the pass with the given target instance name, or throws an
     * exception if no such pass exists.
     */
    void remove_pass(const std::string &target);

    /**
     * Clears the entire pass list.
     */
    void clear_passes();

#ifdef QL_HIERARCHICAL_PASS_MANAGEMENT
    /**
     * Constructs all passes recursively. This freezes the pass options, but
     * allows subtrees to be modified.
     */
    void construct();
#endif

    /**
     * Ensures that all passes have been constructed, and then runs the passes
     * on the given program. This is the same as Program.compile() when the
     * program is referencing the same compiler.
     */
    void compile(const Program &program);

    /**
     * Ensures that all passes have been constructed, and then runs the passes
     * without specification of an input program. The first pass should then act
     * as a language frontend. The cQASM reader satisfies this requirement, for
     * instance.
     */
    void compile_with_frontend(const Platform &platform);

};

} // namespace api
} // namespace ql
