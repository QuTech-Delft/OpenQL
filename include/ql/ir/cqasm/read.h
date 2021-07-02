/** \file
 * cQASM 1.2 reader logic as human-readable complement of the IR.
 */

#pragma once

#include "ql/utils/str.h"
#include "ql/ir/ir.h"

namespace ql {
namespace ir {
namespace cqasm {

/**
 * Defines how scheduling information in the incoming cQASM file is interpreted.
 */
enum class ScheduleMode {

    /**
     * The scheduling information (as encoded via bundles and skip instructions)
     * is retained.
     */
    KEEP,

    /**
     * The scheduling implications of bundles and skip instructions are
     * completely ignored. Instead, instructions are just (arbitrarily) given
     * sequential cycle numbers.
     */
    DISCARD,

    /**
     * As discard, but bundles are instead used as a shorthand notation for
     * blocks of code that have barriers around them, acting on exactly those
     * objects used within the block.
     */
    BUNDLES_AS_BARRIERS

};

/**
 * Options for reading cQASM files.
 */
struct ReadOptions {

    /**
     * The way in which the schedule/timing information in the cQASM file is
     * interpreted.
     */
    ScheduleMode schedule_mode;

};

/**
 * Reads a cQASM 1.2 file into the IR. If reading is successful, ir->program is
 * completely replaced. data represents the cQASM file contents, fname specifies
 * the filename if one exists for the purpose of generating better error
 * messages.
 */
void read(
    const Ref &ir,
    const utils::Str &data,
    const utils::Str &fname = "<unknown>",
    const ReadOptions &options = {}
);

/**
 * Same as read(), but given a file to load, rather than loading from a string.
 */
void read_file(
    const Ref &ir,
    const utils::Str &fname,
    const ReadOptions &options = {}
);

} // namespace cqasm
} // namespace ir
} // namespace ql
