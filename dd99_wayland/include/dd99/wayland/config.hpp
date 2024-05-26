#pragma once


#if defined(__cplusplus)
    #define DD99_WAYLAND_EXPORT [[gnu::visibility("default")]]
    #define DD99_WAYLAND_HIDDEN [[gnu::visibility("hidden")]]
    #define DD99_WAYLAND_DEPRECATED [[deprecated]]
#elif defined(__GNUC__) && __GNUC__ >= 4
    #define DD99_WAYLAND_EXPORT __attribute__((visibility("default")))
    #define DD99_WAYLAND_HIDDEN __attribute__((visibility("hidden")))
    #define DD99_WAYLAND_DEPRECATED __attribute__((deprecated))
#else
    #define DD99_WAYLAND_EXPORT
    #define DD99_WAYLAND_HIDDEN
    #define DD99_WAYLAND_DEPRECATED
#endif

#if defined(__cplusplus)
namespace dd99::wayland
{
    consteval auto is_debug_enabled()
    {
#if not defined(NDEBUG) or defined(DD99_WAYLAND_DEBUG)
        return true;
#else
        return false;
#endif
    }
}
#endif
