#pragma once

#include "dd99/wayland/detail/zview.hpp"
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
        // string_view not allowed anymore
        static_assert(!std::same_as<T, std::string_view>);

        if constexpr (std::same_as<T, zview>) // wayland type: string
        {
            auto size = *reinterpret_cast<const std::uint32_t *>(buffer.data());
            auto size_aligned_to_32bit = (size + sizeof(std::uint32_t) - 1) & ~(sizeof(std::uint32_t) - 1);
            
            // size - 1 because we want to ignore the null terminator
            zview str{buffer.data() + sizeof(std::uint32_t), size - 1};
            assert(buffer.data()[sizeof(std::uint32_t) + size - 1] == 0); // null terminator

            return {sizeof(std::uint32_t) + size_aligned_to_32bit, str};
        }
        else if constexpr (std::same_as<T, std::span<const char>>) // wayland type: array
        {
            auto size = *reinterpret_cast<const std::uint32_t *>(buffer.data());
            auto size_aligned_to_32bit = (size + sizeof(std::uint32_t) - 1) & ~(sizeof(std::uint32_t) - 1);

            return {sizeof(std::uint32_t) + size_aligned_to_32bit, {buffer.data() + sizeof(std::uint32_t), size}};
        }
        else // most argument types have the same binary representation as the corresponding c++ type (and 4 bytes size)
        {
            auto v = *reinterpret_cast<const T*>(buffer.data());
            return {sizeof(v), v};
        }
        
        // new id (new interface created by server) and object id (interface reference)
        // are parsed as plain object id's and are interpreted by caller
    }

    // NOTE: this approach reverses parsing order due to function argument evaluation order being unspecified in C++
    // keeping this here for a while (so this note shows up on the next git commit)
    // template <class ... Args>
    // std::tuple<Args...> parse_msg_args(std::span<const char> buffer)
    // {
    //     std::size_t consumed = 0;
    //     auto get_elem = [&]<class U>(){ auto && [n, elem] = parse_msg_arg<U>(buffer.subspan(consumed)); consumed += n; return elem; };

    //     return std::make_tuple(get_elem.template operator()<Args>()...);
    // }

    template <class T, class ... Args>
    std::tuple<T, Args...> parse_msg_args(std::span<const char> buffer)
    {
        auto [consumed, parsed_elem] = parse_msg_arg<T>(buffer);
        if constexpr (sizeof...(Args) == 0) return std::make_tuple(std::move(parsed_elem));
        else return std::tuple_cat(std::make_tuple(std::move(parsed_elem)), parse_msg_args<Args...>(buffer.subspan(consumed)));
    }

}
