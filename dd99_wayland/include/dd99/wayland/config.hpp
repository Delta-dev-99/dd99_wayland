#pragma once


#if defined(__cplusplus)
    #define TWC_EXPORT [[gnu::visibility("default")]]
#elif defined(__GNUC__) && __GNUC__ >= 4
    #define TWC_EXPORT __attribute__((visibility("default")))
#else
    #define TWC_EXPORT
#endif

#if defined(__cplusplus)
    #define TWC_EXPORT [[deprecated]]
#elif defined(__GNUC__) && __GNUC__ >= 4
    #define TWC_EXPORT __attribute__((deprecated))
#else
    #define TWC_EXPORT
#endif
