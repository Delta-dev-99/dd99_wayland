#pragma once

#include <concepts>



namespace dd99::wayland::proto
{

    // fw declaration
    struct interface;

    // concept of interface: derives from dd99::wayland::proto::interface
    template <class T> concept Interface_C = std::derived_from<T, interface>;

}
