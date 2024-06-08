#pragma once

// #include <dd99/wayland/engine.hpp>
#include <dd99/wayland/types.hpp>



namespace dd99::wayland
{

    // fw-declarations
    struct engine;
    

    // class used as argument in event callbacks (virtual functions)
    // to guarantee type safety for event-created interfaces.
    // 
    // If the event creates an interface of some type T, the user
    // can create an instance of a type derived from T
    // and use the object_id_binder passed on the callback to register
    // the created instance in the engine.
    // This registration is necessary in order to dispatch events to the
    // new interface instance created by the user.
    // 
    template <class T>
    struct interface_binder
    {
        interface_binder(engine & eng, object_id_t obj_id)
            : m_object_id(obj_id)
            , m_engine(eng)
        { }

        void bind(engine & eng, T & interface_instance);

    private:
        object_id_t m_object_id;
        engine & m_engine;
    };

}
