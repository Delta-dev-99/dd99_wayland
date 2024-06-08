#pragma once

#include <dd99/wayland/engine.hpp>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <span>



namespace dd99::wayland::detail
{

    template <class T>
    inline constexpr std::size_t _marshal_size_one(T && v)
    {
        if constexpr (std::same_as<std::remove_cvref_t<T>, std::string_view>)
        {
            // 32-bit size, then the payload, padded to 32-bit alignment
            return v.size() + sizeof(std::uint32_t) + 
                (sizeof(std::uint32_t) - 1 - ((v.size() - 1) & (sizeof(std::uint32_t) - 1)));
        }
        else
        {
            return sizeof(T);
        }
    }

    template <class T>
    inline constexpr void message_marshal_one(engine & eng, std::span<int> fds, const T & v)
    {
        // TODO: array

        if constexpr (std::same_as<std::remove_cvref_t<T>, std::string_view>)
        {
            auto size = static_cast<std::uint32_t>(v.size());
            eng.on_output({reinterpret_cast<char *>(&size), sizeof(size)}, {});
            eng.on_output(v, fds);
        }
        else
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
