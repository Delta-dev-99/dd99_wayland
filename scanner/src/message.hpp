#pragma once

#include "argument.hpp"
#include "element.hpp"
#include "formatting.hpp"
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
            // auto & i = 
            args.emplace_back(arg_node);
            // if (i.base_type.type == argument_type_t::type_t::TYPE_NEWID)
            //     ret_index = args.size() - 1;
        }
    }



    // std::string get_return_type_string() const
    // { return (ret_index != std::numeric_limits<std::size_t>::max()) ? args[ret_index].to_string() : "void"; }

    void print_virtual_callback_declaration(code_generation_context_t & ctx) const
    {
        print_virtual_callback_prototype(ctx);
        ctx.output.write(";\n");
    }

    void print_virtual_callback_definition(code_generation_context_t & ctx, bool outside_class = false) const
    {
        // This function generates the defautl definition for the virtual event callbacks
        // This is used to log wayland events
        print_virtual_callback_prototype(ctx, outside_class);

        if (!ctx.generate_message_logs)
        {
            ctx.output.write(" {}");
            return;
        }

        ctx.output.format("\n"
            "{}{{\n"
            "{}dbg::log_message(\"{}\", \"{}\", dbg::direction::incoming, m_object_id\n"
        , whitespace{ctx.indent_size * ctx.indent_level}
        , whitespace{ctx.indent_size * (ctx.indent_level + 1)}
        , ctx.current_interface
        , name
        );

        ctx.indent_level++;

        for (const auto & arg : args)
        {
            const bool is_new = arg.base_type == argument_type_t::TYPE_NEWID;
            const bool is_interface = arg.base_type == argument_type_t::TYPE_OBJECT;
            const bool is_string = arg.base_type == argument_type_t::TYPE_STRING;
            const bool is_int = arg.base_type == argument_type_t::TYPE_INT || arg.base_type == argument_type_t::TYPE_UINT;

            // new-id is prefixed by interface name string and version when not explicit by protocol
            if (is_new && arg.interface.empty())
            {
                ctx.output.format(""
                    "{0}, std::format(\"{1}_interface: '{{}}'\", {1}_interface.sv())\n"
                    "{0}, std::format(\"{1}_version: '{{}}'\", {1}_version)\n"
                , whitespace{ctx.indent_size * (ctx.indent_level + 1)}
                , arg.name
                );
            }

            ctx.output.format(""
                "{}, std::format(\"{}: "
            , whitespace{ctx.indent_size * (ctx.indent_level + 1)}
            , format::argument_name_cpp{ctx, arg}
            );

            if (is_new) ctx.output.write("new ");
            // if (is_string) ctx.output.write("string");
            if (arg.base_type == argument_type_t::TYPE_FD)
                ctx.output.write("[FILE] ");
            if (arg.base_type == argument_type_t::TYPE_ARRAY)
                ctx.output.write("array ");
            else if (!is_string && !is_int)
                ctx.output.format("{} ", format::argument_type_cpp{ctx, arg});

            if (is_interface || is_new) ctx.output.put('@');
            if (is_string) ctx.output.write("'{}'\", ");
            else ctx.output.write("{}\", ");

            if (is_interface)
            {
                ctx.output.write("reinterpret_cast<interface *>(");
                print_argument_name(ctx, arg);
                ctx.output.write(")->get_id()");
            }
            else if (!arg.enum_name.empty())
            {
                ctx.output.write("static_cast<std::uint32_t>(");
                print_argument_name(ctx, arg);
                ctx.output.write(")");
            }
            else if (arg.base_type == argument_type_t::TYPE_STRING)
            {
                print_argument_name(ctx, arg);
                ctx.output.write(".sv()");
            }
            else
            {
                print_argument_name(ctx, arg);
            }

            ctx.output.write(")\n");
        }
        ctx.output.format("{});\n", whitespace{ctx.indent_size * ctx.indent_level});

        ctx.indent_level--;

        // closing brace
        ctx.output.format(""
            "{0}}}\n"
        , whitespace{ctx.indent_size * ctx.indent_level});

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
            print_argument_name(ctx, arg);
            // arg.print_name(ctx);
        }
        ctx.output.put(')');
    }

    void print_declaration(code_generation_context_t & ctx) const
    {
        print_prototype(ctx, {});
        ctx.output.write(";\n");
    }

    // outgoing
    void print_definition(code_generation_context_t & ctx, bool outside_class) const
    {
        // auto has_return_type = ret_index != std::numeric_limits<std::size_t>::max();
        // auto returns_unknown_interface = has_return_type && (args[ret_index].base_type.type == argument_type_t::type_t::TYPE_NEWID) && args[ret_index].interface.empty();


        print_prototype(ctx, outside_class);

        // function body
        ctx.output.format(""
            "\n"
            "{0}{{\n"
            "{1}constexpr version_t since = {2};\n"
            "{1}constexpr opcode_t opcode = {3};\n"
            "\n"
            "{1}assert(m_object_id != 0);\n"
            "{1}dbg::check_version(since, m_version, interface_name);\n"
        , whitespace{ctx.indent_size * ctx.indent_level}
        , whitespace{ctx.indent_size * (ctx.indent_level + 1)}
        , since
        , opcode);

        ctx.indent_level++;

        // interface name assertion for undefined new ids
        for (const auto & arg : args)
        {
            if (arg.base_type == argument_type_t::TYPE_NEWID && arg.interface.empty())
            {
                ctx.output.format(""
                    "{0}assert({1}_interface == {1}.get_interface_name());\n"
                , whitespace{ctx.indent_size * ctx.indent_level}
                , format::argument_name_cpp{ctx, arg});
            }
        }

        ctx.output.put('\n');

        // count fds
        int fds_count = 0;
        for (const auto & arg : args) {
            if (arg.base_type == argument_type_t::TYPE_FD) {
                fds_count++;
            }
        }

        // create fds array
        if (fds_count > 0)
        {
            ctx.output.format(""
                "{}int fds[{}] = {{"
            , whitespace{ctx.indent_size * ctx.indent_level}
            , fds_count
            );
            { // print fd argument names
                bool is_first_arg = true;
                for (const auto & arg : args)
                {
                    if (arg.base_type != argument_type_t::TYPE_FD) continue;

                    ctx.output.format("{}{}"
                    , is_first_arg ? "" : ", "
                    , format::argument_name_cpp{ctx, arg});

                    is_first_arg = false;
                }
            }
            ctx.output.write("};\n");
        }

        // bind interfaces to new object_ids
        for (const auto & arg : args)
        {
            if (arg.base_type != argument_type_t::TYPE_NEWID) continue;

            if (arg.interface.empty())
            {
                ctx.output.format(""
                    "{0}auto new_{1} = m_engine.bind_interface({1}, {1}_version);\n"
                , whitespace{ctx.indent_size * ctx.indent_level}
                , format::argument_name_cpp{ctx, arg}
                );
            }
            else
            {
                ctx.output.format(""
                    "{0}auto new_{1} = m_engine.bind_interface({1}, m_version);\n"
                , whitespace{ctx.indent_size * ctx.indent_level}
                , format::argument_name_cpp{ctx, arg}
                );
            }
        }
        
        ctx.output.format("{}", whitespace{ctx.indent_size * ctx.indent_level});
        ctx.output.write("send_wayland_message(opcode, ");
        if (fds_count > 0) ctx.output.write("fds");
        else ctx.output.write("{}");
        // passing arguments to `send_wayland_message`
        for (const auto & arg : args)
        {
            // file descriptors are not passed as arguments here
            if (arg.base_type == argument_type_t::TYPE_FD) continue;

            ctx.output.write(", ");

            // new-id is prefixed by interface name string and version when not explicit by protocol
            if (arg.base_type == argument_type_t::TYPE_NEWID && arg.interface.empty())
            {
                ctx.output.format("{0}_interface, {0}_version, "
                , format::argument_name_cpp{ctx, arg});
            }

            if (arg.base_type == argument_type_t::TYPE_NEWID) ctx.output.write("new_");
            ctx.output.format("{}", format::argument_name_cpp{ctx, arg});
        }
        // end of `send_wayland_message` invocation
        ctx.output.write(");\n");

        if (ctx.generate_message_logs)
        {
            // log message
            ctx.output.format("\n"
                "{}dbg::log_message(\"{}\", \"{}\", dbg::direction::outgoing, m_object_id\n"
            , whitespace{ctx.indent_size * ctx.indent_level}
            , ctx.current_interface
            , name);
            // log message arguments
            for (const auto & arg : args)
            {
                const bool is_new = arg.base_type == argument_type_t::TYPE_NEWID;
                const bool is_interface = is_new || arg.base_type == argument_type_t::TYPE_OBJECT;
                const bool is_string = arg.base_type == argument_type_t::TYPE_STRING;
                const bool is_int = arg.base_type == argument_type_t::TYPE_INT || arg.base_type == argument_type_t::TYPE_UINT;

                // new-id is prefixed by interface name string and version when not explicit by protocol
                if (is_new && arg.interface.empty())
                {
                    ctx.output.format(""
                        "{0}, std::format(\"{1}_interface: '{{}}'\", {1}_interface.sv())\n"
                        "{0}, std::format(\"{1}_version: '{{}}'\", {1}_version)\n"
                    , whitespace{ctx.indent_size * (ctx.indent_level + 1)}
                    , arg.name
                    );
                }

                ctx.output.format(""
                    "{}, std::format(\""
                , whitespace{ctx.indent_size * (ctx.indent_level + 1)}
                );

                print_argument_name(ctx, arg);
                ctx.output.write(": ");

                if (is_new) ctx.output.write("new ");
                // if (is_string) ctx.output.write("string");
                if (arg.base_type == argument_type_t::TYPE_FD)
                {
                    ctx.output.write("[FILE] ");
                }
                else if (!is_string && !is_int)
                {
                    arg.print_type(ctx);
                    ctx.output.put(' ');
                }

                if (is_interface) ctx.output.put('@');
                if (is_string) ctx.output.write("'{}'\", ");
                else ctx.output.write("{}\", ");

                if (is_interface)
                {
                    ctx.output.write("reinterpret_cast<interface &>(");
                    print_argument_name(ctx, arg);
                    ctx.output.write(").get_id()");
                }
                else if (!arg.enum_name.empty())
                {
                    ctx.output.write("static_cast<std::uint32_t>(");
                    print_argument_name(ctx, arg);
                    ctx.output.write(")");
                }
                else if (arg.base_type == argument_type_t::TYPE_STRING)
                {
                    print_argument_name(ctx, arg);
                    ctx.output.write(".sv()");
                }
                else
                {
                    print_argument_name(ctx, arg);
                }

                ctx.output.write(")\n");

            }
            ctx.output.format("{});\n", whitespace{ctx.indent_size * ctx.indent_level});
        }

        ctx.indent_level--;

        // closing brace
        ctx.output.format(""
            "{0}}}\n"
        , whitespace{ctx.indent_size * ctx.indent_level});
    }

private:
    void print_argument_name(code_generation_context_t & ctx, const argument_t & arg) const
    {
        arg.print_name(ctx);
        // arg.print_name(ctx);
        // if (remove_wayland_prefix(arg.interface) == arg.name)
        // ctx.output.put('_');
    }

    void print_virtual_callback_prototype(code_generation_context_t & ctx, bool outside_class = false) const
    {
        // virtual void on_{}(arg_type arg_name, ...)
        // type transformations:
        //  newid -> object_id_t
        //  object -> object *

        ctx.output.format(""
            "{}{} void {}{}on_{}("
        , whitespace{ctx.indent_level * ctx.indent_size}
        , outside_class ? "inline" : "virtual"
        , outside_class ? ctx.current_interface : ""
        , outside_class ? "::" : ""
        , name
        );

        // ctx.output.format("virtual void on_{}(", name);
        bool first_arg = true;
        for (const auto & arg : args)
        {
            if (!first_arg) ctx.output.write(", ");
            first_arg = false;
            if (arg.base_type.type == argument_type_t::type_t::TYPE_NEWID)
                ctx.output.write("object_id_t ");
            else
            {
                ctx.output.format("{} {}"
                , format::argument_type_cpp{ctx, arg}
                , arg.base_type == argument_type_t::TYPE_OBJECT ? "* " : "");
            }
            ctx.output.format("{}", format::argument_name_cpp{ctx, arg});
        }
        ctx.output.write(")");
    }

    // indentation, return type, function name and arguments (no semicolon)
    void print_prototype(code_generation_context_t & ctx, bool outside_class = false) const
    {
        // indentation
        ctx.output.format("{}", whitespace{ctx.indent_size * ctx.indent_level});

        // inline
        if (outside_class) ctx.output.write("inline ");

        // return type
        ctx.output.write("void ");

        // function name
        if (outside_class) ctx.output.format("{}::", ctx.current_interface);
        ctx.output.write(name);
        ctx.output.put('(');

        // function arguments
        bool is_first_arg = true;
        for (const auto & argument : args)
        {
            if (!is_first_arg) ctx.output.write(", ");

            // new-id is prefixed by interface name string and version when not explicit by protocol
            if (argument.base_type == argument_type_t::TYPE_NEWID && argument.interface.empty())
            {
                ctx.output.format(""
                    "{0} {1}_interface, std::uint32_t {1}_version, "
                , argument_type_t::protocol_basic_type_to_cpp_type_string(argument_type_t::TYPE_STRING)
                , format::argument_name_cpp{ctx, argument}
                );

                // ctx.output.write(argument_type_t::protocol_basic_type_to_cpp_type_string(argument_type_t::TYPE_STRING));
                // ctx.output.put(' ');
                // print_argument_name(ctx, argument);
                // argument.print_name(ctx);
                // ctx.output.write("_interface, std::uint32_t ");
                // print_argument_name(ctx, argument);
                // argument.print_name(ctx);
                // ctx.output.write("_version, ");
            }

            bool is_ref = !argument.interface.empty() || argument.base_type.type == argument_type_t::type_t::TYPE_NEWID;

            ctx.output.format(""
                "{}{} {}"
            , format::argument_type_cpp{ctx, argument}
            , is_ref ? " &" : ""
            , format::argument_name_cpp{ctx, argument}
            );
            // argument.print_type(ctx);
            // ctx.output.put(' ');
            // if (!argument.interface.empty() || argument.base_type.type == argument_type_t::type_t::TYPE_NEWID) ctx.output.write("& ");
            // print_argument_name(ctx, argument);
            // argument.print_name(ctx);

            is_first_arg = false;
        }
        ctx.output.put(')');
    }
};
