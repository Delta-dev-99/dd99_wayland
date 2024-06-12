#pragma once

#include "element.hpp"
#include "formatting.hpp"
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

    enum class type_t {
        TYPE_INT,
        TYPE_UINT,
        TYPE_FIXED,
        TYPE_OBJECT,
        TYPE_NEWID,
        TYPE_STRING,
        TYPE_ARRAY,
        TYPE_FD,
        TYPE_ENUM,
        TYPE_UNKNOWN, // not actually a type
    } type{type_t::TYPE_UNKNOWN}; // NOTE: this is the only data member

    static constexpr type_t TYPE_INT = type_t::TYPE_INT;
    static constexpr type_t TYPE_UINT = type_t::TYPE_UINT;
    static constexpr type_t TYPE_FIXED = type_t::TYPE_FIXED;
    static constexpr type_t TYPE_OBJECT = type_t::TYPE_OBJECT;
    static constexpr type_t TYPE_NEWID = type_t::TYPE_NEWID;
    static constexpr type_t TYPE_STRING = type_t::TYPE_STRING;
    static constexpr type_t TYPE_ARRAY = type_t::TYPE_ARRAY;
    static constexpr type_t TYPE_FD = type_t::TYPE_FD;
    static constexpr type_t TYPE_ENUM = type_t::TYPE_ENUM;
    static constexpr type_t TYPE_UNKNOWN = type_t::TYPE_UNKNOWN;


    static constexpr std::string_view protocol_basic_type_to_cpp_type_string(type_t type)
    {
        {
            switch (type) {
                case type_t::TYPE_INT:     return "std::int32_t";
                case type_t::TYPE_UINT:    return "std::uint32_t";
                case type_t::TYPE_FIXED:   return "dd99::wayland::proto::fixed_point";
                case type_t::TYPE_STRING:  return "zview";
                case type_t::TYPE_ARRAY:   return "std::span<const char>";
                case type_t::TYPE_FD:      return "int";
                // case type_t::TYPE_ENUM:    return "std::int32_t";
                default: return "SCANNER__UNKNOWN_TYPE";
            }
        }
    }

    constexpr std::string_view protocol_basic_type_to_cpp_type_string() const
    { return protocol_basic_type_to_cpp_type_string(type); }

    

    explicit argument_type_t(pugi::xml_node node)
    {
        for (int i = 0; i < static_cast<int>(std::extent<decltype(protocol_type_string)>()); ++i)
        {
            if (node.attribute("type").value() == protocol_type_string[i])
            {
                type = type_t{i};
                break;
            }
        }
    }

    constexpr bool operator==(type_t v) const noexcept { return type == v; }
};


struct argument_t : element_t
{
    argument_type_t base_type;
    std::string_view interface;
    std::string_view enum_interface{};
    std::string_view enum_name{};
    bool allow_null;

    argument_t(pugi::xml_node node)
        : element_t{node}
        , base_type{node}
        , interface{node.attribute("interface").value()}
        , allow_null{node.attribute("allow-null").as_bool()}
    {
        std::string_view enum_sv = node.attribute("enum").value();
        auto dot_position = enum_sv.find_first_of('.');
        if (dot_position != enum_sv.npos) enum_interface = enum_sv.substr(0, dot_position);
        enum_name = enum_sv.substr(dot_position + 1);
    }


    void print_type(code_generation_context_t & ctx) const
    {
        auto is_enum = [&](){ return base_type.type == argument_type_t::type_t::TYPE_ENUM || !enum_name.empty(); };
        auto is_interface = [&](){ return base_type.type == argument_type_t::type_t::TYPE_NEWID || base_type.type == argument_type_t::type_t::TYPE_OBJECT; };


        if (is_enum())
        {
            auto enum_interface_ = remove_wayland_prefix(enum_interface);
            if (enum_interface_.empty()) enum_interface_ = ctx.current_interface;

            ctx.output.write("detail::");
            ctx.output.write(enum_interface_);
            ctx.output.write("__");
            ctx.output.write(enum_name);
        }
        else if (is_interface())
        {
            if (interface.empty()) ctx.output.write("interface");
            else if (ctx.external_inerface_names.contains(interface)) ctx.output.format("{}::{}", extern_interface_protocol_name(interface), remove_wayland_prefix(interface));
            else ctx.output.write(remove_wayland_prefix(interface));
        }
        else ctx.output.write(base_type.protocol_basic_type_to_cpp_type_string());

        // auto is_reference = (base_type.type == argument_type_t::type_t::TYPE_OBJECT);
        // if (is_reference) ctx.output.write(" &");
    }

    void print_name(code_generation_context_t & ctx) const
    {
        ctx.output.write(name);

        if (base_type == argument_type_t::TYPE_OBJECT ||
            remove_wayland_prefix(interface) == name)
            ctx.output.put('_');
    }

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

        bool is_enum = arg.base_type == argument_type_t::TYPE_ENUM || !arg.enum_name.empty();
        bool is_interface = arg.base_type == argument_type_t::TYPE_NEWID || arg.base_type == argument_type_t::TYPE_OBJECT;

        if (is_enum)
        {
            auto enum_interface = remove_wayland_prefix(arg.enum_interface);
            if (enum_interface.empty()) enum_interface = v.ctx.current_interface;

            std::format_to(ctx.out(), "detail::{}__{}", enum_interface, arg.enum_name);
        }
        else if (is_interface)
        {
            auto interface = remove_wayland_prefix(arg.interface);
            if (interface.empty()) interface = "interface";

            if (v.ctx.external_inerface_names.contains(arg.interface))
                std::format_to(ctx.out(), "{}::{}", extern_interface_protocol_name(arg.interface), interface);
            else std::format_to(ctx.out(), "{}", interface);
        }
        else std::format_to(ctx.out(), "{}", arg.base_type.protocol_basic_type_to_cpp_type_string());

        return ctx.out();
    }
};

template <> struct std::formatter<format::argument_name_cpp> : std::formatter<std::string_view> {
    inline constexpr auto format(const format::argument_name_cpp & v, format_context & ctx) const
    {
        const auto & arg = v.arg;

        bool name_collides = arg.base_type == argument_type_t::TYPE_OBJECT || remove_wayland_prefix(arg.interface) == arg.name;

        std::format_to(ctx.out(), "{}{}", arg.name, name_collides ? "_" : "");

        return ctx.out();
    }
};
