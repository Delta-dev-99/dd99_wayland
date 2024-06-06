#include "asio/buffer.hpp"
#include "asio/buffered_write_stream.hpp"
#include "dd99-wayland-client-protocol-wayland.hpp"
#include "dd99/wayland/engine.hpp"
#include <cstring>
#include <dd99/wayland/wayland_client.hpp>

#include <asio.hpp>

#include <format>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <sys/socket.h>
#include <thread>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <filesystem>


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



struct engine final : dd99::wayland::engine
{
    using sock_type = asio::buffered_write_stream<asio::local::stream_protocol::socket &>;

    engine(sock_type & sock)
        : m_sock(sock)
    {}


    void on_output(std::span<const char> data, std::span<int> ancillary_output_fd_collection) override
    {
        asio::write(m_sock, asio::buffer(data));
    }


    sock_type & m_sock;
};


// struct display final : dd99::wayland::proto::wayland::display
// {
    
// };

struct registry final : pw::registry
{
    using pw::registry::registry;

    void on_global(std::uint32_t name, std::string_view interface, std::uint32_t version) override
    {
        std::format_to(std::ostream_iterator<char>(std::cout), 
            "registry::on_global    name: {:2}    version: {:2}    interface: {}\n"
        , name, version, interface);
    }
};



int main()
{
    // auto newsock = socket(PF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);

    asio::io_context io_ctx;

    auto sock = connect_to_wayland_server(io_ctx);

    asio::buffered_write_stream<decltype(sock) &> bufsock{sock, 1024*4};

    // connected!

    engine wl_eng{bufsock};
    auto & display = wl_eng.create_display();
    auto & reg = display.get_registry<registry>();

    // auto && [display_id, wl_display] = wl_eng.allocate_interface<dd99::wayland::proto::wayland::display>();
    // auto disp = wl_eng.get_display();
    // wlp::wayland::display disp{}


    asio::error_code ec;


    std::vector<char> buf;
    buf.resize(1024*4);
    std::size_t chars_in_buf = 0;
    for (;;) {
        auto n_written [[maybe_unused]] = bufsock.flush();
        chars_in_buf += bufsock.read_some(asio::buffer(buf.data() + chars_in_buf, buf.size() - chars_in_buf), ec);
        auto n_consumed = wl_eng.process_input({buf.data(), buf.data() + chars_in_buf});
        if (n_consumed != chars_in_buf) std::memmove(buf.data(), buf.data() + n_consumed, chars_in_buf - n_consumed);
        chars_in_buf -= n_consumed;
        // buf.erase(buf.begin(), buf.begin() + n_consumed);
        // std::this_thread::sleep_for(std::chrono::microseconds{20});
    }
}
