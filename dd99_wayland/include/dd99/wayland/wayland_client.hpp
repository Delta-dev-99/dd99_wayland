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


#include <cstdint>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <string>
#include <string_view>
#include <span>



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



    // Core of wayland interaction
    // This class acts as an engine for (de)marshaling and dispatching of wayland protocol data
    // This class does not directly represent the wl_display object protocol
    template <class OutCB>
    struct display
    {
        static_assert(std::is_invocable_v<OutCB, std::span<unsigned char>, std::span<int>>, "OutCB signature is incorrect!");

    public: // input and output API

        // if this constructor is used, the user MUST set the callbacks manually
        // before calling any i/o member function.
        display() {}

        // create and set callback. Useful for use with lambdas
        display(OutCB && cb)
            : m_output_cb{std::move(cb)}
        {

        }

        display(const display &) = delete;
        display(display&&) = default;

        // The user is expected to manually read data from the wayland server connection.
        // When data arrives from the server, use this function to give it to the display.
        // This function dispatches the received events to the apropiate callbacks.
        // 
        // TODO: consider partial data frames. This requires buffering.
        // TODO: consider returning some state code.
        void put_input(std::span<unsigned char> data);

        // The user is expected to manually write data to the wayland server connection.
        // When there is data to be sent to the wayland server, this callback gets called.
        // Calling this function is always the result of interacting with wayland objects in this library.
        // 
        // Signature:
        //  `data` is just binary data to be sent to the server
        //  `fds` is a collection of file descriptors to be sent as ancillary data
        // 
        // About buffering:
        // This library does not provide output buffering.
        // The user may choose to buffer this data before sending to the wayland server.
        // Note, however, that the server might expect timely responses to some events.
        using Output_Callback_Signature = void(std::span<unsigned char> data, std::span<int> fds);
        void set_output_callback(Output_Callback_Signature * cb) { m_output_cb = cb; }


        int get_error();
        uint32_t get_protocol_error();


    private:
        OutCB m_output_cb{};

        // file ids allow the interpretation of data based on which source it came from
        // any unique value per file can work, but we just use the fd to simplify stuff
        enum class input_role;
        std::unordered_map<int, input_role> file_id_map{};
    };

}
