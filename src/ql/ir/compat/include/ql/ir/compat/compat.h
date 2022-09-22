/** \file
 * Utility file for including all old IR classes. These classes are only
 * preserved for API-level backward-compatibility; the structure is converted
 * to the tree-gen-based IR at the very start of program/compiler.compile().
 */

#pragma once

#include "ql/ir/compat/gate.h"
#include "ql/ir/compat/platform.h"
#include "ql/ir/compat/classical.h"
#include "ql/ir/compat/bundle.h"
#include "ql/ir/compat/kernel.h"
#include "ql/ir/compat/program.h"
