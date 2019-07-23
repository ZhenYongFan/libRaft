#pragma once

#if defined _WIN32 || defined __CYGWIN__
    #define LIBRAFTCORE_DLL_IMPORT __declspec(dllimport)
    #define LIBRAFTCORE_DLL_EXPORT __declspec(dllexport)
    #define LIBRAFTCORE_DLL_LOCAL
#else
    #if __GNUC__ >= 4
        #define LIBRAFTCORE_DLL_IMPORT __attribute__ ((visibility ("default")))
        #define LIBRAFTCORE_DLL_EXPORT __attribute__ ((visibility ("default")))
        #define LIBRAFTCORE_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
    #else
        #define LIBRAFTCORE_DLL_IMPORT
        #define LIBRAFTCORE_DLL_EXPORT
        #define LIBRAFTCORE_DLL_LOCAL
    #endif
#endif

// Now we use the generic helper definitions above to define LIBRAFTCORE_API and LIBRAFT_LOCAL.
// LIBRAFTCORE_API is used for the public API symbols. It either DLL imports or DLL exports (or does nothing for static build)
// LIBRAFTCORE_LOCAL is used for non-api symbols.

#ifdef LIBRAFTCORE_DLL
    #ifdef LIBRAFTCORE_EXPORTS // defined if we are building the LIBRAFT DLL (instead of using it)
        #define LIBRAFTCORE_API LIBRAFTCORE_DLL_EXPORT
    #else
        #define LIBRAFTCORE_API LIBRAFTCORE_DLL_IMPORT
    #endif
    #define LIBRAFTCORE_LOCAL LIBRAFTCORE_DLL_LOCAL
#else // LIBRAFTCORE_DLL is not defined: this means BtreeIndex is a static lib.
    #define LIBRAFTCORE_API
    #define LIBRAFTCORE_LOCAL
#endif // LIBRAFTCORE_DLL

#ifndef BEGIN_C_DECLS
    #ifdef __cplusplus
        #define BEGIN_C_DECLS extern "C" {
        #define END_C_DECLS   }
    #else /* !__cplusplus */
        #define BEGIN_C_DECLS
        #define END_C_DECLS
    #endif /* __cplusplus */
#endif
