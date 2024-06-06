#pragma once

#include "argument.hpp"
#include "element.hpp"
#include "formatting.hpp"
#include "message.hpp"
#include "enumeration.hpp"
#include <cstddef>
#include <format>
#include <limits>
#include <string_view>



struct interface_t : element_t
{
    int version;
    std::vector<message_t> msg_collection_outgoing{};
    std::vector<message_t> msg_collection_incoming{};
    std::vector<enum_t> enum_collection{};
    struct {
        enum {client_to_server, server_to_client} side;
        std::size_t index = std::numeric_limits<std::size_t>::max();
    } destructor{};
    // message_t * destructor{};

    // interface_t(const interface_t &) = delete;
    // void operator=(const interface_t &) = delete;

    interface_t(const pugi::xml_node node)
        : element_t{node}
        , version {node.attribute("version").as_int(1)}
    {

        int opcode = 0; // request opcodes are incremental in the order of declaration
        for (auto request_node : node.children("request"))
        {
            msg_collection_outgoing.emplace_back(request_node, opcode++);
            if (std::string_view{node.attribute("type").value()} == "destructor")
                destructor = {destructor.client_to_server, msg_collection_outgoing.size() - 1};
        }

        opcode = 0;
        for (auto event_node : node.children("event"))
            msg_collection_incoming.emplace_back(event_node, opcode++);

        for (auto enum_node : node.children("enum"))
            enum_collection.emplace_back(enum_node);
    }

    void print_src(code_generation_context_t & ctx) const
    {
        if (msg_collection_incoming.empty()) return;

        ctx.output.format(""
            "\n\n"
            "{0}// {2:*>{3}}\n"
            "{0}// * INTERFACE {4} *\n"
            "{0}// {2:*>{3}}\n"
            "\n"
            "{0}{4}::static_data_t {4}::static_data{{}};\n"
            "\n"
            "{0}void {4}::parse_and_dispatch_event(std::span<const char> buffer)\n"
            "{0}{{\n"
            "{1}object_id_t obj_id;\n"
            "{1}message_size_t msg_size;\n"
            "{1}opcode_t code;\n"
            "{1}\n"
            "{1}assert(buffer.size() >= 8); // header size\n"
            "{1}\n"
            "{1}auto data_words = reinterpret_cast<const std::uint32_t *>(buffer.data());\n"
            "{1}obj_id = data_words[0];\n"
            "{1}msg_size = data_words[1] >> 16;\n"
            "{1}code = data_words[1] & ((1 << 16) - 1);\n"
            "{1}\n"
            "{1}assert(obj_id == m_object_id);\n"
            "{1}\n"
            "{1}buffer = buffer.subspan(8); // skip header\n"
            "{1}\n"
            "{1}switch(code){{\n"
            // "{0}}}\n"
        , whitespace{ctx.indent_size * ctx.indent_level}
        , whitespace{ctx.indent_size * (ctx.indent_level + 1)}
        , ""
        , name.size() + 14
        , name);

        ctx.indent_level += 2;
        
        for (const auto & msg: msg_collection_incoming)
        {
            ctx.output.format(""
                "{0}case {1}:"
                // "{0}{{\n"
                // "{1}"
                // "{1}auto && ["
            , whitespace{ctx.indent_size * ctx.indent_level}
            // , whitespace{ctx.indent_size * (ctx.indent_level + 1)}
            , msg.opcode);

            if (msg.args.empty()) ctx.output.format(" on_{}(); ", msg.name);
            else
            {
                ctx.output.format("{{\n"
                    "{1}auto && ["
                , whitespace{ctx.indent_size * ctx.indent_level}
                , whitespace{ctx.indent_size * (ctx.indent_level + 1)});

                // parse arguments
                { // argument names
                    bool is_first_arg = true;
                    for (const auto & arg : msg.args)
                    {
                        if (!is_first_arg) ctx.output.write(", ");
                        arg.print_name(ctx);
                        is_first_arg = false;
                    }
                }
                ctx.output.write("] = parse_msg_args<");
                { // argument types
                    bool is_first_arg = true;
                    for (const auto & arg : msg.args)
                    {
                        if (!is_first_arg) ctx.output.write(", ");
                        if (arg.base_type.type == argument_type_t::type_t::TYPE_NEWID || arg.base_type.type == argument_type_t::type_t::TYPE_OBJECT)
                        {
                            ctx.output.write("object_id_t");
                        }
                        else arg.print_type(ctx);
                        is_first_arg = false;
                    }
                }
                ctx.output.write(">(buffer);\n");
                
                // lookup object ids
                for (const auto & arg : msg.args)
                {
                    if (arg.base_type.type == argument_type_t::type_t::TYPE_OBJECT)
                    {
                        ctx.output.format(""
                            "{}auto "
                        , whitespace{ctx.indent_size * (ctx.indent_level + 1)});
                        arg.print_name(ctx);
                        ctx.output.write("ptr = m_engine.get_interface");
                        if (!arg.interface.empty())
                        {
                            ctx.output.put('<');
                            arg.print_type(ctx);
                            ctx.output.put('>');
                        }
                        ctx.output.put('(');
                        arg.print_name(ctx);
                        ctx.output.write(");\n");
                    }
                }

                ctx.output.format(""
                    "{}on_{}("
                , whitespace{ctx.indent_size * (ctx.indent_level + 1)}
                , msg.name);
                { // argument names
                    bool is_first_arg = true;
                    for (const auto & arg : msg.args)
                    {
                        if (!is_first_arg) ctx.output.write(", ");
                        arg.print_name(ctx);
                        if (arg.base_type.type == argument_type_t::type_t::TYPE_OBJECT) ctx.output.write("ptr");
                        is_first_arg = false;
                    }
                }

                ctx.output.format(");\n"
                    "{}}} " // break;
                , whitespace{ctx.indent_size * ctx.indent_level}
                , whitespace{ctx.indent_size * (ctx.indent_level + 1)}
                , msg.name);

                // close brackets before "break;"
                // ctx.output.format(""
                //     "{}}} " // break;
                // , whitespace{ctx.indent_size * ctx.indent_level});
            }

            ctx.output.format(""
                // "{0}}}\n"
                // "{0}"
                "break;\n"
            , whitespace{ctx.indent_size * ctx.indent_level}
            , whitespace{ctx.indent_size * (ctx.indent_level + 1)}
            , msg.opcode);
        }

        ctx.indent_level -= 2;

        ctx.output.format(""
            "{1}}}\n"
            "{0}}}\n"
        , whitespace(ctx.indent_size * ctx.indent_level)
        , whitespace(ctx.indent_size * (ctx.indent_level + 1))
        , name);
    }

    void print_fw_declaration(code_generation_context_t & ctx) const
    {
        ctx.output.format("{}struct {};\n", whitespace{ctx.indent_size * ctx.indent_level}, name);
    }

    void print_definition(code_generation_context_t & ctx) const
    {
        ctx.output.write("\n\n");
        if (!summary.empty())       ctx.output.format("{}// INTERFACE {}\n", whitespace{ctx.indent_size * ctx.indent_level}, original_name);
        if (!summary.empty())       ctx.output.format("{}// SUMMARY: {}\n", whitespace{ctx.indent_size * ctx.indent_level}, summary);
        if (!description.empty())   ctx.output.format("{0}/* DESCRIPTION:\n{1}\n{0}*/\n", whitespace{ctx.indent_size * ctx.indent_level}, indent_lines{description, ctx.indent_size * ctx.indent_level});


        // inherit base class and begin struct scope
        ctx.output.format(""
            "{0}struct {3} : dd99::wayland::proto::interface {{\n"
            "{0}public: // interface constants\n"
            "{1}static constexpr std::string_view interface_name{{\"{5}\"}};\n"
            "{1}static constexpr version_t interface_version = {4};\n"
            "{1}\n"
            "{0}public: // static interface data\n"
            "{1}static static_data_t static_data;\n"
            "\n"
            "{0}public: // constructor\n"
            "{1}{3}(engine & eng)\n"
            "{2}: dd99::wayland::proto::interface{{eng}}\n"
            "{1}{{ }}\n"
            "{1}\n"
            "{0}public: // used internally for dispatching events\n"
            "{1}void parse_and_dispatch_event(std::span<const char>){6}\n"
            "{1}\n"
            "{0}public: // API requests\n"
        , whitespace{ctx.indent_size * ctx.indent_level}
        , whitespace{ctx.indent_size * (ctx.indent_level + 1)}
        , whitespace{ctx.indent_size * (ctx.indent_level + 2)}
        , name
        , version
        , original_name
        , msg_collection_incoming.empty() ? " {} // no events" : ";");

        // requests (fw declarations)
        ctx.indent_level++;
        for (const auto & request : msg_collection_outgoing)
            request.print_declaration(ctx);

        // events (signatures)
        // ctx.output.put('\n');
        // for (const auto & event : msg_collection_incoming)
        // {
        //     ctx.output.format("{}using {}_cb_sig_t = ", whitespace{ctx.indent_size * ctx.indent_level}, event.name);
        //     event.print_callback_signature(ctx);
        //     ctx.output.write(";\n");
        // }
        // ctx.indent_level--;

        ctx.output.format(""
            "\n{}public: // API events\n"
        , whitespace{ctx.indent_size * (ctx.indent_level - 1)});

        // events (virtual functions)
        for (const auto & event : msg_collection_incoming)
        {
            ctx.output.format("{}", whitespace{ctx.indent_level * ctx.indent_size});
            event.print_virtual_callback(ctx);
            ctx.output.put('\n');
        }
        ctx.indent_level--;

        // for (const auto & event : server_to_client_msg_collection)
        //     event.print_declaration_r(ctx);

        // end struct scope
        ctx.output.format("{}}};// {}\n", whitespace{ctx.indent_size * ctx.indent_level}, name);
    }

    void print_function_definitions(code_generation_context_t & ctx) const
    {
        ctx.output.format(""
            "{0}// {1:*>{2}}\n"
            "{0}// * INTERFACE {3} *\n"
            "{0}// {1:*>{2}}\n"
            "{0}"
        , whitespace{ctx.indent_size * ctx.indent_level}
        , ""
        , name.size() + 14
        , name);

        for (const auto & request : msg_collection_outgoing)
        {
            ctx.output.put('\n');
            request.print_definition(ctx, name);
        }

        // for (const auto & event : server_to_client_msg_collection)
        //     event.print_definition_r(ctx);
    }
};
