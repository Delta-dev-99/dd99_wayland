#pragma once


#include <dd99/wayland/interface_binder.hpp>
#include <dd99/wayland/types.hpp>

#include <cassert>
#include <concepts>
#include <cstddef>
#include <memory>
#include <span>



namespace dd99::wayland
{

    // fw-declarations
    namespace proto { struct interface; }
    namespace proto::wayland { struct display; }

    // for pimpl
    namespace detail { struct engine_data; }



    // The engine stores all created interfaces and assigns an object-id to new interfaces
    // Interface creation/destruction and binding can only be done by the engine
    // Interface functions that create new interfaces delegate construction to the engine
    // 
    // The reading/writing of data through the connection to wayland server is up to the user
    // To use: inherit from this class and define the virtual methods
    struct engine
    {
    protected:
        // virtual destruction not allowed
        // you must destruct the engine instance through a properly typed instance object destructor
        ~engine() = default;


    public:
        engine();

        engine(const engine &) = delete; // no copy
        engine(engine&&) = default; // default move


    public: // API
        proto::interface * get_interface(object_id_t id); // nullptr when not found
        template <class T> T * get_interface(object_id_t id) { return reinterpret_cast<T *>(get_interface(id)); }

    
    public: // Wayland-related API

        // An engine can only have one display and it must be the first created object.
        // 
        // Second display instance:
        // If a second display is created, it will result in assertion failure at runtime (if assertions are enabled).
        // If assertions are disabled, the new instance will be created without errors, but it will not receive any events and using any of it's functions will probably result in a protocol error.
        // 
        // More specifically, the main display instance is predefined at object id 1 and does not need to be registered.
        // The creation of the display only registers the new instance internally in the engine (locally).
        // This does not register a new object id with the wayland server.
        // A second instance will (locally) get object id 2 or above. Using it's functions will result in a message sent to the server that uses an object id that was not registered.
        // The server has no way to interpret that message.
        void bind_display(proto::wayland::display & display_instance)
        {
            auto new_id = bind_interface(reinterpret_cast<proto::interface &>(display_instance), 1);

            // wayland display must be the first object bound
            // more than one display is not allowed
            // *** user is responsible for this ***
            assert(new_id == 1);
        }


    public: // I/O API

        // The user is expected to manually read data from the wayland server connection and handle buffering.
        // When data arrives from the server, use this function to give it to the display.
        // This function dispatches the received events to the apropiate callbacks.
        // 
        // About buffering:
        // The user is expected to buffer received data. This handles the possibility
        // of partial messages that must be kept until they are processed.
        // The return value can be used to indicate whether there is a partial message in buffer.
        // This function takes `iovec` (scatter/gather style) input to allow efficient implementations using ringbuffers.
        // 
        // @RETURN: bytes consumed
        // std::size_t process_input(iovec * data, std::size_t iovec_count);
        std::size_t process_input(std::span<const char> data);


    public: // I/O Events that MUST be implemented by derived clases
        
        // When there is data to be sent to the wayland server, this callback gets called.
        // Calling this function is always the result of interacting with wayland objects in this library.
        // 
        // Signature:
        //  `data` is just binary data to be sent to the server
        //  `fds` is a collection of file descriptors to be sent as ancillary data
        virtual void on_output(std::span<const char> data, std::span<int> ancillary_output_fd_collection) = 0;


    public: // internal functions used by protocol interfaces. Do not use directly. TODO: move to accessor for interfaces

        object_id_t bind_interface(proto::interface &, version_t version);

        void unbind_interface(object_id_t id);

        // template <class T, class ... Args>
        // std::pair<object_id_t, T &> allocate_interface(Args && ... args);

        // void free_interface(proto::interface &);


    private: // auxiliary type definitions

        // obscure data type used for dependency decoupling
        using data_t = detail::engine_data;

        // implementation deleter type (just a function ptr)
        // used for decoupling `impl_t`'s destructor implementation from the API
        using data_deleter_signature = void(data_t*);
        using data_deleter_t = data_deleter_signature *;


    private: // data members
        
        std::unique_ptr<data_t, data_deleter_t> m_data_ptr;

    };



    // template<class T, class ... Args>
    // std::pair<object_id_t, T &> engine::allocate_interface(Args && ... args)
    // {
    //     auto && [new_id, new_interface] = bind_interface(std::make_unique<T>(*this, std::forward<Args>(args)...));
    //     return {new_id, reinterpret_cast<T&>(new_interface)};
    // }


    // template <std::derived_from<proto::wayland::display> T, class ... Args>
    // T & engine::create_display(Args && ... args)
    // {
    //     auto && [new_id, new_interface] = bind_interface(std::make_unique<T>(*this, std::forward<Args>(args)...));
    //     assert(new_id == 1);
    //     return reinterpret_cast<T&>(new_interface);
    // }

}
