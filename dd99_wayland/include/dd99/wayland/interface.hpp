#pragma once


#include <bits/types/struct_iovec.h>
#include <concepts>
#include <cstdint>
#include <dd99/wayland/config.hpp>
#include <dd99/wayland/engine.hpp>
#include <format>
#include <source_location>
#include <stdexcept>


namespace dd99::wayland
{
    namespace proto
    {

        struct fixed_point
        {
            constexpr explicit fixed_point(double v) : m_value{static_cast<std::int32_t>(v * 256.0)} {}
            constexpr explicit operator double() { return to_double(); }
            constexpr double to_double() { return m_value / 256.0; }

        private:
            std::int32_t m_value;
        };

        // base class for all protocol-defined interfaces
        // objects are instances of derived classes
        struct interface
        {
        protected: // types
            using engine_t = engine<>;
            // friend engine_t;
            friend dd99::wayland::detail::engine_impl;
            using object_id_t = engine_t::object_id_t;
            using version_t = engine_t::version_t;
            using opcode_t = engine_t::opcode_t;


        protected: // member variables
            engine_t::engine_accessor m_engine;
            std::uint32_t m_object_id = 0;


        public: // destruction
            virtual ~interface() { m_engine.destroy_interface(*this); };


        protected: // construction (only by engine)
            interface(engine_t::engine_accessor engine_accessor)
                : m_engine{engine_accessor}
            { }

        
        // public:
        //     auto get_id() const { return m_object_id; }


        protected:
            // static data annexed to each interface
            struct static_data_t
            {
                version_t server_supported_version = 0;
            };

        
        protected: // functions exposed to derived classes
            template <class ... Args>
            void send_wayland_message(opcode_t opcode, Args && ... args);


        protected:
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


        template <class T> concept Interface_C = std::derived_from<T, interface>;

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
