#pragma once


#include <dd99/wayland/config.hpp>
#include <dd99/wayland/engine.hpp>
#include <dd99/wayland/interface_concept.hpp>
#include <dd99/wayland/message_marshaling.hpp>
#include <dd99/wayland/message_parsing.hpp> // used by interfaces that include this file
#include <dd99/wayland/types.hpp>

// #include <bits/types/struct_iovec.h>

#include <cstdint>
#include <format>
#include <source_location>
#include <stdexcept>



namespace dd99::wayland::proto
{

    // // fw declaration
    // template <class T, class Id_T, Id_T Base>
    // struct object_map;



    // base class for all protocol-defined interfaces
    // objects are instances of derived classes
    struct interface
    {
    protected: // types
        friend dd99::wayland::engine;
        // using engine_t = engine<>;
        // // friend engine_t;
        // friend dd99::wayland::detail::engine_impl;
        // using object_id_t = engine_t::object_id_t;
        // using version_t = engine_t::version_t;
        // using opcode_t = engine_t::opcode_t;


    protected: // member variables
        // engine_t::engine_accessor m_engine;
        engine & m_engine;
        std::uint32_t m_object_id = 0;


    protected:

        interface(engine & eng)
            : m_engine{eng}
        { }


    public: // destructor

        virtual ~interface() = default;
    
    
    protected:
        // static data annexed to each interface class
        struct static_data_t
        {
            // TODO: server supported version should be set to 0 until server reports information about it.
            // However, server is assumed to support basic wayland protocol interfaces
            // This value temporarily set to 1 to avoid version check failures
            version_t server_supported_version = 1;
        };

    
    protected: // functions exposed to derived classes
        template <class ... Args>
        void send_wayland_message(opcode_t opcode, std::span<int> ancillary_fds, Args && ... args);


    protected: // functions that derived classes must implement
        virtual void parse_and_dispatch_event(std::span<const char> data) = 0;


    protected: // for debugging
        constexpr void dbg_check_version(version_t minimum_required_version,
                                            version_t server_supported_version,
                                            std::string_view interface_name,
                                            const std::source_location& location = std::source_location::current()) 
        {
            if constexpr (is_debug_enabled())
            {
                if (server_supported_version < minimum_required_version)
                    throw std::runtime_error{std::format(
                            "[DD99_WAYLAND] In file {}:{}:{}: In function {}: ERROR: Version check failed. Server supported version: {}. Minimum required version: {}."
                        , location.file_name()
                        , location.line()
                        , location.column()
                        , location.function_name()
                        , server_supported_version
                        , minimum_required_version
                        , interface_name)};
            }
        }
    };



    template <class ... Args>
    void interface::send_wayland_message(opcode_t opcode, std::span<int> fds, Args && ... args)
    {
        auto interfaces_to_ids = []<class T>(T && t)
        {
            // incomplete types are treated as interfaces
            if constexpr (!requires{sizeof(std::remove_cvref_t<T>);}) return reinterpret_cast<const interface &>(t).m_object_id;
            else if constexpr (Interface_C<std::remove_cvref_t<T>>) return reinterpret_cast<const interface &>(t).m_object_id;
            else return std::forward<T>(t);
        };

        detail::message_marshal(m_engine, m_object_id, opcode, fds, interfaces_to_ids.template operator()<Args>(args) ...);
    }

}



// 
// NOTES on marshalling:
// 
// there's a global registry object
// We have data about interfaces supported by server
// 



// una "interface" es el conjunto de funciones, con sus versiones (compile-time data)
// el servidor soporta un conjunto de interfaces con versiones especificas (run-time data)
// un objeto tiene id e interface


// cada "interface" `interface_x` se representa por una clase derivada de cierta base
// los datos son: los prototipos de funciones y los datos `static` de la clase

// `interface_x` deriva de `interface` (referred as `base` here)
// 
