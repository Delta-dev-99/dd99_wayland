#pragma once

#include "element.hpp"
#include "formatting.hpp"
#include <algorithm>
#include <cctype>
#include <string_view>


struct enum_elem_t
{
    std::string name;
    std::string_view value;
    std::string_view summary;

    enum_elem_t(const pugi::xml_node node)
        : name{node.attribute("name").value()}
        , value{node.attribute("value").value()}
        , summary{node.attribute("summary").value()}
    {
        if (is_keyword(name)) name += '_';
        if (std::isdigit(name[0])) name = '_' + name;
    }
};

struct enum_t : element_t
{
    std::vector<enum_elem_t> elems{};
    bool name_collides = false; // set to true by parent interface if name collides with requests

    enum_t(pugi::xml_node node) : element_t(node)
    {
        for (const auto entry_node : node.children("entry"))
        {
            elems.emplace_back(entry_node);
        }
    }

    void print_enum_definition(code_generation_context_t & ctx, std::string_view prefix) const
    {
        ctx.output.format(""
            "{}enum class {}__{} : std::uint32_t {{\n"
        , whitespace{ctx.indent_size * ctx.indent_level}
        , prefix
        , name);

        auto max_entry_name_size = std::max_element(elems.begin(), elems.end(), 
            [](const enum_elem_t & a, const enum_elem_t & b){ return a.name.size() < b.name.size(); })->name.size();

        auto max_entry_value_size = std::max_element(elems.begin(), elems.end(), 
            [](const enum_elem_t & a, const enum_elem_t & b){ return a.value.size() < b.value.size(); })->value.size();

        for (const auto & entry : elems)
        {
            ctx.output.format(""
                "{}{:{}} = {:{}}, // {}\n"
            , whitespace{ctx.indent_size * (ctx.indent_level + 1)}
            , entry.name
            , max_entry_name_size
            , entry.value
            , max_entry_value_size
            , entry.summary);
        }

        ctx.output.format(""
            "{}}}; // enum {}\n"
        , whitespace{ctx.indent_size * ctx.indent_level}
        , name);
    }
};

