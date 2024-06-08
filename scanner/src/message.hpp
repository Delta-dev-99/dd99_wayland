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
    // std::size_t ret_index = std::numeric_limits<std::size_t>::max();

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
            // if (i.base_type.type == argument_type_t::type_t::TYPE_NEWID)
            //     ret_index = args.size() - 1;
        }
    }



    // std::string get_return_type_string() const
    // { return (ret_index != std::numeric_limits<std::size_t>::max()) ? args[ret_index].to_string() : "void"; }

    void print_virtual_callback(code_generation_context_t & ctx) const
    {
        ctx.output.format("virtual void on_{}(", name);
        bool first_arg = true;
        for (const auto & arg : args)
        {
            if (!first_arg) ctx.output.write(", ");
            first_arg = false;
            if (arg.base_type.type == argument_type_t::type_t::TYPE_NEWID)
                ctx.output.write("object_id_t ");
            else
            {
                arg.print_type(ctx);
                ctx.output.put(' ');

                if (arg.base_type.type == argument_type_t::type_t::TYPE_OBJECT)
                    ctx.output.write("* ");
            }
            arg.print_name(ctx);
        }
        ctx.output.write(") {}");
    }

    void print_callback_signature(code_generation_context_t & ctx) const
    {
        ctx.output.write("void (void * userdata");
        for (const auto & arg : args)
        {
            ctx.output.write(", ");
            arg.print_type(ctx);
            ctx.output.put(' ');
            if (arg.base_type.type == argument_type_t::type_t::TYPE_OBJECT || arg.base_type.type == argument_type_t::type_t::TYPE_NEWID)
                ctx.output.write("& ");
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
        // auto has_return_type = ret_index != std::numeric_limits<std::size_t>::max();
        // auto returns_unknown_interface = has_return_type && (args[ret_index].base_type.type == argument_type_t::type_t::TYPE_NEWID) && args[ret_index].interface.empty();


        print_prototype(ctx, scope);

        // TODO: support ancillary fd
        
        // function body
        ctx.output.format(""
            "\n"
            "{0}{{\n"
            "{1}constexpr version_t since = {2};\n"
            "{1}constexpr opcode_t opcode = {3};\n"
            "\n"
            "{1}assert(m_object_id != 0);\n"
            "{1}dbg_check_version(since, static_data.server_supported_version, interface_name);\n"
            "\n"
        , whitespace{ctx.indent_size * ctx.indent_level}
        , whitespace{ctx.indent_size * (ctx.indent_level + 1)}
        , since
        , opcode);

        for (const auto & arg : args)
        {
            if (arg.base_type.type != argument_type_t::type_t::TYPE_NEWID) continue;

            ctx.output.format("{}", whitespace{ctx.indent_size * (ctx.indent_level + 1)});
            ctx.output.write("auto new_");
            arg.print_name(ctx);
            ctx.output.write(" = m_engine.bind_interface(");
            arg.print_name(ctx);
            ctx.output.write(");\n");
        }
        // if (has_return_type)
        // {
        //     ctx.output.format("{}", whitespace{ctx.indent_size * (ctx.indent_level + 1)});
        //     ctx.output.write("auto new_id = m_engine.bind_interface(");
        //     args[ret_index].print_name(ctx);
        //     ctx.output.write(");\n\n");
        // }
        ctx.output.format("{}", whitespace{ctx.indent_size * (ctx.indent_level + 1)});
        ctx.output.write("send_wayland_message(opcode, {}");
        // if (has_return_type) ctx.output.write(", new_id");
        // passing arguments to `send_wayland_message`
        for (const auto & arg : args)
        {
            ctx.output.write(", ");
            if (arg.base_type.type == argument_type_t::type_t::TYPE_NEWID) ctx.output.write("new_");
            arg.print_name(ctx);
        }
        // end of `send_wayland_message` invocation
        ctx.output.write(");\n");
        // closing brace
        ctx.output.format(""
            "{0}}}\n"
        , whitespace{ctx.indent_size * ctx.indent_level});
    }

private:
    // indentation, return type, function name and arguments (no semicolon)
    void print_prototype(code_generation_context_t & ctx, std::string_view scope = {}) const
    {
        // auto has_return_type = ret_index != std::numeric_limits<std::size_t>::max();
        // auto returns_interface = has_return_type && (args[ret_index].base_type.type == argument_type_t::type_t::TYPE_NEWID);
        // auto returns_unknown_interface = returns_interface && args[ret_index].interface.empty();

        // indentation
        ctx.output.format("{}", whitespace{ctx.indent_size * ctx.indent_level});

        // make template in this case
        // if (returns_unknown_interface)
        //     ctx.output.write("template <Interface_C T> ");
        // else if (returns_interface)
        //     ctx.output.format("template <std::derived_from<{}> T> ", remove_wayland_prefix(args[ret_index].interface));

        // inline
        if (!scope.empty()) ctx.output.write("inline ");

        // return type
        ctx.output.write("void ");

        // function name
        if (!scope.empty()) ctx.output.format("{}::", scope);
        ctx.output.write(name);
        ctx.output.put('(');

        // function arguments
        bool is_first_arg = true;
        for (const auto & argument : args)
        {
            // if (argument.base_type.type == argument_type_t::type_t::TYPE_NEWID)
            //     continue;

            if (!is_first_arg) ctx.output.write(", ");
            argument.print_type(ctx);
            ctx.output.put(' ');
            if (!argument.interface.empty() || argument.base_type.type == argument_type_t::type_t::TYPE_NEWID) ctx.output.write("& ");
            argument.print_name(ctx);

            is_first_arg = false;
        }
        ctx.output.put(')');
    }
};
