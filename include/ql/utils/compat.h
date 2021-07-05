/** \file
 * Contains utilities for cross-platform compatibility.
 */

#pragma once

#include "ql/config.h"

/**
 * Prefix for global variable declarations.
 *
 * For public globals (declared in a header file), use it like this in the
 * header file:
 *
 *     QL_GLOBAL extern <type> <name>;
 *
 * The definition in the accompanying CC file doesn't need the prefix in this
 * case. Private globals that are *only* defined in a CC file do however need
 * the prefix, still:
 *
 *     QL_GLOBAL <type> <name>;
 *
 * As for the definition itself... there are three options:
 *
 *  - if OpenQL is linked dynamically (*.dll) and OpenQL is being built, the
 *    dllexport prefix is needed;
 *  - if OpenQL is linked dynamically (*.dll) and something depending on it is
 *    being built, the dllimport prefix is needed; and
 *  - if OpenQL is linked statically (*.lib), there must be no prefix in either
 *    case.
 *
 * All of these are contextual in a way that cannot normally be detected from
 * within the C preprocessor; we need to know if and how OpenQL is/was built
 * from within a public header file. Whether it's being built now or not is
 * determined using BUILDING_OPENQL, which is defined on the command line only
 * when OpenQL itself is being compiled. Whether it is being built or was built
 * as a static or dynamic library can however only be done using the generated
 * config.h file.
 *
 * This nonsense is unfortunately necessary for Windows library support.
 */
#ifdef _MSC_VER
#ifndef QL_SHARED_LIB
#define QL_GLOBAL
#else
#ifdef BUILDING_OPENQL
#define QL_GLOBAL __declspec(dllexport)
#define QL_GLOBAL
#else
#define QL_GLOBAL __declspec(dllimport)
#endif
#endif
#else
#define QL_GLOBAL
#endif
