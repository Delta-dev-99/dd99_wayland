
// #include <algorithm>
// #include <cerrno>
// #include <cstddef>
// #include <cstring>
// #include <pugixml.hpp>
// #include <unistd.h>
#include <fcntl.h>

#include <cstring>
// #include <cstdio>
// #include <format>
// #include <iterator>
// #include <string_view>
// #include <deque>
// #include <vector>
#include <filesystem>
#include <format>
#include <iostream>
#include <iterator>
#include <set>
#include <string_view>
#include <tuple>
#include <unistd.h>
#include <utility>

#include "formatting.hpp"
#include "protocol.hpp"



using namespace std::string_view_literals;




// inline auto parse_document(const pugi::xml_document & doc)
// {
//     std::vector<protocol_t> protocols;
    
//     // NOTE: xml_node is a lightweight handle, so it's ok to copy
//     for (const auto protocol_node : doc.children("protocol"))
//         protocols.emplace_back(protocol_node);

//     return protocols;
// }


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
    bool generate_message_logs = true;

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
                else if (v == "server") side = side_t::SERVER;
                else if (v == "no-comments") omit_comments = true;
                else if (v == "no-message-logs") generate_message_logs = false;
                else if (v.starts_with("include="))
                {
                    if (v.size() == 8)
                    {
                        // leaving space between the option and the value is not allowed
                        print_help = true;
                        std::format_to(std::ostream_iterator<char>{std::cerr}, "Error: Whitespace not allowed after option with value: \"{}\"", v);
                    }
                    forced_includes.push_back(v.substr(8));
                }
                else if (v.starts_with("main-include="))
                {
                    if (v.size() == 13)
                    {
                        // leaving space between the option and the value is not allowed
                        print_help = true;
                        std::format_to(std::ostream_iterator<char>{std::cerr}, "Error: Whitespace not allowed after option with value: \"{}\"", v);
                    }
                    main_include = v.substr(13);
                }
                else
                {
                    // unknown option
                    print_help = true;
                    std::format_to(std::ostream_iterator<char>{std::cerr}, "Error: Unknown option: \"{}\"\n", v);
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
                        std::format_to(std::ostream_iterator<char>{std::cerr}, "Error: Unknown short option: \"{}\"\n", v);
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
            std::format_to(std::ostream_iterator<char>{std::cerr}, "ERROR: Missing file arguments\n");
            print_help = true;
            return;
        }

        // last 2 arguments have special meaning
        hdr_file_path = argv[argc-2];
        src_file_path = argv[argc-1];
    }
};


inline void print_help(scan_args & args)
{
    std::format_to(std::ostream_iterator<char>{std::cout},
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
        "    {:2}    {:15}        {}\n",
    args.commandline,
    "-h", "--help"                  , "print this help",
    "-v", "--version"               , "print version",
    "-s", "--server"                , "Generate server-side files. If this option is not present, client-side is assumed",
    "-c", "--no-comments"           , "Do not output comments to generated files",
    ""  , "--no-message-logs"       , "Do not generate logging code for wayland messages",
    ""  , "--include=<inc>"         , "Add \"#include inc\" to generated header",
    ""  , "--main-include=<inc>"    , "Use \"#include inc\" instead of default dd99 wayland library header"
    );
}

inline void print_version() { std::format_to(std::ostream_iterator<char>{std::cout}, "0-alpha\n"); }

int main(int argc, char ** argv)
{
    scan_args args{argc, argv};

    if (args.print_version) print_version();
    if (args.print_help) print_help(args);
    if (args.print_help || args.print_version) return 0;

    std::format_to(std::ostream_iterator<char>{std::cout}, "Generating header \"{}\" and source \"{}\" from protocols:", args.hdr_file_path, args.src_file_path);
    if (args.xml_file_paths.size() == 1) std::format_to(std::ostream_iterator<char>{std::cout}, " {}", args.xml_file_paths.front());
    else for (const auto x : args.xml_file_paths) std::format_to(std::ostream_iterator<char>{std::cout}, "\n  - {}", x);
    *std::ostream_iterator<char>{std::cout} = '\n';

    // the collection of protocols parsed from all input xml files
    std::vector<pugi::xml_document> docs; // these own the memory most string_views point to
    std::vector<protocol_t> protocols;

    // map (protocol name) -> {map (interface name) -> {set (defined names)}}
    std::map<std::string_view, std::map<std::string_view, std::set<std::string_view>>> name_index;


    // parse each input xml file and append all parsed protocols to the collection
    for (const auto xml_file_path : args.xml_file_paths)
    {
        auto & doc = docs.emplace_back();
        doc.load_file(xml_file_path.data());
    
        // NOTE: xml_node is a lightweight handle, so it's ok to copy
        for (const auto protocol_node : doc.children("protocol"))
        {
            // parse protocols, interfaces, messages, etc
            auto & protocol = protocols.emplace_back(protocol_node, args.side == scan_args::side_t::SERVER);

            // index names to avoid name collisions (xml specification contains quite a few)
            auto [it, b] = name_index.emplace(std::piecewise_construct, std::make_tuple(std::string_view{protocol.name}), std::make_tuple()); // index protocol name
            for (const auto & interface : protocol.interfaces)
            {
                auto [it2, b2] = it->second.emplace(std::piecewise_construct, std::make_tuple(std::string_view{interface.name}), std::make_tuple()); // index interface name

                // index function names inside interface
                for (const auto & msg : interface.msg_collection_outgoing) it2->second.emplace(msg.name);
            }
        }
    }

    std::size_t interface_count = 0; for (const auto & p : protocols) interface_count += p.interfaces.size();
    std::format_to(std::ostream_iterator<char>{std::cout}, "parsed {} protocols, totalling {} interfaces\n", protocols.size(), interface_count);


    // open output files
    auto hdr_fd = ::open(args.hdr_file_path.data(), O_CREAT | O_WRONLY | O_TRUNC,
        S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH);
    auto src_fd = ::open(args.src_file_path.data(), O_CREAT | O_WRONLY | O_TRUNC,
        S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH);

    // check errors
    if ((hdr_fd == -1) || (src_fd == -1))
    {
        std::format_to(std::ostream_iterator<char>{std::cerr}, "Error opening output files: {}\n", strerror(errno));
        return -1;
    }

    using buf_t = char[4*1024];
    buf_t hdr_buf;
    buf_t src_buf;
    fd_buffered_output hdr_buffered_output{.m_fd = hdr_fd, .m_buf = hdr_buf, .m_capacity = sizeof(hdr_buf)};
    fd_buffered_output src_buffered_output{.m_fd = src_fd, .m_buf = src_buf, .m_capacity = sizeof(src_buf)};



    std::set<std::string_view> external_interface_names;
    // find all interface names used as arguments
    for (const auto & d : docs)
    for (const auto & proto : d.children("protocol"))
    for (const auto & interf : proto.children("interface"))
    for (const auto & interf_child : interf.children())
    {
        if (std::string_view{interf_child.name()} != "event" && 
            std::string_view{interf_child.name()} != "request")
            continue;

        for (const auto & arg : interf_child.children("arg"))
        {
            std::string_view iname = arg.attribute("interface").value();
            if (!iname.empty()) external_interface_names.insert(iname);
        }
    }
    // remove names of all interfaces defined in the parsed files
    for (const auto & p : protocols)
    for (const auto & interf : p.interfaces)
    {
        external_interface_names.erase(interf.original_name);
    }
    // print some info
    std::format_to(std::ostream_iterator<char>{std::cout}, "found {} external interface references\n", external_interface_names.size());



    // * Generate header *
    // -------------------
    {
        code_generation_context_t ctx
        {
            .output = hdr_buffered_output,
            .generate_message_logs = args.generate_message_logs,
            .external_inerface_names = external_interface_names,
            .protocols = protocols,
            .name_index = name_index,
        };

        auto main_include = args.main_include;
        if (main_include.empty())
        {
            // main_include = (args.side == scan_args::side_t::CLIENT) ? "<dd99/wayland/interface.hpp>" : "<dd99/wayland/wayland_server.hpp>";
            main_include = "<dd99/wayland/interface.hpp>";
        }

        // header guard and default includes
        ctx.output.format(""
            "#pragma once\n"
            "#include {}\n"
        , main_include);

        // extra includes
        for (const auto x : args.forced_includes)
            ctx.output.format("#include {}\n", x);

        // begin namespace for protocols
        ctx.output.format("\n\n"
            "namespace dd99::wayland::proto{{\n\n");


        // forward declarations of interfaces from other, non-parsed, protocols
        {
            // for (const auto & p : protocols)
            // for (const auto & interf : p.interfaces)
            // {
            //     for (const auto & msg : interf.msg_collection_incoming)
            //     {
            //         for (const auto & arg : msg.args)
            //     }
            //     for (const auto & msg : interf.msg_collection_outgoing)
            //     {

            //     }
            // }

            if (!external_interface_names.empty()) ctx.output.write("// fw-declarations for external interfaces\n");
            for (const auto & x : external_interface_names)
            {
                ctx.output.format(""
                    "{}namespace {} {{ struct {}; }}\n"
                , whitespace{ctx.indent_size * ctx.indent_level}
                , extern_interface_protocol_name(x)
                , remove_wayland_prefix(x));
            }
            if (!external_interface_names.empty()) ctx.output.put('\n');
        }

        // generate protocols
        for (const auto & protocol : protocols)
        {
            protocol.print_hdr(ctx);
        }

        // end namespace for protocols
        ctx.output.format("\n}} // namespace dd99::wayland::proto\n");
    }


    // * Generate source *
    // -------------------
    {
        code_generation_context_t ctx
        {
            .output = src_buffered_output,
            .generate_message_logs = args.generate_message_logs,
            .external_inerface_names = external_interface_names,
            .protocols = protocols,
            .name_index = name_index,
        };

        // find relative path of header file with respect to source file `hdr_rel_path` to use for "#include"
        auto hdr_path = std::filesystem::path{args.hdr_file_path}.lexically_normal();
        auto src_path = std::filesystem::path{args.src_file_path}.lexically_normal();
        // if (hdr_path.is_relative()) hdr_path = std::filesystem::current_path()/hdr_path;
        // if (src_path.is_relative()) src_path = std::filesystem::current_path()/src_path;
        auto hdr_rel_path = hdr_path.lexically_relative(src_path.parent_path());

        ctx.output.format(""
            "#include \"{}\"\n"
        , hdr_rel_path.generic_string());

        // begin namespace for protocols
        ctx.output.write("\n\n"
            "namespace dd99::wayland::proto{\n");

        for (const auto & protocol : protocols)
        {
            protocol.print_src(ctx);
        }

        // end namespace for protocols
        ctx.output.write("\n"
            "} // namespace dd99::wayland::proto\n");
    }

}
