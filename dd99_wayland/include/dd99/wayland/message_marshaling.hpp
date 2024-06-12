#pragma once

#include "dd99/wayland/detail/zview.hpp"
#include <concepts>
#include <dd99/wayland/engine.hpp>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <utility>
#include <span>



namespace dd99::wayland::detail
{

    template <class T>
    inline constexpr std::size_t _marshal_size_one(T && v)
    {
        // string_view not allowed any more
        static_assert(!std::same_as<std::remove_cvref_t<T>, std::string_view>);

        if constexpr (std::same_as<std::remove_cvref_t<T>, zview>)
        {
            auto str_size = v.size() + 1; // account for the null terminator
            // 32-bit size, then the payload, padded to 32-bit alignment
            return str_size + sizeof(std::uint32_t) + 
                (sizeof(std::uint32_t) - 1 - ((str_size - 1) & (sizeof(std::uint32_t) - 1)));
        }
        else if constexpr (std::same_as<std::remove_cvref_t<T>, std::span<const char>>)
        {
            auto array_size = v.size();
            // 32-bit size, then the payload, padded to 32-bit alignment
            return array_size + sizeof(std::uint32_t) + 
                (sizeof(std::uint32_t) - 1 - ((array_size - 1) & (sizeof(std::uint32_t) - 1)));
        }
        else
        {
            return sizeof(T);
        }
    }

    template <class T>
    inline constexpr void message_marshal_one(engine & eng, std::span<int> fds, const T & v)
    {
        constexpr zview zeroes = "\0\0";
        // TODO: array

        if constexpr (std::same_as<std::remove_cvref_t<T>, zview>)
        {
            auto wire_size = _marshal_size_one(v);
            auto str_size = static_cast<std::uint32_t>(v.size()) + 1; // account for the null terminator
            auto padding_size = wire_size - sizeof(std::uint32_t) - str_size;

            assert(padding_size < 4);

            eng.on_output({reinterpret_cast<char *>(&str_size), sizeof(str_size)}, {});
            eng.on_output({v.data(), str_size}, fds);
            eng.on_output({zeroes.data(), padding_size}, {});
        }
        else if constexpr (std::same_as<std::remove_cvref_t<T>, std::span<const char>>)
        {
            auto wire_size = _marshal_size_one(v);
            auto array_size = static_cast<std::uint32_t>(v.size());
            auto padding_size = wire_size - sizeof(std::uint32_t) - array_size;
            
            assert(padding_size < 4);

            eng.on_output({reinterpret_cast<char *>(&array_size), sizeof(array_size)}, {});
            eng.on_output(v, fds);
            eng.on_output({zeroes.data(), padding_size}, {});
        }
        else // most things used here are already binary compatible and have a size that is a multiple of 32-bit
        {
            eng.on_output({reinterpret_cast<const char *>(&v), sizeof(v)}, fds);
        }
    }

    template <class ... Args>
    inline constexpr void message_marshal(engine & eng, object_id_t id, opcode_t opcode, std::span<int> fds, Args && ... args)
    {
        std::uint32_t size = _marshal_size_one(id)
                            + _marshal_size_one(opcode)
                            + _marshal_size_one(message_size_t{})
                            + (_marshal_size_one(args) + ... + 0);

        message_marshal_one(eng, {}, id);
        if constexpr (sizeof...(args) == 0) message_marshal_one(eng, fds, (size << 16) | opcode);
        else
        {

            message_marshal_one(eng, {}, (size << 16) | opcode);

            [&]<class Tup, std::size_t ... I>(const Tup & tup, std::index_sequence<I...>){
                (((I < sizeof...(args) - 1)
                    ? message_marshal_one(eng, {}, std::get<I>(tup))
                    : message_marshal_one(eng, fds, std::get<I>(tup))), ...);
            }(std::forward_as_tuple(args...), std::make_index_sequence<sizeof...(args)>());

        }

        // message_marshal_one(eng, fds, id);
        // message_marshal_one(eng, {}, (size << 16) | opcode);
        // (message_marshal_one(eng, {}, std::forward<Args>(args)),...);
    }

    // inline constexpr void message_marshal(engine & eng, object_id_t id, opcode_t opcode, std::span<int> fds)
    // {
    //     std::uint32_t size = _marshal_size_one(id)
    //                         + _marshal_size_one(opcode)
    //                         + _marshal_size_one(message_size_t{});

    //     message_marshal_one(eng, {}, id);
    //     message_marshal_one(eng, fds, (size << 16) | opcode);
    // }

}
