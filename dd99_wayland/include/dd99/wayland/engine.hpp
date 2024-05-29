#pragma once

#include <concepts>
#include <memory>
#include <span>
#include <cstdint>
#include <utility>



namespace dd99::wayland
{
    namespace proto{
        struct interface;
        namespace wayland{
            struct display;
        }
    }

    namespace detail { struct engine_impl; }



    // signature for the output callback used by the engine
    using engine_cb_signature = void(void * user_data, std::span<unsigned char> output_data, std::span<int> ancillary_output_fd_collection);
    using engine_cb_t = engine_cb_signature *;

    // engine template declaration
    template <class OutCB = engine_cb_t> struct engine;





    // Core of wayland interaction
    // This class acts as an engine for (de)marshaling and dispatching of wayland protocol data
    // 
    // This is the default template specialization
    // Other template specializations are wrappers around this one
    // This template specialization implements the engine logic
    template<>
    struct engine<engine_cb_t>
    {
    public:
        using version_t = std::uint32_t;
        using object_id_t = std::uint32_t;
        using opcode_t = std::uint16_t;
        using message_length_t = std::uint16_t;


    public: // constructors

        // the user MUST set the callback to a valid function pointer
        // before calling any i/o member function.
        // output_cb: callback called when engine wants to output something
        engine(engine_cb_t output_cb = nullptr, void * user_data = nullptr);


        engine(const engine &) = delete; // no copy
        engine(engine&&) = default; // default move

    
    public:
        // get a reference of the root object of the protocol: the wayland display
        // dd99::wayland::proto::wayland::display & get_display();
        void bind_display();

    
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
        void set_output_callback(engine_cb_t cb, void* user_data);


        // int get_error();
        // uint32_t get_protocol_error();

    
    protected: // API exposed to interfaces
        friend struct dd99::wayland::proto::interface;

        template <class Interface> std::pair<object_id_t, Interface &> create_interface(); // called by interfaces on messages
        void destroy_iterface(const proto::interface & x); // called by interface destructors

        // accessor for protocol-defined interfaces
        struct engine_accessor
        {
            engine & m_engine;
            template <class Interface> auto create_interface() { return m_engine.create_interface<Interface>(); }

            void destroy_interface(const proto::interface & x) { return m_engine.destroy_iterface(x); }
        };

    
    private:
        std::pair<object_id_t, dd99::wayland::proto::interface &> store_new_interface(std::unique_ptr<dd99::wayland::proto::interface>);


    private:
        using impl_t = detail::engine_impl;
        using impl_deleter_signature = void(impl_t*);
        using impl_deleter_t = impl_deleter_signature *;
        
        std::unique_ptr<impl_t, impl_deleter_t> impl;
    };

    template <class Interface>
    inline auto engine<engine_cb_t>::create_interface() -> std::pair<object_id_t, Interface &>
    {
        // Interface x{.m_engine = engine_accessor{*this}};
        auto && [id, new_interface_ref] = store_new_interface(std::make_unique<Interface>(engine_accessor{*this}));
        return {id, *reinterpret_cast<Interface *>(& new_interface_ref)};
    }





    // wrapper to allow custom callback types
    // this is the main definition of the template
    template <class OutCB>
    struct engine : engine<engine_cb_t>
    {
        static_assert(std::is_invocable_v<OutCB, std::span<unsigned char>, std::span<int>>, "OutCB signature is incorrect!");
        using base = engine<engine_cb_t>;

        // create and set callback. Useful for use with lambdas
        engine(OutCB && cb)
            : base{[](void * instance, std::span<unsigned char> data, std::span<int> fd){ reinterpret_cast<engine *>(instance)->output_cb(data, fd); }}
            , output_cb{std::move(cb)}
        { }

        void set_output_callback(OutCB && cb) { output_cb = std::forward<OutCB>(cb); }


    private:
        OutCB output_cb;
    };

    // engine template deduction guide to prioritize basic callback form
    template <std::convertible_to<engine_cb_t> T>
    engine(T&&) -> engine<engine_cb_t>;
    

}
