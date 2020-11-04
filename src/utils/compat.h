#pragma once

#ifdef _MSC_VER
#ifdef BUILDING_OPENQL
#define OPENQL_DECLSPEC __declspec(dllexport)
#else
#define OPENQL_DECLSPEC __declspec(dllimport)
#endif
#else
#define OPENQL_DECLSPEC
#endif
