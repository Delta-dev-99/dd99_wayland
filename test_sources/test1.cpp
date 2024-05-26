#include <bits/types/struct_iovec.h>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <dd99/wayland/wayland_client.hpp>
#include <stdexcept>
#include <asio.hpp>
#include <thread>
#include <vector>
#include "asio/buffer.hpp"
#include "asio/connect.hpp"
#include "asio/io_context.hpp"
#include "asio/local/stream_protocol.hpp"
#include "asio/write.hpp"
#include "dd99-wayland-client-protocol-wayland.hpp"


int main()
{
    asio::io_context io_ctx;

    // struct wnd
    // {
    //     void event() {}
    // } mywnd;
    // dd99::wayland::engine wl{[&](std::span<unsigned char>, std::span<int>){mywnd.event();}};


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

    // connected!

    dd99::wayland::engine wl_eng
    {
        [&](std::span<unsigned char> data, std::span<int>)
        {
            asio::write(sock, asio::buffer(data));
        }
    };




    std::vector<char> buf;
    buf.reserve(2048);
    for (;;) {
        auto n [[maybe_unused]] = sock.read_some(asio::buffer(buf));
        auto n_consumed = wl_eng.process_input(buf);
        buf.erase(buf.begin(), buf.begin() + n_consumed);
        std::this_thread::sleep_for(std::chrono::microseconds{20});
    }
}
