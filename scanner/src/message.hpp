#pragma once

#include "argument.hpp"
#include "element.hpp"
#include "formatting.hpp"
#include <cstddef>
#include <limits>
#include <unistd.h>



struct message_t : element_t
{
    int since;
    int opcode;
    std::vector<argument_t> args{};
    std::size_t ret_index = std::numeric_limits<std::size_t>::max();

    // message_t(const message_t &) = delete;
    // void operator=(const message_t &) = delete;

    message_t(const pugi::xml_node node, int opcode_)
        : element_t{node}
        , since{node.attribute("since").as_int(1)}
        , opcode{opcode_}
    {
        for (const auto arg_node : node.children("arg"))
        {
            auto & i = args.emplace_back(arg_node);
            if (i.base_type.type == argument_type_t::type_t::TYPE_NEWID)
                ret_index = args.size() - 1;
        }
    }



    // std::string get_return_type_string() const
    // { return (ret_index != std::numeric_limits<std::size_t>::max()) ? args[ret_index].to_string() : "void"; }

    void print_callback_signature(code_generation_context_t & ctx) const
    {
        ctx.output.write("void (*)(void * userdata");
        for (const auto & arg : args)
        {
            ctx.output.write(", ");
            arg.print_type(ctx);
            ctx.output.put(' ');
            arg.print_name(ctx);
        }
        ctx.output.put(')');
    }

    void print_declaration(code_generation_context_t & ctx) const
    {
        print_prototype(ctx, {});
        ctx.output.write(";\n");
    }

    // outgoing
    void print_definition(code_generation_context_t & ctx, std::string_view scope = {}) const
    {
        auto has_return_type = ret_index != std::numeric_limits<std::size_t>::max();
        auto returns_unknown_interface = has_return_type && (args[ret_index].base_type.type == argument_type_t::type_t::TYPE_NEWID) && args[ret_index].interface.empty();


        print_prototype(ctx, scope);
        
        // function body
        ctx.output.format(""
            "\n"
            "{0}{{\n"
            "{1}constexpr version_t since = {2};\n"
            "{1}constexpr opcode_t opcode = {3};\n"
        , whitespace{ctx.indent_size * ctx.indent_level}
        , whitespace{ctx.indent_size * (ctx.indent_level + 1)}
        , since
        , opcode);
        if (has_return_type) 
        {
            ctx.output.format("\n{}", whitespace{ctx.indent_size * (ctx.indent_level + 1)});
            if (returns_unknown_interface)
            {
                ctx.output.write("auto [new_id, new_interface] = m_engine.create_interface<T>();\n");
            }
            else
            {
                ctx.output.write("auto [new_id, new_interface] = m_engine.create_interface<");
                args[ret_index].print_type(ctx);
                ctx.output.write(">();\n");
            }
            // ctx.output.format("{}const auto new_id = new_interface.get_id();\n", whitespace{ctx.indent_size * (ctx.indent_level + 1)});
        }
        ctx.output.format(""
            "\n"
            "{0}dbg_check_version(since, static_data.server_supported_version, interface_name);\n"
            "\n"
            "{0}send_wayland_message(opcode"
        , whitespace{ctx.indent_size * (ctx.indent_level + 1)});
        if (has_return_type) ctx.output.write(", new_id");
        // passing arguments to `send_wayland_message`
        for (const auto & arg : args)
        {
            if (arg.base_type.type == argument_type_t::type_t::TYPE_NEWID) continue;
            ctx.output.write(", ");
            arg.print_name(ctx);
        }
        // end of `send_wayland_message` invocation
        ctx.output.write(");\n");
        // returning 
        if (has_return_type)
        {
            ctx.output.format("{}return new_interface;\n", whitespace{ctx.indent_size * (ctx.indent_level + 1)});
        }
        // closing brace
        ctx.output.format(""
            "{0}}}\n"
        , whitespace{ctx.indent_size * ctx.indent_level});
    }

private:
    // indentation, return type, function name and arguments (no semicolon)
    void print_prototype(code_generation_context_t & ctx, std::string_view scope = {}) const
    {
        auto has_return_type = ret_index != std::numeric_limits<std::size_t>::max();
        auto returns_unknown_interface = has_return_type && (args[ret_index].base_type.type == argument_type_t::type_t::TYPE_NEWID) && args[ret_index].interface.empty();

        // indentation
        ctx.output.format("{}", whitespace{ctx.indent_size * ctx.indent_level});

        // make template in this case
        if (returns_unknown_interface)
            ctx.output.write("template <Interface_C T> ");

        // inline
        if (!scope.empty()) ctx.output.write("inline ");

        //return type
        if (returns_unknown_interface) ctx.output.put('T');
        else if (has_return_type) args[ret_index].print_type(ctx);
        else ctx.output.write("void");

        // function name
        if (!scope.empty()) ctx.output.format(" {}::", scope);
        else ctx.output.put(' ');
        ctx.output.write(name);
        ctx.output.put('(');


        // function arguments
        bool is_first_arg = true;
        for (const auto & argument : args)
        {
            if (argument.base_type.type == argument_type_t::type_t::TYPE_NEWID)
                continue;

            if (!is_first_arg) ctx.output.write(", ");
            argument.print_type(ctx);
            ctx.output.put(' ');
            argument.print_name(ctx);

            is_first_arg = false;
        }
        ctx.output.put(')');
    }
};