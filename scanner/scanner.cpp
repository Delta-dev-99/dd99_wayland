
#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <pugixml.hpp>
#include <unistd.h>
#include <fcntl.h>

#include <cstdio>
#include <format>
#include <iterator>
#include <string_view>
#include <deque>
#include <vector>
#include <filesystem>



using namespace std::string_view_literals;


// used to create a formatted string and write to fd
// implements buffering using a static vector
template <class ... Args>
inline constexpr auto write_format(int fd, std::format_string<Args...> fmt, Args && ... args)
{
    static std::vector<char> buffer(1024);
    buffer.clear();

    std::vformat_to(std::back_inserter(buffer), fmt.get(), std::make_format_args(args...));
    return write(fd, buffer.data(), buffer.size());
}

// data used for code generation
struct code_generation_context_t
{
    int fd;
    int indent_size = 4; // TODO: add related option
};

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



struct element_t
{
    std::string_view original_name;
    std::string_view name;
    std::string_view summary;
    std::string_view description;

    element_t(const pugi::xml_node node)
        : original_name{node.attribute("name").value()}
        , name{original_name}
        , summary{node.child("description").attribute("summary").value()}
        , description{node.child("description").text().get()}
    {
        // remove name prefix
        if (name.starts_with("wl_") || name.starts_with("wp_"))
            name.remove_prefix(3);
    }
};

struct enum_t
{

    enum_t(pugi::xml_node)
    {

    }
};

struct argument_type_t
{
    static constexpr std::string_view type_str[]{
        "int",
        "uint",
        "fixed",
        "object",
        "new_id",
        "string",
        "array",
        "fd",
        "enum",
    };

    enum class type_t {
        TYPE_INT,
        TYPE_UINT,
        TYPE_FIXED,
        TYPE_OBJECT,
        TYPE_NEWID,
        TYPE_STRING,
        TYPE_ARRAY,
        TYPE_FD,
        TYPE_ENUM,
        TYPE_UNKNOWN, // not actually a type
    } type{type_t::TYPE_UNKNOWN};

    

    explicit argument_type_t(pugi::xml_node node)
    {
        for (int i = 0; i < static_cast<int>(std::extent<decltype(type_str)>()); ++i)
        {
            if (type_str[i] == node.attribute("type").value())
            {
                type = type_t{i};
                break;
            }
        }
    }
};

struct argument_t : element_t
{
    argument_type_t type;
    std::string_view interface;
    std::string_view enum_interface{};
    std::string_view enum_name{};
    bool allow_null;

    argument_t(pugi::xml_node node)
        : element_t{node}
        , type{node}
        , interface{node.attribute("interface").value()}
        , allow_null{node.attribute("allow-null").as_bool()}
    {
        std::string_view enum_sv = node.attribute("enum").value();
        auto dot_position = enum_sv.find_first_of('.');
        enum_interface = enum_sv.substr(0, dot_position);
        enum_name = enum_sv.substr(dot_position + 1);
    }
};

struct request_t : element_t
{
    int since;
    int opcode;
    std::deque<argument_t> args{};
    argument_t * ret{};

    request_t(const request_t &) = delete;
    void operator=(const request_t &) = delete;

    request_t(const pugi::xml_node node, int opcode_)
        : element_t{node}
        , since{node.attribute("since").as_int(1)}
        , opcode{opcode_}
    {
        for (const auto arg_node : node.children("arg"))
        {
            auto & i = args.emplace_back(arg_node);
            if (i.type.type == argument_type_t::type_t::TYPE_NEWID)
                ret = &i;
        }
    }
};

// Events and requests have the same structure.
// In this context their difference is only semantic.
using event_t = request_t;

struct interface_t : element_t
{
    int version;
    std::deque<request_t> requests{};
    std::deque<event_t> events{};
    std::deque<enum_t> enums{};
    request_t * destructor{};

    interface_t(const interface_t &) = delete;
    void operator=(const interface_t &) = delete;

    interface_t(const pugi::xml_node node)
        : element_t{node}
        , version {node.attribute("version").as_int(1)}
    {

        int opcode = 0; // request opcodes are incremental in the order of declaration
        for (auto request_node : node.children("request"))
        {
            auto & i = requests.emplace_back(request_node, opcode++);
            if (std::string_view{node.attribute("type").value()} == "destructor")
                destructor = &i;
        }

        opcode = 0;
        for (auto event_node : node.children("event"))
            events.emplace_back(event_node, opcode++);

        for (auto enum_node : node.children("enum"))
            enums.emplace_back(enum_node);
    }

    void print_fw_declaration(code_generation_context_t & ctx) const
    {
        write_format(ctx.fd, "{}struct {};\n", whitespace{ctx.indent_size * 2}, name);
    }

    void print_definition(code_generation_context_t & ctx) const
    {
        write_format(ctx.fd,"\n");
        if (!summary.empty()) write_format(ctx.fd, "{}// SUMMARY: {}\n", whitespace{ctx.indent_size * 2}, summary);
        if (!description.empty()) write_format(ctx.fd, "{0}/* DESCRIPTION:\n{1}\n{0}*/\n", whitespace{ctx.indent_size * 2}, indent_lines{description, ctx.indent_size * 2});

        // begin struct scope
        write_format(ctx.fd, "{}struct {}{{\n", whitespace{ctx.indent_size * 2}, name);



        // end struct scope
        write_format(ctx.fd, "{}}};// {}\n", whitespace{ctx.indent_size * 2}, name);
    }
};

struct protocol_t : element_t
{
    std::string_view copyright_str;
    std::deque<interface_t> interfaces{};
    
    explicit protocol_t(const pugi::xml_node node)
        : element_t(node)
        , copyright_str{node.child("copyright").text().get()}
    {
        for (const auto interface_node : node.children("interface"))
            interfaces.emplace_back(interface_node);
    }


    void print_hdr(code_generation_context_t & ctx) const
    {
        print_namespace_begin(ctx);

        write_format(ctx.fd, "\n");
        for (const auto & x : interfaces) x.print_fw_declaration(ctx);

        write_format(ctx.fd, "\n");
        for (const auto & x : interfaces) x.print_definition(ctx);

        print_namespace_end(ctx);
    }

private:
    void print_namespace_begin(code_generation_context_t & ctx) const
    {
        // TODO: sanitize comments
        // print copyright and descriptions first 
        write_format(ctx.fd, "\n\n{}// PROTOCOL {}\n", whitespace{ctx.indent_size}, name);
        if (!summary.empty()) write_format(ctx.fd, "{}// SUMMARY: {}\n", whitespace{ctx.indent_size}, summary);
        if (!description.empty()) write_format(ctx.fd, "{0}/* DESCRIPTION:\n{1}\n{0}*/\n", whitespace{ctx.indent_size}, indent_lines{description, ctx.indent_size});
        if (!copyright_str.empty()) write_format(ctx.fd, "{0}/* COPYRIGHT:\n{1}\n{0}*/\n", whitespace{ctx.indent_size}, indent_lines{copyright_str, ctx.indent_size});

        write_format(ctx.fd, "{}namespace {} {{\n", whitespace{ctx.indent_size}, name);
    }

    void print_namespace_end(code_generation_context_t & ctx) const
    {
        write_format(ctx.fd, "\n{}}} //namespace {}\n", whitespace{ctx.indent_size}, name);
    }
};


inline auto parse_document(const pugi::xml_document & doc)
{
    std::deque<protocol_t> protocols;
    
    // NOTE: xml_node is a lightweight handle, so it's ok to copy
    for (const auto protocol_node : doc.children("protocol"))
        protocols.emplace_back(protocol_node);

    return protocols;
}


// structure used to scan command-line arguments
struct scan_args
{
    bool print_version{};
    bool print_help{};

    std::string_view commandline{};
    std::vector<std::string_view> xml_file_paths{};
    std::string_view hdr_file_path{}; // header file to be generated
    std::string_view src_file_path{}; // source file to be generated
    std::vector<std::string_view> forced_includes{}; // includes added to generated header
    std::string_view main_include{}; // includes added to generated header
    bool omit_comments{}; // output comments to generated files?
    enum class visibility_t {PUBLIC, PRIVATE} visibility {visibility_t::PRIVATE};
    enum class side_t {SERVER, CLIENT} side {side_t::CLIENT};

    scan_args(int argc, char** argv)
    {
        // syntax:
        // -v  --version        print version
        // -h  --help           print help
        // [OPTIONS] <input-xml> [<more-input-xml> ...] <output-header> <output-source>
        // 
        // OPTIONS:
        //  -s  --server-side   server side
        //  -c  --no-comments   do not output comments
        //      --include=<hdr> add "#include hdr"

        commandline = argv[0];

        int i = 1; // used as index into argv for scanning

        // scan [OPTIONS] (always begin with '-')
        for (; i < argc; ++i)
        {
            std::string_view x{argv[i]};
            
            // check if this is an option (options start with '-')
            // options are only allowed before all files
            // break if we find there are no more options
            if (x[0] != '-') break; // this means there are no more options

            // check if we have double '-' (long options start with "--")
            if (x[1] == '-')
            {
                // parse long options
                auto v = x.substr(2);
                if      (v == "help") print_help = true;
                else if (v == "version") print_version = true;
                else if (v == "server_side") side = side_t::SERVER;
                else if (v == "no_comments") omit_comments = true;
                else if (v.starts_with("include="))
                {
                    if (v.size() == 8)
                    {
                        // leaving space between the option and the value is not allowed
                        print_help = true;
                        write_format(STDIN_FILENO, "Error: Whitespace not allowed after option with value: \"{}\"", v);
                    }
                    forced_includes.push_back(v.substr(8));
                }
                else if (v.starts_with("main-include="))
                {
                    if (v.size() == 13)
                    {
                        // leaving space between the option and the value is not allowed
                        print_help = true;
                        write_format(STDIN_FILENO, "Error: Whitespace not allowed after option with value: \"{}\"", v);
                    }
                    main_include{v.substr(13)};
                }
                else
                {
                    // unknown option
                    print_help = true;
                    write_format(STDIN_FILENO, "Error: Unknown option: \"{}\"\n", v);
                }
            }
            else
            {
                // parse short options (may be chained together; each char is an option)
                for (auto v : x.substr(1)) switch (v) {
                    case 'h': print_help = true; break;
                    case 'v': print_version = true; break;
                    case 's': side = side_t::SERVER; break;
                    case 'c': omit_comments = true; break;
                    default: // unknown option
                        print_help = true;
                        write_format(STDIN_FILENO, "Error: Unknown short option: \"{}\"\n", v);
                }
            }
        }

        // do not scan for files if asked for help or version
        if (print_help || print_version) return;

        // scan file arguments
        // last 2 arguments have special meaning
        for (; i < argc - 2; ++i) xml_file_paths.push_back(argv[i]);

        // check if there are not enough file arguments
        // this may happen if all arguments are options
        if (i > argc - 2)
        {
            write_format(STDIN_FILENO, "ERROR: Missing file arguments\n");
            print_help = true;
            return;
        }

        // last 2 arguments have special meaning
        hdr_file_path = argv[argc-2];
        src_file_path = argv[argc-1];
    }
};


constexpr inline void print_help(scan_args & args)
{
    write_format(STDOUT_FILENO,
        "usage: {} [OPTIONS] <input-xml> [<more-input-xml> ...] <output-hdr> <output-src>\n"
        "\n"
        "Generate c++ code and header file for wayland protocols from xml protocol specifications\n"
        "\n"
        "OPTIONS:\n"
        "    {:2}    {:15}        {}\n"
        "    {:2}    {:15}        {}\n"
        "    {:2}    {:15}        {}\n"
        "    {:2}    {:15}        {}\n"
        "    {:2}    {:15}        {}\n",
    args.commandline,
    "-h", "--help"                  , "print this help",
    "-v", "--version"               , "print version",
    "-s", "--server-side"           , "Generate server-side files. If this option is not present, client-side is assumed",
    "-c", "--no-comments"           , "Do not output comments to generated files",
    ""  , "--include=<inc>"         , "Add \"#include inc\" to generated header",
    ""  , "--main-include=<inc>"    , "Use \"#include inc\" instead of default wayland library header"
    );
}

constexpr inline void print_version() { write_format(STDOUT_FILENO, "0-alpha\n"); }

int main(int argc, char ** argv)
{
    scan_args args{argc, argv};

    if (args.print_version) print_version();
    if (args.print_help) print_help(args);
    if (args.print_help || args.print_version) return 0;

    // the collection of protocols parsed from all input xml files
    std::deque<pugi::xml_document> docs; // these own the memory most string_views point to
    std::deque<protocol_t> protocols;

    // parse each input xml file and append all parsed protocols to the collection
    for (const auto xml_file_path : args.xml_file_paths)
    {
        auto & doc = docs.emplace_back();
        doc.load_file(xml_file_path.data());

        // this function requires a zero-terminated string
        // using the string_view is ok because it points to some value from argv
        auto parsed_protocols = parse_document(doc);

        // join containers. TODO: there must be a better way
        for (auto & protocol : parsed_protocols)
            protocols.emplace_back(std::move(protocol));
    }

    std::size_t interface_count = 0; for (const auto & p : protocols) interface_count += p.interfaces.size();
    write_format(STDOUT_FILENO, "parsed {} protocols, totalling {} interfaces\n", protocols.size(), interface_count);


    // open output files
    auto hdr_fd = ::open(args.hdr_file_path.data(), O_CREAT | O_WRONLY | O_TRUNC,
        S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH);
    auto src_fd = ::open(args.src_file_path.data(), O_CREAT | O_WRONLY | O_TRUNC,
        S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH);

    // check errors
    if ((hdr_fd == -1) || (src_fd == -1))
    {
        write_format(STDERR_FILENO, "Error opening output files: {}\n", strerror(errno));
        return -1;
    }



    // * Generate header *
    // -------------------
    {
        code_generation_context_t ctx
        {
            .fd = hdr_fd,
        };

        auto main_include = args.main_include;
        if (main_include.empty()) main_include = (args.side == scan_args::side_t::CLIENT)
            ? "<dd99/wayland/client.hpp>" : "<dd99/wayland/server.hpp>";

        // header guard and default includes
        write_format(hdr_fd, ""
            "#pragma once\n"
            "#include {}\n"
        , main_include);

        // extra includes
        for (const auto x : args.forced_includes)
            write_format(hdr_fd, "#include {}\n", x);

        // begin namespace for protocols
        write_format(hdr_fd, "\n\n"
            "namespace dd99::wayland::proto{{\n");

        // generate protocols
        for (const auto & protocol : protocols)
        {
            protocol.print_hdr(ctx);
        }

        // end namespace for protocols
        write_format(hdr_fd, "\n}} // namespace dd99::wayland::proto\n");
    }


    // * Generate source *
    // -------------------
    {
        // find relative path of header file with respect to source file `hdr_rel_path` to use for "#include"
        auto hdr_path = std::filesystem::path{args.hdr_file_path}.lexically_normal();
        auto src_path = std::filesystem::path{args.src_file_path}.lexically_normal();
        // if (hdr_path.is_relative()) hdr_path = std::filesystem::current_path()/hdr_path;
        // if (src_path.is_relative()) src_path = std::filesystem::current_path()/src_path;
        auto hdr_rel_path = hdr_path.lexically_relative(src_path.parent_path());

        write_format(STDOUT_FILENO, "hdr: {}\nsrc: {}\nhdr_rel: {}\n"
                    ,hdr_path.generic_string()
                    ,src_path.generic_string()
                    ,hdr_rel_path.generic_string());

        write_format(src_fd, ""
            "#include \"{}\"\n"
        , hdr_rel_path.generic_string());
    }

}
