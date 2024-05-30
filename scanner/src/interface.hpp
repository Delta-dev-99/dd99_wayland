#pragma once

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

    void print_fw_declaration(code_generation_context_t & ctx) const
    {
        ctx.output.format("{}struct {};\n", whitespace{ctx.indent_size * ctx.indent_level}, name);
    }

    void print_definition(code_generation_context_t & ctx) const
    {
        ctx.output.put('\n');
        if (!summary.empty())       ctx.output.format("{}// INTERFACE {}\n", whitespace{ctx.indent_size * ctx.indent_level}, original_name);
        if (!summary.empty())       ctx.output.format("{}// SUMMARY: {}\n", whitespace{ctx.indent_size * ctx.indent_level}, summary);
        if (!description.empty())   ctx.output.format("{0}/* DESCRIPTION:\n{1}\n{0}*/\n", whitespace{ctx.indent_size * ctx.indent_level}, indent_lines{description, ctx.indent_size * ctx.indent_level});


        // inherit base class and begin struct scope
        ctx.output.format(""
            "{0}struct {3} : dd99::wayland::proto::interface {{\n"
            "{1}static constexpr std::string_view interface_name{{\"{5}\"}};\n"
            "{1}static constexpr version_t interface_version = {4};\n"
            "{1}static static_data_t static_data;\n"
            "\n"
            // "{1}void parse_and_dispatch_event(iovec *, std::size_t);\n"
            // "{1}{3}(engine_t::engine_accessor _engine\n"
            // "{0}private:\n"
            "{1}{3}(engine & eng)\n"
            "{2}: dd99::wayland::proto::interface{{eng}}\n"
            "{1}{{ }}\n\n"
            "{1}void parse_and_dispatch_event(std::span<char>);\n"
            // "{0}public:\n"
            // "{1}version_t get_interface_version() override {{ return interface_version; }}\n"
            // "{1}static_data_t & get_static_data() override {{ return static_data; }}\n"
        , whitespace{ctx.indent_size * ctx.indent_level}
        , whitespace{ctx.indent_size * (ctx.indent_level + 1)}
        , whitespace{ctx.indent_size * (ctx.indent_level + 2)}
        , name
        , version
        , original_name);

        // requests (fw declarations)
        ctx.indent_level++;
        ctx.output.put('\n');
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

        // events (virtual functions)
        ctx.output.put('\n');
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
        ctx.output.format("{0}// {1:*>{2}}\n{0}// * INTERFACE {3} *\n{0}// {1:*>{2}}\n{0}", whitespace{ctx.indent_size * ctx.indent_level}, "", name.size() + 14, name);

        for (const auto & request : msg_collection_outgoing)
        {
            ctx.output.put('\n');
            request.print_definition(ctx, name);
        }

        // for (const auto & event : server_to_client_msg_collection)
        //     event.print_definition_r(ctx);
    }
};
