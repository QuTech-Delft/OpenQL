#define QISA_MAJOR_VERSION 0

#define QISA_MINOR_VERSION 2

#define QISA_PATCH_VERSION 0

// Make it easier to check for QISA version dependencies.
// This assumes the PATCH and MINOR version will not exceed 99
#define QISA_FULL_VERSION (QISA_MAJOR_VERSION * 10000 + QISA_MINOR_VERSION * 100 + QISA_PATCH_VERSION)

#define QISA_VERSION_STRING_S1(arg) #arg
#define QISA_VERSION_STRING_S(arg) QISA_VERSION_STRING_S1(arg)

#define QISA_VERSION_STRING (QISA_VERSION_STRING_S(QISA_MAJOR_VERSION) "." \
                             QISA_VERSION_STRING_S(QISA_MINOR_VERSION) "." \
                             QISA_VERSION_STRING_S(QISA_PATCH_VERSION))
