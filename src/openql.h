/**
 * @file   openql.h
 * @date   11/2016
 * @author Nader Khammassi
 * @brief  main openql header
 */
#ifndef OPENQL_H
#define OPENQL_H

#if defined(WIN32)
#define __attribute__(A) /* do nothing on windows */
#endif

#include "compile_options.h"
#include "instruction_map.h"
#include "optimizer.h"
#include "circuit.h"
#include "program.h"
#include "compiler.h"
#include "cqasm/cqasm_reader.h"

namespace ql
{
    // FIXME: nothing remains after cleanup
}

#endif // OPENQL_H
