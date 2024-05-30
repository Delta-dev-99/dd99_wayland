#pragma once


#include <dd99/wayland/types.hpp>
#include <dd99/wayland/config.hpp>
#include <dd99/wayland/engine.hpp>

// #include <bits/types/struct_iovec.h>

#include <concepts>
#include <cstdint>
#include <format>
#include <source_location>
#include <stdexcept>



namespace dd99::wayland::proto
{

    // fw declaration
    struct interface;

    template <class T, class Id_T, Id_T Base>
    struct object_map;


    // concept of interface: derives from dd99::wayland::proto::interface
    template <class T> concept Interface_C = std::derived_from<T, interface>;



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
            version_t server_supported_version = 0;
        };

    
    protected: // functions exposed to derived classes
        template <class ... Args>
        void send_wayland_message(opcode_t opcode, Args && ... args);


    protected: // functions that derived classes must implement
        virtual void parse_and_dispatch_event(std::span<char> data) = 0;


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
