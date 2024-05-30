#pragma once

#include <cstdint>



namespace dd99::wayland
{

    using version_t = std::uint32_t;
    using object_id_t = std::uint32_t;
    using opcode_t = std::uint16_t;
    using message_size_t = std::uint16_t;

    namespace proto
    {
        struct fixed_point
        {
            constexpr explicit fixed_point(double v) : m_value{static_cast<std::int32_t>(v * 256.0)} {}
            constexpr explicit operator double() { return to_double(); }
            constexpr double to_double() { return m_value / 256.0; }

        private:
            std::int32_t m_value;
        };
    }

}
