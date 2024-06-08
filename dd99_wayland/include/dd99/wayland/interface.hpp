#pragma once


#include <dd99/wayland/config.hpp>
#include <dd99/wayland/engine.hpp>
#include <dd99/wayland/interface_binder.hpp>
#include <dd99/wayland/interface_concept.hpp>
#include <dd99/wayland/message_marshaling.hpp>
#include <dd99/wayland/message_parsing.hpp> // used by interfaces that include this file
#include <dd99/wayland/types.hpp>

#include <cstdint>
#include <format>
#include <source_location>
#include <stdexcept>



namespace dd99::wayland::proto
{

    // base class for all protocol-defined interfaces
    // objects are instances of derived classes
    // derived classes are generated from xml protocol descriptions
    struct interface
    {
    protected: // types
        friend dd99::wayland::engine;


    public: // constructor/destructor
        virtual ~interface() = default;

        interface(engine & eng)
            : m_engine{eng}
        { }
    
    
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


    protected: // functions that derived classes must implement (generated from xml)
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


    protected: // member variables
        engine & m_engine;
        std::uint32_t m_object_id = 0;
    };



    // ***********************************************
    // * Template member functions (implementations) *
    // ***********************************************

    // transform interface references to object_ids and forward to marshalling
    template <class ... Args>
    void interface::send_wayland_message(opcode_t opcode, std::span<int> fds, Args && ... args)
    {
        auto interfaces_to_ids = []<class T>(T && t)
        {
            // incomplete types are assumed to be interfaces
            if constexpr (!requires{sizeof(std::remove_cvref_t<T>);}) return reinterpret_cast<const interface &>(t).m_object_id;
            else if constexpr (Interface_C<std::remove_cvref_t<T>>) return reinterpret_cast<const interface &>(t).m_object_id;
            else return std::forward<T>(t);
        };

        dd99::wayland::detail::message_marshal(m_engine, m_object_id, opcode, fds
            , interfaces_to_ids.template operator()<Args>(args) ...);
    }

}
