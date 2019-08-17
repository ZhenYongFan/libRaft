#pragma once

#if defined _WIN32 || defined __CYGWIN__
    #define LIBRAFTEXT_DLL_IMPORT __declspec(dllimport)
    #define LIBRAFTEXT_DLL_EXPORT __declspec(dllexport)
    #define LIBRAFTEXT_DLL_LOCAL
#else
    #if __GNUC__ >= 4
        #define LIBRAFTEXT_DLL_IMPORT __attribute__ ((visibility ("default")))
        #define LIBRAFTEXT_DLL_EXPORT __attribute__ ((visibility ("default")))
        #define LIBRAFTEXT_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
    #else
        #define LIBRAFTEXT_DLL_IMPORT
        #define LIBRAFTEXT_DLL_EXPORT
        #define LIBRAFTEXT_DLL_LOCAL
    #endif
#endif

// Now we use the generic helper definitions above to define LIBRAFTEXT_API and LIBRAFT_LOCAL.
// LIBRAFTEXT_API is used for the public API symbols. It either DLL imports or DLL exports (or does nothing for static build)
// LIBRAFTEXT_LOCAL is used for non-api symbols.

#ifdef LIBRAFTEXT_DLL
    #ifdef LIBRAFTEXT_EXPORTS // defined if we are building the LIBRAFT DLL (instead of using it)
        #define LIBRAFTEXT_API LIBRAFTEXT_DLL_EXPORT
    #else
        #define LIBRAFTEXT_API LIBRAFTEXT_DLL_IMPORT
    #endif
    #define LIBRAFTEXT_LOCAL LIBRAFTEXT_DLL_LOCAL
#else // LIBRAFTEXT_DLL is not defined: this means BtreeIndex is a static lib.
    #define LIBRAFTEXT_API
    #define LIBRAFTEXT_LOCAL
#endif // LIBRAFTEXT_DLL

#ifndef BEGIN_C_DECLS
    #ifdef __cplusplus
        #define BEGIN_C_DECLS extern "C" {
        #define END_C_DECLS   }
    #else /* !__cplusplus */
        #define BEGIN_C_DECLS
        #define END_C_DECLS
    #endif /* __cplusplus */
#endif
