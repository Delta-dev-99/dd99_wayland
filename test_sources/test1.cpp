#include "dd99-wayland-client-protocol-wayland.hpp"
#include <dd99/wayland/wayland_client.hpp>

#include <asio.hpp>

#include <stdexcept>
#include <thread>
#include <vector>

#include <chrono>
#include <cstdlib>
#include <filesystem>


namespace wlp = dd99::wayland::proto;



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


struct engine : dd99::wayland::engine
{
    engine(asio::local::stream_protocol::socket & sock)
        : m_sock(sock)
    {}


    void on_output(std::span<char> data, std::span<int> ancillary_output_fd_collection) override
    {
        asio::write(m_sock, asio::buffer(data));
    }


    asio::local::stream_protocol::socket & m_sock;
};



int main()
{
    asio::io_context io_ctx;

    auto sock = connect_to_wayland_server(io_ctx);

    // connected!

    engine wl_eng{sock};
    auto & display = wl_eng.create_display();
    // auto && [display_id, wl_display] = wl_eng.allocate_interface<dd99::wayland::proto::wayland::display>();
    // auto disp = wl_eng.get_display();
    // wlp::wayland::display disp{}




    std::vector<char> buf;
    buf.reserve(2048);
    for (;;) {
        auto n [[maybe_unused]] = sock.read_some(asio::buffer(buf));
        auto n_consumed = wl_eng.process_input(buf);
        buf.erase(buf.begin(), buf.begin() + n_consumed);
        std::this_thread::sleep_for(std::chrono::microseconds{20});
    }
}
