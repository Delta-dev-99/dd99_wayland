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

    std::size_t engine::process_input(std::span<const char> data)
    {
        std::size_t consumed = 0;

        // process one message per cycle
        // stop when there's not enough data to complete a message
        for(;;)
        {
            const auto available_data = data.size();
            constexpr auto hdr_size = sizeof(object_id_t) + sizeof(message_size_t) + sizeof(opcode_t);

            if (available_data < hdr_size) break;

            const object_id_t msg_obj_id = *reinterpret_cast<const std::uint32_t *>(data.data());
            const message_size_t msg_size = static_cast<std::uint16_t>((*(reinterpret_cast<const std::uint32_t *>(data.data()) + 1)) >> 16);
            const opcode_t code [[maybe_unused]] = static_cast<std::uint16_t>((*(reinterpret_cast<const std::uint32_t *>(data.data()) + 1)) & ((1<<16)-1));
            
            if (available_data < msg_size) break;
            consumed += msg_size;
            
            m_data_ptr->m_client_object_map[msg_obj_id]->parse_and_dispatch_event(data);

            data = data.subspan(msg_size);
        }

        return consumed;
    }


    object_id_t engine::bind_interface(proto::interface & interface_instance)
    {
        // inserting into the object map allocates an object id (the key of the map)
        auto it = m_data_ptr->m_client_object_map.insert(&interface_instance);
        // get the new allocated object id
        auto new_object_id = it.get_key();

        interface_instance.m_object_id = new_object_id;
        return new_object_id;
    }

    void engine::unbind_interface(object_id_t id)
    {
        if (id >= detail::engine_data::server_object_id_base)
        {
            m_data_ptr->m_server_object_map.erase(id);
        }
        else
        {
            m_data_ptr->m_client_object_map.erase(id);
        }
    }

    proto::interface * engine::get_interface(object_id_t id)
    {
        if (id >= detail::engine_data::server_object_id_base)
        {
            auto it = m_data_ptr->m_server_object_map.find(id);
            if (it == m_data_ptr->m_server_object_map.end()) return nullptr;
            else return it->second.get();
        }
        else
        {
            if (m_data_ptr->m_client_object_map.is_in_range(id))
                return m_data_ptr->m_client_object_map[id];
            else return nullptr;
        }
    }

}
