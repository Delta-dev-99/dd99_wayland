#include <dd99/wayland/engine.hpp>
#include <dd99/wayland/interface.hpp>
#include "object_map.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <memory>
#include <numeric>
#include <vector>



namespace dd99::wayland
{

    // struct wayland_object
    // {
    //     std::uint32_t object_id;
    // };

    namespace detail
    {

        // this is the actual engine implementation
        struct engine_impl
        {
            using object_id_t = engine<>::object_id_t;
            using message_size_t = engine<>::message_length_t;
            
            static constexpr object_id_t client_object_id_base = 1;
            static constexpr object_id_t server_object_id_base = 0xf0000000;


            engine_impl(engine_cb_t _cb, void * _user_data)
                : cb{_cb}
                , user_data{_user_data}
            { }



            std::size_t process_input(iovec * data, std::size_t iovec_count);



            engine_cb_t cb; // output callback
            void * user_data; // user data for output callback
            
            object_map<proto::interface, object_id_t, client_object_id_base> m_client_object_map{};
            object_map<proto::interface, object_id_t, server_object_id_base> m_server_object_map{};
        };

    }





    engine<engine_cb_t>::engine(engine_cb_t cb, void * user_data)
        : impl(new detail::engine_impl{cb, user_data}, [](detail::engine_impl * p){delete p;})
    { }

    void engine<engine_cb_t>::set_output_callback(engine_cb_t cb, void * user_data)
    {
        impl->cb = cb;
        impl->user_data = user_data;
    }

    std::size_t engine<engine_cb_t>::process_input(iovec * data, std::size_t iovec_count)
    {
        return impl->process_input(data, iovec_count);
    }





    namespace detail
    {
        std::size_t engine_impl::process_input(iovec * data, std::size_t iovec_count)
        {
            if (iovec_count == 0) return 0;

            const std::size_t total_data = std::accumulate(data, data + iovec_count, std::size_t{}, 
                [](std::size_t && acc, iovec & v){ return std::move(acc) + v.iov_len; });

            std::size_t consumed = 0;
            // std::size_t current_buf = 0;
            // std::size_t buf_pos = 0;

            auto extract_data = [&]<class T>() -> const T &
            {
                const auto available_contiguous_data = data[current_buf].iov_len - buf_pos;
                const auto data_ptr = static_cast<const char *>(data[current_buf].iov_base) + buf_pos;
                // check data element is contiguous in buffer and meets alignment requirements
                if ((available_contiguous_data >= sizeof(T)) &&
                    (reinterpret_cast<std::uintptr_t>(data_ptr) & (alignof(T) - 1) == 0))
                {
                    return *static_cast<const T *>(data_ptr);
                }
                else
                {
                    alignas(T) char elem_data[sizeof(T)];
                    auto remaining = sizeof(T);
                    std::copy_n(data_ptr, std::min(data[current_buf].iov_len - buf_pos, remaining), elem_data);
                }
            };

            // // *** TODO: do this the right way
            // static std::vector<char> buffer;
            // std::size_t sz = 0;
            // for (std::size_t i = 0; i < iovec_count; ++i) sz += data[i].iov_len;
            // buffer.clear();
            // buffer.reserve(sz);
            // if (buffer.capacity() < sz) buffer.resize(sz); // just to be sure
            // for (std::size_t i = 0; i < iovec_count; ++i)
            //     std::copy(reinterpret_cast<char *>(data[i].iov_base)
            //             , reinterpret_cast<char *>(data[i].iov_base) + data[i].iov_len
            //             , std::back_inserter(buffer));

            for (;;)
            {
                const auto available_data = total_data - consumed;

                constexpr auto hdr_size = sizeof(object_id_t) + sizeof(message_size_t);
                if (available_data < hdr_size) break;

                const object_id_t & msg_obj_id = * reinterpret_cast<object_id_t *>(buffer.data() + consumed);
                message_size_t & msg_size = * reinterpret_cast<message_size_t *>(buffer.data() + consumed + sizeof(msg_obj_id));

                if (available_data < msg_size) break;

                consumed += msg_size;

                // auto x = m_client_object_map[msg_obj_id]->parse_and_dispatch_event()
                // get_object().parse_and_dispatch_event(buffer.data() + consumed);
            }

            return consumed;
        }
    }

}
