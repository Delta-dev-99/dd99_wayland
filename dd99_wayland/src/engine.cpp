#include <dd99/wayland/engine.hpp>



namespace dd99::wayland
{

    namespace detail
    {

        struct engine_impl
        {
            engine_cb_t cb;
            void * user_data;
        };

    }



    engine<engine_cb_t>::engine(engine_cb_t cb)
        : impl(new detail::engine_impl{cb}, [](detail::engine_impl * p){delete p;})
    {

    }

    void engine<engine_cb_t>::set_output_callback(engine_cb_t cb, void * user_data)
    {
        impl->cb = cb;
        impl->user_data = user_data;
    }

}
