#pragma once

#include <dd99/wayland/interface.hpp>
#include "dd99/wayland/types.hpp"
#include "object_map.hpp"

#include <map>



namespace dd99::wayland::detail
{

        // the data used by the engine
        // this structure is used via PIMPL
        // Object maps used for translating object-id to object instance
        struct engine_data
        {
            static constexpr object_id_t client_object_id_base = 1;
            static constexpr object_id_t server_object_id_base = 0xFF000000;

            using local_obj_map_type = object_map<proto::interface, object_id_t, client_object_id_base>;
            // using remote_obj_map_type = object_map<proto::interface, object_id_t, server_object_id_base>;
            using remote_obj_map_type = std::map<object_id_t, std::unique_ptr<proto::interface>>;

            local_obj_map_type m_client_object_map{};
            remote_obj_map_type m_server_object_map{};
        };

}
