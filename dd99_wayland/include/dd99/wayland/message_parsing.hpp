#pragma once

#include "dd99/wayland/interface.hpp"
#include "dd99/wayland/types.hpp"
#include <concepts>
#include <cstdint>
#include <span>
#include <string_view>
#include <tuple>



namespace dd99::wayland::proto
{

    // returns the number of consumed bytes and the parsed element
    template <class T>
    std::pair<std::size_t, T> parse_msg_arg(std::span<const char> buffer)
    {
        // TODO: array

        if constexpr (std::same_as<T, std::string_view>) // wayland type: string
        {
            auto size = *reinterpret_cast<const std::uint32_t *>(buffer.data());
            return {sizeof(std::uint32_t) + ((size + sizeof(std::uint32_t) - 1) & ~(sizeof(std::uint32_t) - 1))
                , std::string_view{buffer.data() + sizeof(std::uint32_t), size}};
        }
        else // most argument types have the same binary representation as the corresponding c++ type (and 4 bytes size)
        {
            auto v = *reinterpret_cast<const T*>(buffer.data());
            return {sizeof(v), v};
        }
        
        // new id (new interface created by server) and object id (interface reference)
        // are parsed as plain object id's and are interpreted by caller
    }

    template <class ... Args>
    std::tuple<Args...> parse_msg_args(std::span<const char> buffer)
    {
        std::size_t consumed = 0;
        auto get_elem = [&]<class U>(){ auto && [n, elem] = parse_msg_arg<U>(buffer.subspan(consumed)); consumed += n; return elem; };

        return std::make_tuple(get_elem.template operator()<Args>()...);
    }

}
