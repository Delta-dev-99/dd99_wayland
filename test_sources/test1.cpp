#include <dd99/wayland/wayland_client.hpp>
#include "dd99-wayland-client-protocol-wayland.hpp"


int main()
{

    struct wnd
    {
        void event() {}
    } mywnd;

    dd99::wayland::engine disp
    {
        [](std::span<unsigned char>, std::span<int>)
        {

        }
    };

    dd99::wayland::engine wl{[&](std::span<unsigned char>, std::span<int>){mywnd.event();}};
}
