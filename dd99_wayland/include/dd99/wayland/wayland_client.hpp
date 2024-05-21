#pragma once


// The goal of this library is to offer full Wayland support as cleanly as possible, while giving users full freedom.
// 
// Wayland protocols:
//  For core wayland and extension protocol support, you may use wayland-scanner with the standard protocol .xml files.
//  This requires incorporating an extra step to the build system.
//  Refer to Wayland documentation for details. (this page may be useful? https://wayland-book.com).
// 
// I/O:
//  This library does not attempt to do input/output, but rather give users the freedom to do it any way they please.
// 
// Multithreading:
//  This library is expected to be used on multithreaded environments. However, no thread safety provisions are offered.
//  Concurrent ussage of objects is not safe, unless explicitly stated otherwise.
//  The user is expected to provide thread safety when needed.
// 
// NOTE:
//  Thread safe alternatives and a simple main loop are expected to be implemented.
//  In some cases this will be done with wrappers or in a separate library.
//  These features are not a design goal, but may be offered to allow easier ussage of the library.


#include <dd99/wayland/engine.hpp>

#include <cstdint>
// #include <type_traits>
// #include <utility>
#include <vector>
#include <string>
#include <string_view>



namespace dd99::wayland
{

    namespace detail
    {

        struct message
        {
            std::string_view name;
            std::string_view signature;
        };

        struct interface
        {
            std::string name;
            int version;
            std::vector<message> methods;
            std::vector<message> events;
        };



        struct protocol_error
        {
            uint32_t code;
            // interface
            // object id
        };

    }



    

}
