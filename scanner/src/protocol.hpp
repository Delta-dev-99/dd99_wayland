#pragma once

#include "formatting.hpp"
#include "interface.hpp"


struct protocol_t : element_t
{
    std::string_view copyright_str;
    std::vector<interface_t> interfaces{};
    
    explicit protocol_t(const pugi::xml_node node, bool server_side)
        : element_t(node)
        , copyright_str{node.child("copyright").text().get()}
    {
        for (const auto interface_node : node.children("interface"))
            interfaces.emplace_back(interface_node, server_side);
    }


    void print_hdr(code_generation_context_t & ctx) const
    {
        ctx.current_protocol_ptr = this;

        print_namespace_begin(ctx);
        ctx.indent_level++;

        // fw declaration of interfaces
        ctx.output.put('\n');
        for (const auto & x : interfaces) x.print_fw_declaration(ctx);

        // fw declaration of enums
        ctx.output.put('\n');
        ctx.output.format("{}namespace detail {{ // fw declaration of enums\n", whitespace{ctx.indent_size * ctx.indent_level});
        for (const auto & x : interfaces)
        for (const auto & y : x.enum_collection)
        {
            ctx.output.format(""
                "{}enum class {}__{} : std::uint32_t;\n"
            , whitespace{ctx.indent_size * (ctx.indent_level + 1)}
            , x.name
            , y.name);
        }
        ctx.output.format("{}}}// namespace detail\n", whitespace{ctx.indent_size * ctx.indent_level});

        // interface definitions
        ctx.output.put('\n');
        for (const auto & x : interfaces) x.print_definition(ctx);

        // interface function definitions
        ctx.output.put('\n');
        for (const auto & x : interfaces)
        {
            ctx.output.put('\n');
            x.print_member_definitions_section(ctx);
        }

        ctx.indent_level--;
        print_namespace_end(ctx);

        ctx.current_protocol_ptr = {};
    }

    void print_src(code_generation_context_t & ctx) const
    {
        ctx.current_protocol_ptr = this;

        print_namespace_begin(ctx);
        ctx.indent_level++;

        for (const auto & x : interfaces) x.print_src(ctx);

        ctx.indent_level--;
        print_namespace_end(ctx);

        ctx.current_protocol_ptr = {};
    }

private:
    void print_namespace_begin(code_generation_context_t & ctx) const
    {
        // TODO: sanitize comments
        // print copyright and descriptions first 
        ctx.output.format("\n{}// PROTOCOL {}\n", whitespace{ctx.indent_size * ctx.indent_level}, name);
        if (!summary.empty()) ctx.output.format("{}// SUMMARY: {}\n", whitespace{ctx.indent_size * ctx.indent_level}, summary);
        if (!description.empty()) ctx.output.format("{0}/* DESCRIPTION:\n{1}\n{0}*/\n", whitespace{ctx.indent_size * ctx.indent_level}, indent_lines{description, ctx.indent_size * ctx.indent_level});
        if (!copyright_str.empty()) ctx.output.format("{0}/* COPYRIGHT:\n{1}\n{0}*/\n", whitespace{ctx.indent_size * ctx.indent_level}, indent_lines{copyright_str, ctx.indent_size * ctx.indent_level});

        ctx.output.format("{}namespace {} {{\n", whitespace{ctx.indent_size * ctx.indent_level}, name);
    }

    void print_namespace_end(code_generation_context_t & ctx) const
    {
        ctx.output.format("\n{}}} //namespace {}\n", whitespace{ctx.indent_size * ctx.indent_level}, name);
    }
};
