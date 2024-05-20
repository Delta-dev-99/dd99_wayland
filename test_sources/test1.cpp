#include <dd99/wayland/wayland_client.hpp>
#include <span>



int main()
{

    dd99::wayland::display disp
    {
        [](std::span<unsigned char>, std::span<int>)
        {

        }
    };

    

}
