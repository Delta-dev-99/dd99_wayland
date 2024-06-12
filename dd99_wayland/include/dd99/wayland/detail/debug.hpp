#pragma once

#include "dd99/wayland/types.hpp"
#include <concepts>
#include <dd99/wayland/config.hpp>
#include <dd99/wayland/interface.hpp>

#include <iostream>
#include <iterator>
#include <source_location>
#include <stdexcept>
#include <format>
#include <string_view>



template <> struct std::formatter<std::span<const char>> : std::formatter<std::string_view> {
    inline /* constexpr */ auto format(const std::span<const char> & v, format_context & ctx) const
    {
        if (v.empty())
        {
            std::format_to(ctx.out(), "[0]{{}}");
            return ctx.out();
        }

        std::format_to(ctx.out(), "[{}]{{{:x}", v.size(), v.front());
        for (auto it = ++v.begin(); it != v.end(); ++it)
        {
            std::format_to(ctx.out(), ", {:x}", *it);
        }
        std::format_to(ctx.out(), "}}");

        return ctx.out();
    }
};

template <> struct std::formatter<dd99::wayland::proto::fixed_point> : std::formatter<std::string_view> {
    inline /* constexpr */ auto format(const dd99::wayland::proto::fixed_point & v, format_context & ctx) const
    {
        return std::format_to(ctx.out(), "{}", static_cast<double>(v));
    }
};


namespace dd99::wayland::dbg
{
    inline constexpr void check_version(version_t since,
                                        version_t version,
                                        std::string_view interface_name,
                                        const std::source_location& location = std::source_location::current())
    {
        if constexpr (is_version_check_enabled())
        {
            if (version < since)
                throw std::runtime_error{std::format(
                        "[DD99_WAYLAND] In file {}:{}:{}: In function {}: ERROR: Version check failed. Version: {}. Minimum required version: {}."
                    , location.file_name()
                    , location.line()
                    , location.column()
                    , location.function_name()
                    , version
                    , since
                    , interface_name)};
        }
    }


    template <class T> struct argument
    {
        std::string_view name;
        std::string_view type;
        const T & value;
    };

    // template <class T> struct new_id_wrapper
    // {
    //     T & value;
    //     auto get_id() const { return reinterpret_cast<const proto::interface &>(value).get_id(); }
    //     auto get_interface_name() const { return reinterpret_cast<const proto::interface &>(value).get_interface_name(); }
    // };
    // template <class T> struct unwrap_type { using type = T; };
    // template <class T> struct unwrap_type<new_id_wrapper<T>> { using type = T; };
    // template <class T> using unwrap_type_t = typename unwrap_type<T>::type;
    // template <class T> static constexpr bool is_new_id = false;
    // template <class T> static constexpr bool is_new_id<new_id_wrapper<T>> = true;

    enum class direction {incoming, outgoing};

    template <std::convertible_to<std::string_view> ... Args>
    inline constexpr void log_message(std::string_view interface_name
                                    , std::string_view fn_name
                                    , direction d
                                    , object_id_t id
                                    , Args ... args)
    {
        if constexpr (is_wire_debug_enabled())
        {
            std::format_to(std::ostream_iterator<char>{std::cout}, ""
                "[DD99_WAYLAND] {} {}.{}(@{}"
            , d == direction::incoming ? "<--" : "-->"
            , interface_name
            , fn_name
            , id
            );

            (std::format_to(std::ostream_iterator<char>{std::cout}, ", {}", args), ...);

            std::cout << ")\n";
        }
    }

    // template <class ... Args>
    // inline constexpr void log_message(std::string_view interface_name
    //                                 , std::string_view fn_name
    //                                 , direction d
    //                                 , object_id_t id
    //                                 , std::pair<std::string_view, Args> ... args)
    // {
    //     if constexpr (is_wire_debug_enabled())
    //     {
    //         std::format_to(std::ostream_iterator<char>{std::cout}, ""
    //             "[DD99_WAYLAND] {} {}.{}(@{}"
    //         , d == direction::incoming ? "<--" : "-->"
    //         , interface_name
    //         , fn_name
    //         , id
    //         );

    //         auto fmt_arg = [](std::string_view arg_name, const auto & arg){
    //             using arg_type = unwrap_type_t<std::remove_cvref_t<decltype(arg)>>;
    //             constexpr bool is_new_id_v              = is_new_id<std::remove_cvref_t<decltype(arg)>>;
    //             // constexpr bool is_undefined_interface_v = std::same_as<arg_type, dd99::wayland::proto::interface>;
    //             // constexpr bool is_interface_v           = !requires{ sizeof(arg_type); } || dd99::wayland::proto::Interface_C<arg_type>;

    //             if constexpr (is_new_id_v)
    //                 std::cout << std::format(", {}: new {} @{}", arg_name, arg.get_interface_name(), arg.get_id());
    //             else if constexpr (!requires{ sizeof(arg_type); }) // incomplete types assumed to be interface types
    //                 std::cout << std::format(", {}: {} @{}", arg_name, reinterpret_cast<const proto::interface &>(arg).get_interface_name(), reinterpret_cast<const proto::interface &>(arg).get_id());
    //             else if constexpr (dd99::wayland::proto::Interface_C<arg_type>) // incomplete types assumed to be interface types
    //                 std::cout << std::format(", {}: {} @{}", arg_name, reinterpret_cast<const proto::interface &>(arg).get_interface_name(), reinterpret_cast<const proto::interface &>(arg).get_id());
    //             else if constexpr 
    //                 std::cout << std::format(", {}: {}", arg_name,  arg);

    //             // if constexpr (std::same_as<arg_type, dd99::wayland::proto::interface>) std::cout << std::format("unknown@{}", arg.get_id());
    //             // else if constexpr (!requires{sizeof(arg_type);}) std::cout << std::format("{}@{}", arg.get_interface_name(), arg.get_id());
    //             // else if constexpr (dd99::wayland::proto::Interface_C<arg_type>) std::cout << std::format("{}@{}", arg.get_interface_name(), arg.get_id());
    //         };

    //         // ((std::cout << std::format(", {}@{}", args.first, args.second)), ...);
    //         (fmt_arg(args.first, args.second), ...);

    //         std::cout << ")\n";
    //     }
    // }
}
