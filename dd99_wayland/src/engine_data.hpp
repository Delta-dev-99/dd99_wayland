#pragma once

#include <dd99/wayland/interface.hpp>
#include "object_map.hpp"



namespace dd99::wayland::detail
{

        // the data used by the engine
        // this structure is used via PIMPL
        // Object maps used for translating object-id to object instance
        struct engine_data
        {
            static constexpr object_id_t client_object_id_base = 1;
            static constexpr object_id_t server_object_id_base = 0xFF000000;

            using client_obj_map_type = object_map<proto::interface, object_id_t, client_object_id_base>;
            using server_obj_map_type = object_map<proto::interface, object_id_t, server_object_id_base>;

            client_obj_map_type m_client_object_map{};
            server_obj_map_type m_server_object_map{};
        };

}
