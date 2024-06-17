#pragma once

#include "element.hpp"
#include "formatting.hpp"
#include <algorithm>
#include <format>
#include <string_view>



struct argument_type_t
{
    static constexpr std::string_view protocol_type_string[]{
        "int",
        "uint",
        "fixed",
        "object",
        "new_id",
        "string",
        "array",
        "fd",
        "enum",
    };

    enum underlying_type_t {
        T_INT,
        T_UINT,
        T_FIXED,
        T_OBJECT,
        T_NEWID,
        T_STRING,
        T_ARRAY,
        T_FD,
        T_ENUM,
        T_UNKNOWN, // not actually a type
    };


    static constexpr std::string_view protocol_basic_type_to_cpp_type_string(underlying_type_t type)
    {
        {
            switch (type) {
                case underlying_type_t::T_INT:     return "std::int32_t";
                case underlying_type_t::T_UINT:    return "std::uint32_t";
                case underlying_type_t::T_FIXED:   return "proto::fixed_point";
                case underlying_type_t::T_STRING:  return "proto::zview";
                case underlying_type_t::T_ARRAY:   return "std::span<const char>";
                case underlying_type_t::T_FD:      return "int";
                // case type_t::TYPE_ENUM:    return "std::int32_t";
                default: return "SCANNER__UNKNOWN_TYPE";
            }
        }
    }
    

    constexpr argument_type_t(underlying_type_t v) noexcept
        : m_type{v}
    { }

    explicit argument_type_t(pugi::xml_node node)
    {
        for (int i = 0; i < static_cast<int>(std::extent<decltype(protocol_type_string)>()); ++i)
        {
            if (node.attribute("type").value() == protocol_type_string[i])
            {
                m_type = static_cast<underlying_type_t>(i);
                break;
            }
        }
    }


    constexpr std::string_view protocol_basic_type_to_cpp_type_string() const
    { return protocol_basic_type_to_cpp_type_string(m_type); }

    constexpr bool operator==(argument_type_t v) const noexcept { return m_type == v.m_type; }
    // constexpr bool operator==(underlying_type_t v) const noexcept { return m_type == v; }


    underlying_type_t m_type{T_UNKNOWN}; // NOTE: this is the only data member
};


struct argument_t : element_t, private argument_type_t
{
    // argument_type_t base_type;
    std::string_view interface;
    std::string_view enum_interface{};
    std::string_view enum_name{};
    bool allow_null;

    argument_t(pugi::xml_node node)
        : element_t{node}
        , argument_type_t{node}
        , interface{node.attribute("interface").value()}
        , allow_null{node.attribute("allow-null").as_bool()}
    {
        std::string_view enum_sv = node.attribute("enum").value();
        auto dot_position = enum_sv.find_first_of('.');
        if (dot_position != enum_sv.npos) enum_interface = enum_sv.substr(0, dot_position);
        enum_name = enum_sv.substr(dot_position + 1);
    }

    argument_t(element_t && base_element_
             , argument_type_t base_type_
             , bool allow_null_ = false
             , std::string_view interface_ = {}
             , std::string_view enum_interface_ = {}
             , std::string_view enum_name_ = {})
        : element_t{std::move(base_element_)}
        , argument_type_t{base_type_}
        , interface{interface_}
        , enum_interface{enum_interface_}
        , enum_name{enum_name_}
        , allow_null{allow_null_}
    { }

    // get type
    constexpr argument_type_t & type() noexcept { return *this; }
    constexpr const argument_type_t & type() const noexcept { return *this; }

    constexpr bool is_fd() const noexcept { return type() == T_FD; }
    constexpr bool is_array() const noexcept { return type() == T_ARRAY; }
    constexpr bool is_string() const noexcept { return type() == T_STRING; }
    constexpr bool is_enum() const noexcept { return type() == T_ENUM || !enum_name.empty(); } // some enums use 'T_UINT' type tag
    constexpr bool is_new_interface() const noexcept { return type() == T_NEWID; }
    constexpr bool is_unspecified_new_interface() const noexcept { return is_new_interface() && interface.empty(); }
    constexpr bool is_existent_interface() const noexcept { return type() == T_OBJECT; }
    constexpr bool is_interface() const noexcept { return is_new_interface() || is_existent_interface(); }

    constexpr bool can_ommit_type_in_log() const noexcept { return type() == T_STRING || type() == T_INT || type() == T_UINT || type() == T_FIXED; }


    void print_type(code_generation_context_t & ctx) const;
    // {
        // if (is_enum())
        // {
        //     auto enum_interface_ = remove_wayland_prefix(enum_interface);
        //     if (enum_interface_.empty()) enum_interface_ = reinterpret_cast<const element_t *>(ctx.current_interface_ptr)->name;

        //     ctx.output.format("detail::{}__{}", enum_interface_, enum_name);
        //     // ctx.output.write("detail::");
        //     // ctx.output.write(enum_interface_);
        //     // ctx.output.write("__");
        //     // ctx.output.write(enum_name);
        // }
        // else if (is_interface())
        // {
        //     if (interface.empty()) ctx.output.write("interface");
        //     else if (ctx.external_inerface_names.contains(interface)) ctx.output.format("{}::{}", extern_interface_protocol_name(interface), remove_wayland_prefix(interface));
        //     else ctx.output.write(remove_wayland_prefix(interface));
        // }
        // else ctx.output.write(type().protocol_basic_type_to_cpp_type_string());

        // auto is_reference = (base_type.type == argument_type_t::type_t::TYPE_OBJECT);
        // if (is_reference) ctx.output.write(" &");
    // }

    void print_name(code_generation_context_t & ctx) const;
    // {
    //     ctx.output.write(name);

    //     if (type() == T_OBJECT ||
    //         remove_wayland_prefix(interface) == name)
    //         ctx.output.put('_');
    // }

};




namespace format
{
    struct argument_type_cpp
    {
        code_generation_context_t & ctx;
        const argument_t & arg;
    };

    struct argument_name_cpp
    {
        code_generation_context_t & ctx;
        const argument_t & arg;
    };
}

template <> struct std::formatter<format::argument_type_cpp> : std::formatter<std::string_view> {
    inline constexpr auto format(const format::argument_type_cpp & v, format_context & ctx) const
    {
        const auto & arg = v.arg;
        const auto & current_protocol = * reinterpret_cast<const element_t *>(v.ctx.current_protocol_ptr);
        const auto & current_interface = * reinterpret_cast<const element_t *>(v.ctx.current_interface_ptr);

        if (arg.is_enum())
        {
            auto enum_interface = remove_wayland_prefix(arg.enum_interface);
            if (enum_interface.empty()) enum_interface = reinterpret_cast<const element_t *>(v.ctx.current_interface_ptr)->name;

            std::format_to(ctx.out(), "detail::{}__{}", enum_interface, arg.enum_name);
        }
        else if (arg.is_interface())
        {
            auto interface = remove_wayland_prefix(arg.interface);
            if (interface.empty()) interface = "interface";

            if (v.ctx.external_inerface_names.contains(arg.interface)) {
                // external interface names are inside another namespace
                // TODO: Use name index for this case too
                std::format_to(ctx.out(), "{}::{}", extern_interface_protocol_name(arg.interface), interface);
            } else {
                // check for name collisions
                if (v.ctx.name_index[current_protocol.name][current_interface.name].contains(interface)) {
                    // the current interface defines a function with the same name as `interface` and causes a collision
                    // `interface` must be qualified (using namespace and scope specifier)
                    auto it = std::find_if(v.ctx.name_index.begin(), v.ctx.name_index.end(), [&](const auto & x){ return x.second.contains(interface); });
                    if (it != v.ctx.name_index.end()) std::format_to(ctx.out(), "{}::", it->first);
                }
                std::format_to(ctx.out(), "{}", interface);
            }
        }
        else std::format_to(ctx.out(), "{}", arg.type().protocol_basic_type_to_cpp_type_string());

        return ctx.out();
    }
};

template <> struct std::formatter<format::argument_name_cpp> : std::formatter<std::string_view> {
    inline constexpr auto format(const format::argument_name_cpp & v, format_context & ctx) const
    {
        const auto & arg = v.arg;

        bool name_collides = arg.is_interface() || remove_wayland_prefix(arg.interface) == arg.name;

        std::format_to(ctx.out(), "{}{}", arg.name, name_collides ? "_" : "");

        return ctx.out();
    }
};




inline void argument_t::print_type(code_generation_context_t & ctx) const { ctx.output.format("{}", format::argument_type_cpp{ctx, *this}); }
inline void argument_t::print_name(code_generation_context_t & ctx) const { ctx.output.format("{}", format::argument_name_cpp{ctx, *this}); }
