#pragma once

#include "element.hpp"


struct enum_t : element_t
{
    enum_t(pugi::xml_node node) : element_t(node) { }
};
