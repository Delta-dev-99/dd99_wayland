#pragma once

#include "element.hpp"
#include "formatting.hpp"
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


    constexpr std::string_view protocol_basic_type_to_cpp_type_string() const
    {
        // if (const_ref_view)
        {
            switch (type) {
                case type_t::TYPE_INT:     return "std::int32_t";
                case type_t::TYPE_UINT:    return "std::uint32_t";
                case type_t::TYPE_FIXED:   return "dd99::wayland::proto::fixed_point";
                case type_t::TYPE_STRING:  return "std::string_view";
                case type_t::TYPE_ARRAY:   return "std::span<std::int32_t>";
                case type_t::TYPE_FD:      return "int";
                // case type_t::TYPE_ENUM:    return "std::int32_t";
                default: return "SCANNER__UNKNOWN_TYPE";
            }
        }
        // else
        // {
        //     switch (type) {
        //         case type_t::TYPE_INT:     return "std::int32_t";
        //         case type_t::TYPE_UINT:    return "std::uint32_t";
        //         case type_t::TYPE_FIXED:   return "dd99::wayland::proto::fixed_point";
        //         case type_t::TYPE_STRING:  return "std::string";
        //         case type_t::TYPE_ARRAY:   return "std::vector<std::int32_t>";
        //         case type_t::TYPE_FD:      return "int";
        //         // case type_t::TYPE_ENUM:    return "std::int32_t";
        //         default: return "SCANNER__UNKNOWN_TYPE";
        //     }
        // }
    }

    

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
        enum_interface = enum_sv.substr(0, dot_position);
        enum_name = enum_sv.substr(dot_position + 1);
    }


    void print_type(code_generation_context_t & ctx) const
    {
        auto is_enum = [&](){ return base_type.type == argument_type_t::type_t::TYPE_ENUM; };
        auto is_interface = [&](){ return base_type.type == argument_type_t::type_t::TYPE_NEWID || base_type.type == argument_type_t::type_t::TYPE_OBJECT; };

        if (is_enum()) ctx.output.format("{}::{}", enum_interface, enum_name);
        else if (is_interface()) ctx.output.write(interface.empty() ? "interface" : remove_wayland_prefix(interface));
        else ctx.output.write(base_type.protocol_basic_type_to_cpp_type_string());

        // auto is_reference = (base_type.type == argument_type_t::type_t::TYPE_OBJECT);
        // if (is_reference) ctx.output.write(" &");
    }

    void print_name(code_generation_context_t & ctx) const
    {
        ctx.output.write(name);

        if (base_type.type == argument_type_t::type_t::TYPE_OBJECT)
            ctx.output.put('_');
    }


    // std::string to_string() const
    // {
    //     std::string type;

    //     auto is_enum = [&](){ return base_type.type == argument_type_t::type_t::TYPE_ENUM; };
    //     auto is_interface = [&](){ return base_type.type == argument_type_t::type_t::TYPE_NEWID || base_type.type == argument_type_t::type_t::TYPE_OBJECT; };

    //     // type string
    //     if (is_enum())  type = std::format("{}::{}", enum_interface, enum_name);
    //     else if (is_interface())    type = (!interface.empty()) ? remove_wayland_prefix(interface) : "interface";
    //     else type = base_type.protocol_basic_type_to_cpp_type_string();

    //     auto is_reference = is_interface() && (base_type.type == argument_type_t::type_t::TYPE_OBJECT);

    //     return std::format("{} {}{}{}"
    //     , type
    //     , is_reference ? "& " : ""
    //     , name
    //     , name == type ? "_" : "");
    // }
};
