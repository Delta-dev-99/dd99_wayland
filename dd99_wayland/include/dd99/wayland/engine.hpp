#pragma once


#include <dd99/wayland/types.hpp>

#include <cassert>
#include <concepts>
#include <cstddef>
#include <memory>
#include <span>



namespace dd99::wayland
{

    // fw-declaration of wayland display interface
    // namespace proto{
    //     struct interface;
    //     namespace wayland{
    //         struct display;
    //     }
    // }


    // fw-declarations
    namespace proto { struct interface; }
    namespace proto::wayland { struct display; }

    // for pimpl
    namespace detail { struct engine_data; }




    
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

    
    public: // Wayland-related API

        // create display of user-derived type
        template <std::derived_from<proto::wayland::display> T = proto::wayland::display, class ... Args>
        T & create_display(Args && ... args);


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
        std::size_t process_input(std::span<char> data);


    public: // I/O Events that MUST be implemented by derived clases
        
        // When there is data to be sent to the wayland server, this callback gets called.
        // Calling this function is always the result of interacting with wayland objects in this library.
        // 
        // Signature:
        //  `data` is just binary data to be sent to the server
        //  `fds` is a collection of file descriptors to be sent as ancillary data
        virtual void on_output(std::span<char> data, std::span<int> ancillary_output_fd_collection) = 0;


    public: // internal functions used by protocol interfaces

        std::pair<object_id_t, proto::interface &> bind_interface(std::unique_ptr<proto::interface>);

        template <class T, class ... Args>
        std::pair<object_id_t, T &> allocate_interface(Args && ... args);

        void free_interface(proto::interface &);


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



    template<class T, class ... Args>
    std::pair<object_id_t, T &> engine::allocate_interface(Args && ... args)
    {
        auto && [new_id, new_interface] = bind_interface(new T{*this, std::forward<Args>(args)...});
        return {new_id, reinterpret_cast<T&>(new_interface)};
    }


    template <std::derived_from<proto::wayland::display> T, class ... Args>
    T & engine::create_display(Args && ... args)
    {
        auto && [new_id, new_interface] = bind_interface(std::make_unique<T>(*this, std::forward<Args>(args)...));
        assert(new_id == 1);
        return reinterpret_cast<T&>(new_interface);
    }

}
