#pragma once

#include <cerrno>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <unistd.h>
#include <algorithm>
#include <format>
#include <utility>
#include <vector>
#include <map>


// used to create a formatted string and write to fd
// implements buffering using a static vector
// template <class ... Args>
// inline constexpr auto write_format(int fd, std::format_string<Args...> fmt, Args && ... args)
// {
//     static std::vector<char> buffer(1024);
//     buffer.clear();

//     std::vformat_to(std::back_inserter(buffer), fmt.get(), std::make_format_args(args...));
//     return write(fd, buffer.data(), buffer.size());
// }


// wl_display => display.
// wp_shm => shm.
inline constexpr std::string_view remove_wayland_prefix(std::string_view name)
{
    if (name.starts_with("wl_") || name.starts_with("wp_"))
        name.remove_prefix(3);
    return name;
}

inline constexpr std::string_view extern_interface_protocol_name(std::string_view interface_name)
{
    if (interface_name.starts_with("wl_")) return "wayland";
    else if (interface_name.starts_with("xdg_")) return "xdg_shell";
    else return "unknown_protocol";
}

inline bool is_keyword(std::string_view str)
{
    static const std::set<std::string_view> keywords =
    {
        "alignas","alignof","and","and_eq","asm","auto","bitand","bitor","bool","break","case","catch","char","char8_t","char16_t","char32_t","class","compl","concept","const","consteval","constexpr","constinit","const_cast","continue","co_await","co_return","co_yield","decltype","default","delete","do","double","dynamic_cast","else","enum","explicit","export","extern","false","float","for","friend","goto","if","inline","int","long","mutable","namespace","new","noexcept","not","not_eq","nullptr","operator","or","or_eq","private","protected","public","register","reinterpret_cast","requires","return","short","signed","sizeof","static","static_assert","static_cast","struct","switch","template","this","thread_local","throw","true","try","typedef","typeid","typename","union","unsigned","using","virtual","void","volatile","wchar_t","while","xor","xor_eq",
    };

    return keywords.contains(str);
}


// used with std::format
struct indent_lines{
    std::string_view str;
    int indent;
};
template <> struct std::formatter<indent_lines> : std::formatter<std::string_view> {
    inline constexpr auto format(const indent_lines & v, format_context & ctx) const
    {
        auto strv = v.str;

        std::size_t pos;
        while ((pos = strv.find_first_of('\n')) != std::string_view::npos)
        {
            std::fill_n(ctx.out(), v.indent, ' ');
            std::copy_n(strv.begin(), pos + 1, ctx.out());
            strv.remove_prefix(pos + 1);
        }

        if (!strv.empty())
        {
            std::fill_n(ctx.out(), v.indent, ' ');
            std::copy_n(strv.begin(), strv.size(), ctx.out());
        }

        return ctx.out();
    }
};

// used with std::format
struct whitespace{
    int n;
};
template <> struct std::formatter<whitespace> : std::formatter<std::string_view> {
    inline constexpr auto format(const whitespace & v, format_context & ctx) const
    {
        std::fill_n(ctx.out(), v.n, ' ');
        return ctx.out();
    }
};



struct fd_buffered_output
{
    int m_fd;
    char * m_buf;
    std::size_t m_capacity;
    std::size_t m_size{};


public:

    ~fd_buffered_output()
    {
        while(m_size) flush();
    }
    
    
public:

    // write the data to file. Also handles the case of partial write
    std::size_t flush()
    {
        if (m_size == 0) return 0; // nothing to flush
        auto n = write_file(m_buf, m_size); // write buffered data to file
        if (n == m_size) [[likely]] m_size = 0; // check if full buffer was written. clear the buffer
        else [[unlikely]] consume(n); // in case of partial write, erase elements from buffer
        return n;
    }

    // returns the number of bytes written to the underlying file
    // will return 0 most of the time due to buffering
    std::size_t write(const char * data, std::size_t count)
    {
        std::size_t n = 0;

        // too much data to buffer. Just flush and write directly.
        if (count > m_capacity) [[unlikely]]
        {
            n += flush();

            // check if flush() actually wrote the full buffer
            if (m_size == 0) [[likely]]
            {
                auto _n = write_file(data, count);   // write new data directly to file
                n += _n;
                // just in case of partial write: recurse the write() function (this one).
                if (_n < count) [[unlikely]] return n + write(data + _n, count - _n);
                else return n;
            }
        }

        //calculate free space in buffer
        auto freespace = m_capacity - m_size;
        auto _n = std::min(freespace, count);
        
        // write as much data as possible to buffer
        std::copy_n(data, _n, m_buf + m_size);
        m_size += _n;

        // if there is more data to write, flush and recurse
        if (count - _n > 0) [[unlikely]]
        {
            n += flush();
            return n + write(data + _n, count - _n);
        }
        else return n;
    }

    std::size_t write(std::string_view str)
    { return write(str.data(), str.size()); }

    // returns the number of bytes written to the underlying file
    // will return 0 most of the time
    std::size_t put(char v)
    {
        std::size_t n = 0;
        if (m_capacity == m_size) n += flush();
        if (m_capacity == m_size) throw std::runtime_error{"failed to flush buffer"};
        m_buf[m_size++] = v;
        return n;
    }


private:
    struct iterator
    {
        using difference_type = std::ptrdiff_t;

        fd_buffered_output * buf;

        // iterator(const iterator &) = default;
        // iterator(iterator &&) = default;
        // iterator(fd_buffer * _buf) : buf{_buf} {}
        // iterator & operator=(const iterator &) = default;

        struct write_on_assign
        {
            fd_buffered_output * buf;
            const write_on_assign & operator=(char v) const { buf->put(v); return *this; }
        };

        write_on_assign operator*() const { return {buf}; }
        iterator & operator++() { return *this; }
        iterator operator++(int) { return *this; }
    };
    
    iterator get_output_iterator() { return {this}; }


public:
    // shorthand for formatting into the buffered output
    template <class ...  Args>
    auto format(std::format_string<Args...> fmt, Args && ... args)
    { return std::format_to(get_output_iterator(), fmt, std::forward<Args>(args)...); }


private:
    // erase elements from begining of buffer
    void consume(std::size_t count)
    {
        std::copy(m_buf + count, m_buf + m_size, m_buf);
        m_size -= count;
    }

    // wrap ::write calls with error check
    std::size_t write_file(const char * data, std::size_t count)
    {
        auto n = ::write(m_fd, data, count); // write data
        if (n == -1) [[unlikely]] throw std::system_error{errno, std::system_category(), "write error"};   // check for error
        return static_cast<std::size_t>(n);
    }

};


// fw_declaration
struct protocol_t;
struct interface_t;


// data used for code generation
struct code_generation_context_t
{
    fd_buffered_output & output;
    int indent_size = 4; // TODO: add related option
    int indent_level = 0;
    
    bool generate_message_logs;

    const std::set<std::string_view> & external_inerface_names;
    const std::vector<protocol_t> & protocols;

    const protocol_t * current_protocol_ptr{};
    const interface_t * current_interface_ptr{};

    // map (protocol name) -> {map (interface name) -> {set (defined names)}}
    std::map<std::string_view, std::map<std::string_view, std::set<std::string_view>>> & name_index;

    // std::string_view current_protocol{};
    // std::string_view current_interface{};
};
