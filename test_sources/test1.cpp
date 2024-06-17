
#include "dd99-wayland-client-protocol-wayland.hpp"
#include "dd99-wayland-client-protocol-xdg-shell.hpp"
#include <dd99/wayland/wayland_client.hpp>

#include <asio.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <new>
#include <stdexcept>
#include <sys/socket.h>
#include <system_error>
#include <type_traits>
#include <vector>


namespace wlp = dd99::wayland::proto;
namespace pw = wlp::wayland;



// struct wayland_app
// {
//     static wayland_app & get_instance() { static wayland_app instance; return instance; }

// private:
//     wayland_app() = default;


// private:

// };




auto connect_to_wayland_server(asio::io_context & io_ctx)
{
    auto wayland_path = []()->std::filesystem::path {
        std::filesystem::path wayland_display = []()->const char * {
            auto env = std::getenv("WAYLAND_DISPLAY");
            if (env) return env;
            else return "wayland-0";
        }();
        if (wayland_display.is_absolute()) return wayland_display;
        std::filesystem::path runtime_dir = [](){
            auto env = std::getenv("XDG_RUNTIME_DIR");
            if (env) return env;
            else throw std::runtime_error{"no path to wayland"};
        }();
        if (!runtime_dir.is_absolute()) throw std::runtime_error{"no path to wayland"};
        return runtime_dir/wayland_display;
    }();

    asio::local::stream_protocol::socket sock{io_ctx};
    sock.connect(asio::local::stream_protocol::endpoint{wayland_path});

    return sock;
}





// a callback that calls any custom callable
template <class F>
struct callback final : pw::callback
{
    callback(dd99::wayland::engine & eng, F && f)
        : pw::callback{eng}
        , m_f{f}
    { }

    void on_done(std::uint32_t serial) override { pw::callback::on_done(serial); m_f(); }

    F m_f;
};

// a callback that calls a member function
template <class T>
struct callback_member final : pw::callback
{
    using signature = void(T*, std::uint32_t);
    using instance_t = T;

    callback_member(dd99::wayland::engine & eng, T * this_ptr, void (instance_t::* fnct_ptr)(std::uint32_t))
        : pw::callback{eng}
        , m_i{this_ptr}
        , m_f{fnct_ptr}
    { }

    void on_done(std::uint32_t serial) override { pw::callback::on_done(serial); (m_i->*m_f)(serial); }

    instance_t * m_i;
    void (instance_t::* m_f)(std::uint32_t);
};


template <asio::completion_token_for<void(asio::error_code, std::uint32_t)> CompletionToken>
struct asio_callback : pw::callback
{
    using async_result_type = asio::async_result<std::decay_t<CompletionToken>, void(asio::error_code, std::uint32_t)>;
    using result_type = async_result_type::return_type;
    using handler_type = async_result_type::handler_type;

    asio_callback(dd99::wayland::engine & eng, CompletionToken && token)
        : pw::callback{eng}
    {
        result = asio::async_initiate<CompletionToken, void(asio::error_code, std::uint32_t)>(
            [](auto handler, std::optional<handler_type> & instance_handler, std::optional<asio::executor_work_guard<asio::any_io_executor>> & instance_work_guard)
            {
                auto ex = asio::get_associated_executor(handler);
                instance_work_guard.emplace(asio::make_work_guard(ex));
                instance_handler.emplace(std::move(handler));
            }, token, std::ref(handler), std::ref(work_guard));
    }

    auto get_async()
    {
        auto t = std::move(*result);
        result.reset();
        return std::move(t);
    }

protected:
    void on_done(std::uint32_t callback_data) override
    {
        // (*work_guard).reset();
        work_guard.reset();
        if (handler)
        {
            // std::move(*handler)({}, callback_data); // somehow this causes a crash
            asio::dispatch([=, handler = std::move(*handler)]() mutable { handler({}, callback_data); });
            handler.reset();
        }
    }

private:
    std::optional<result_type> result;
    std::optional<handler_type> handler;
    std::optional<asio::executor_work_guard<asio::any_io_executor>> work_guard;
};
template <asio::completion_token_for<void(asio::error_code, std::uint32_t)> CompletionToken>
asio_callback(dd99::wayland::engine &, CompletionToken &&) -> asio_callback<CompletionToken>;


struct xdg_toplevel final : wlp::xdg_shell::xdg_toplevel
{
    using wlp::xdg_shell::xdg_toplevel::xdg_toplevel;

    bool closed = false;

protected:
    void on_close() override { wlp::xdg_shell::xdg_toplevel::on_close(); destroy(); closed = true; }
};

struct xdg_surface final : wlp::xdg_shell::xdg_surface
{
    using wlp::xdg_shell::xdg_surface::xdg_surface;

protected:
    void on_configure(std::uint32_t serial) override { wlp::xdg_shell::xdg_surface::on_configure(serial); ack_configure(serial); }
};


struct xdg_base final : wlp::xdg_shell::xdg_wm_base
{
    using wlp::xdg_shell::xdg_wm_base::xdg_wm_base;

protected:
    void on_ping(std::uint32_t serial) override { wlp::xdg_shell::xdg_wm_base::on_ping(serial); pong(serial); }
};


struct registry final : pw::registry
{
    registry(dd99::wayland::engine & eng)
        : pw::registry{eng}
        , xdg_wm_base{eng}
        , compositor{eng}
        , shm{eng}
    { }

protected:
    void on_global(std::uint32_t name, dd99::wayland::proto::zview interface, std::uint32_t version) override
    {
        pw::registry::on_global(name, interface, version);

        // std::format_to(std::ostream_iterator<char>(std::cout), 
        //     "registry::on_global    name: {:2}    version: {:2}    interface: {}\n"
        // , name, version, interface.sv());

        if (interface == wlp::xdg_shell::xdg_wm_base::interface_name) bind(name, interface, version, xdg_wm_base);
        else if (interface == pw::compositor::interface_name) bind(name, interface, version, compositor);
        else if (interface == pw::shm::interface_name) bind(name, interface, version, shm);
    }

public:
    xdg_base xdg_wm_base;
    pw::compositor compositor;
    pw::shm shm;
};


struct display final : pw::display
{
    using pw::display::display;

protected:
    void on_delete_id(std::uint32_t id) override
    {
        pw::display::on_delete_id(id);
        m_engine.unbind_interface(id);
    }
};


struct engine final : dd99::wayland::engine
{
    // using sock_type = asio::buffered_write_stream<asio::local::stream_protocol::socket &>;
    using sock_type = asio::local::stream_protocol::socket;

    engine(sock_type & sock)
        : m_sock(sock)
    {}


    void on_output(std::span<const char> data, std::span<int> ancillary_output_fd_collection) override
    {
        // static std::ofstream binary_output_log{"binary_output_log"};
        // binary_output_log.write(data.data(), data.size());
        // binary_output_log.flush();

        if (ancillary_output_fd_collection.empty())
        {
            auto n [[maybe_unused]] = asio::write(m_sock, asio::buffer(data));
            // if (n == 0)
            //     throw std::runtime_error{"failed write!"};
            return;
        }


        // m_sock.flush();
        // asio::write(m_sock.next_layer(), asio::buffer("something"));

        ::msghdr msg {};
        ::cmsghdr * cmsg;

        ::iovec io{
            .iov_base = const_cast<char *>(data.data()),
            .iov_len = data.size(),
        };

        auto data_size = sizeof(int) * ancillary_output_fd_collection.size();
        auto fd_buf_size = CMSG_SPACE(data_size);

        auto fd_buf = new (std::align_val_t{alignof(::cmsghdr)}) char[fd_buf_size];

        msg.msg_iov = &io;
        msg.msg_iovlen = 1;
        msg.msg_control = fd_buf;
        msg.msg_controllen = fd_buf_size;
        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(data_size);
        std::memcpy(CMSG_DATA(cmsg), ancillary_output_fd_collection.data(), data_size);

        // auto r = sendmsg(m_sock.next_layer().native_handle(), &msg, 0);
        auto r = sendmsg(m_sock.native_handle(), &msg, 0);

        operator delete[](fd_buf, std::align_val_t{alignof(::cmsghdr)});

        if (r == -1) throw std::system_error{{errno, std::system_category()}};
    }


    sock_type & m_sock;
};


struct wayland_app
{
    static constexpr std::string_view shared_memory_filename {"dd99-wlxx-sharedmem"};

    ~wayland_app()
    {
        boost::interprocess::shared_memory_object::remove(shared_memory_filename.data());
    }

    wayland_app(asio::io_context & io_ctx)
        : m_socket{connect_to_wayland_server(io_ctx)}
        , m_buffered_write_socket{m_socket, 1024*4}
        , m_engine{m_socket}
        , m_display{m_engine}
        , m_registry{m_engine}
        , m_shm_pool{m_engine}
        , m_pixel_buffer{m_engine}
        , m_surface{m_engine}
        , m_xdg_surface{m_engine}
        , m_xdg_toplevel{m_engine}
        , m_shared_memory{boost::interprocess::create_only, shared_memory_filename.data(), boost::interprocess::read_write}
    {
        m_buffer.reserve(1024*4);

        m_engine.bind_display(m_display);
        m_display.get_registry(m_registry);
        m_callback.emplace(m_engine, this, &wayland_app::on_registry_sync);
        m_display.sync(*m_callback);
        // m_buffered_socket.flush();
    }

public:
    void run()
    {
        // static std::ofstream binary_input_log{"binary_input_log"};

        using std::memmove;

        // make sure buffer has some space
        m_previous_buffer_size = m_buffer.size();
        auto freespace = m_buffer.capacity() - m_buffer.size();
        if (freespace == 0) m_buffer.resize(m_buffer.size() + 1024*4);
        else m_buffer.resize(m_buffer.capacity());

        m_socket.async_read_some(asio::buffer(m_buffer.data() + m_previous_buffer_size, m_buffer.capacity() - m_previous_buffer_size),
        [&](std::error_code ec, std::size_t n){

            // binary_input_log.write(m_buffer.data() + m_previous_buffer_size, n);
            // binary_input_log.flush();

            if (ec) throw std::system_error{ec};

            m_buffer.resize(m_previous_buffer_size + n);
            auto n_consumed = m_engine.process_input(m_buffer);
            if (n_consumed != m_buffer.size()) memmove(m_buffer.data(), m_buffer.data() + n_consumed, m_buffer.size() - n_consumed);
            m_buffer.resize(m_buffer.size() - n_consumed);
            run();
        });
    }


private:
    void on_registry_sync(std::uint32_t cb_serial)
    {
        m_callback.reset();

        if (!m_registry.compositor.get_id() || !m_registry.shm.get_id() || !m_registry.xdg_wm_base.get_id())
        {
            std::cerr << "server does not support some required interfaces\n";
            return;
        }

        struct pixel {
            // Components are in reverse order, because endian?
            std::uint8_t b;
            std::uint8_t g;
            std::uint8_t r;
            std::uint8_t a;
        };
        const auto width = 400;
        const auto height = 300;
        const auto n_pixels = width * height;
        const auto shared_size = n_pixels * sizeof(pixel);
        m_shared_memory.truncate(shared_size);

        m_registry.compositor.create_surface(m_surface);
        m_registry.xdg_wm_base.get_xdg_surface(m_xdg_surface, m_surface);
        m_xdg_surface.get_toplevel(m_xdg_toplevel);

        // m_buffered_socket.flush();
        m_registry.shm.create_pool(m_shm_pool, m_shared_memory.get_mapping_handle().handle, shared_size);
        m_shm_pool.create_buffer(m_pixel_buffer, 0, width, height, width * sizeof(pixel), pw::shm::format::argb8888);
        boost::interprocess::mapped_region shared_region{m_shared_memory, boost::interprocess::read_write};

        const pixel green {
            .b = 0,
            .g = 200,
            .r = 0,
            .a = 100,
        };

        const auto pixel_out = static_cast<pixel*>(shared_region.get_address());
        std::fill(pixel_out, pixel_out + n_pixels, green);

        m_surface.attach(m_pixel_buffer, 0, 0);
        m_surface.commit();
        // m_buffered_socket.flush();
    }


public:
    asio::local::stream_protocol::socket m_socket;
    asio::buffered_write_stream<decltype(m_socket) &> m_buffered_write_socket;
    std::vector<char> m_buffer{};
    std::size_t m_previous_buffer_size = 0;

    engine m_engine;
    display m_display;
    registry m_registry;

    std::optional<callback_member<wayland_app>> m_callback;

    pw::shm_pool m_shm_pool;
    pw::buffer m_pixel_buffer;
    pw::surface m_surface;
    xdg_surface m_xdg_surface;
    xdg_toplevel m_xdg_toplevel;

    boost::interprocess::shared_memory_object m_shared_memory;
};



int main()
{
    asio::io_context io_ctx;

    boost::interprocess::shared_memory_object::remove(wayland_app::shared_memory_filename.data());
    wayland_app app{io_ctx};

    app.run();

    // asio::co_spawn(io_ctx, feed_wayland_data(app.m_engine, app.m_buffered_socket), asio::detached);

    io_ctx.run();
}

