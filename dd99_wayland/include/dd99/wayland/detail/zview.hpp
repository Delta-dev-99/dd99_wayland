#pragma once

#include <string_view>


namespace dd99::wayland::proto
{
    
    // a string_view that's guaranteed to be zero-terminated or empty
    // *** intended to be used mainly as raw data() in C function calls
    // the null-terminator is not counted in the size()
    // the null-terminator is also not iterable in the range [begin - end)
    template <class CharT, class TraitsT = std::char_traits<CharT>>
    struct basic_zview
    {
        using sv_type = std::basic_string_view<CharT, TraitsT>;

        using traits_type =            typename sv_type::traits_type;
        using value_type =             typename sv_type::value_type;
        using pointer =                typename sv_type::pointer;
        using const_pointer =          typename sv_type::const_pointer;
        using reference =              typename sv_type::reference;
        using const_reference =        typename sv_type::const_reference;
        using const_iterator =         typename sv_type::const_iterator;
        using iterator =               typename sv_type::iterator;
        using const_reverse_iterator = typename sv_type::const_reverse_iterator;
        using reverse_iterator =       typename sv_type::reverse_iterator;
        using size_type =              typename sv_type::size_type;
        using difference_type =        typename sv_type::difference_type;

        static constexpr auto npos = sv_type::npos;

    public:
        // can be converted to string_view for most stuff (like comparisons)
        [[nodiscard]] constexpr operator sv_type() const noexcept { return m_sv; }
        [[nodiscard]] constexpr auto sv() const noexcept { return m_sv; }

        constexpr basic_zview() noexcept
            : m_sv{}
        { }

        constexpr basic_zview(const basic_zview &) noexcept = default;

        [[gnu::nonnull]]
        constexpr basic_zview(const CharT * str) noexcept
            : m_sv(str)
        { }

        constexpr basic_zview(const CharT * str, size_type str_size) noexcept
            : m_sv(str, str_size)
        { }

        [[nodiscard]] constexpr auto begin() const noexcept { return m_sv.begin(); }
        [[nodiscard]] constexpr auto end() const noexcept { return m_sv.end(); }
        [[nodiscard]] constexpr auto cbegin() const noexcept { return m_sv.cbegin(); }
        [[nodiscard]] constexpr auto cend() const noexcept { return m_sv.cend(); }
        [[nodiscard]] constexpr auto rbegin() const noexcept { return m_sv.rbegin(); }
        [[nodiscard]] constexpr auto rend() const noexcept { return m_sv.rend(); }
        [[nodiscard]] constexpr auto crbegin() const noexcept { return m_sv.crbegin(); }
        [[nodiscard]] constexpr auto crend() const noexcept { return m_sv.crend(); }

        [[nodiscard]] constexpr auto size() const noexcept { return m_sv.size(); }
        [[nodiscard]] constexpr auto length() const noexcept { return m_sv.length(); }
        [[nodiscard]] constexpr auto max_size() const noexcept { return m_sv.max_size() - 1; }
        [[nodiscard]] constexpr auto empty() const noexcept { return m_sv.empty(); }

        [[nodiscard]] constexpr auto & operator[](size_type pos) const noexcept { return m_sv[pos]; }
        
        [[nodiscard]] constexpr auto data() const noexcept { return m_sv.data(); }

    private:
        sv_type m_sv;
    };

    using zview = basic_zview<char>;

}
