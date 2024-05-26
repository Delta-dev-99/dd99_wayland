#pragma once

#include <string_view>
#include <pugixml.hpp>
#include "formatting.hpp"



// base for all parsing and code generation
// that is why some parsing and generation primitives are in this header
// this structure parses common values that almost all other structures have
struct element_t
{
    std::string_view original_name;
    std::string_view name;
    std::string_view summary;
    std::string_view description;

    element_t(const pugi::xml_node node)
        : original_name{node.attribute("name").value()}
        , name{remove_wayland_prefix(original_name)}
        , summary{node.child("description").attribute("summary").value()}
        , description{node.child("description").text().get()}
    { }
};
