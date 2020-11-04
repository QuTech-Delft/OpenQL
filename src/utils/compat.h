/** \file
 * Contains utilities for cross-platform compatibility.
 */

#pragma once

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
 * This nonsense is unfortunately necessary for Windows library support.
 */
#ifdef _MSC_VER
#ifdef BUILDING_OPENQL
#define QL_GLOBAL __declspec(dllexport)
#else
#define QL_GLOBAL __declspec(dllimport)
#endif
#else
#define QL_GLOBAL
#endif
