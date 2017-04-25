/*
 * Author: Imran Ashraf
 */

#ifndef API_H
#define API_H

#include "globals.h"

#ifdef __cplusplus
extern "C" {
#endif

EXPORT int init();
EXPORT int schedule();
EXPORT int compile();

#ifdef __cplusplus
}
#endif

#endif