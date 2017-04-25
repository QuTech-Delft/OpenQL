/*
 * Author: Imran Ashraf
 */

#ifndef GLOBALS_H
#define GLOBALS_H


#if defined(_MSC_VER)
    //  Microsoft 
    #define EXPORT extern "C" __declspec(dllexport)
    #define IMPORT extern "C" __declspec(dllimport)
#elif defined(__GNUC__)
    //  GCC
    #define EXPORT extern "C"  __attribute__((visibility("default")))
    #define IMPORT extern "C"
#else
    //  do nothing and hope for the best?
    #define EXPORT
    #define IMPORT
    #pragma warning Unknown dynamic link import/export semantics.
#endif


#endif
