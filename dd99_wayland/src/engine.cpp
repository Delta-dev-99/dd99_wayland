#include <dd99/wayland/engine.hpp>
#include <dd99/wayland/interface.hpp>
#include <dd99/wayland/types.hpp>
#include "engine_data.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>



namespace dd99::wayland
{


    engine::engine()
        : m_data_ptr{new data_t, [](data_t * ptr){ return delete ptr; }}
    { }

    std::size_t engine::process_input(std::span<char> data)
    {
        std::size_t consumed = 0;

        // process one message per cycle
        // stop when there's not enough data to complete a message
        for(;;)
        {
            const auto available_data = data.size() - consumed;
            constexpr auto hdr_size = sizeof(object_id_t) + sizeof(message_size_t);

            if (available_data < hdr_size) break;

            object_id_t msg_obj_id = *reinterpret_cast<std::uint32_t *>(data.data());
            message_size_t msg_size = static_cast<std::uint16_t>((*(reinterpret_cast<std::uint32_t *>(data.data()) + 1)) >> 16);
            
            if (available_data < msg_size) break;
            consumed += msg_size;
            
            m_data_ptr->m_client_object_map[msg_obj_id]->parse_and_dispatch_event(data);

            data = data.subspan(msg_size);
        }

        return consumed;
    }

}
