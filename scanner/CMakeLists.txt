
find_package(pugixml REQUIRED)

set(target_name dd99_wayland_scanner)
add_executable(${target_name})
set_target_warnings(${target_name} PRIVATE)
target_compile_features(${target_name} PUBLIC cxx_std_23)
target_include_directories(${target_name} PUBLIC include)
target_sources(${target_name} PRIVATE
    src/scanner.cpp
)
target_link_libraries(${target_name} PRIVATE pugixml::pugixml)





function(dd99_add_wayland_client_protocol target_or_sources_var)
    # Parse arguments
    set(oneValueArgs PROTOCOL BASENAME)
    cmake_parse_arguments(ARGS "" "${oneValueArgs}" "" ${ARGN})

    if(ARGS_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown keywords given to ecm_add_wayland_client_protocol(): \"${ARGS_UNPARSED_ARGUMENTS}\"")
    endif()

    get_filename_component(_infile ${ARGS_PROTOCOL} ABSOLUTE)
    set(_client_header "${CMAKE_CURRENT_BINARY_DIR}/dd99-wayland-client-protocol-${ARGS_BASENAME}.hpp")
    set(_client_code "${CMAKE_CURRENT_BINARY_DIR}/dd99-wayland-client-protocol-${ARGS_BASENAME}.cpp")

    set_source_files_properties(${_client_header} GENERATED)
    set_source_files_properties(${_client_code} GENERATED)
    set_property(SOURCE ${_client_header} ${_client_code} PROPERTY SKIP_AUTOMOC ON)

    add_custom_command(OUTPUT "${_client_header}" "${_client_code}"
        COMMENT "[dd99_wayland_scanner] generating client files for ${_infile}"
        COMMAND dd99_wayland_scanner ${_infile} ${_client_header} ${_client_code}
        DEPENDS ${_infile} dd99_wayland_scanner VERBATIM)

    if (TARGET ${target_or_sources_var})
        target_sources(${target_or_sources_var} PRIVATE "${_client_header}" "${_client_code}")
    else()
        list(APPEND ${target_or_sources_var} "${_client_header}" "${_client_code}")
        set(${target_or_sources_var} ${${target_or_sources_var}} PARENT_SCOPE)
    endif()
endfunction()


function(dd99_add_wayland_server_protocol target_or_sources_var)
    # Parse arguments
    set(oneValueArgs PROTOCOL BASENAME)
    cmake_parse_arguments(ARGS "" "${oneValueArgs}" "" ${ARGN})

    if(ARGS_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown keywords given to ecm_add_wayland_server_protocol(): \"${ARGS_UNPARSED_ARGUMENTS}\"")
    endif()

    get_filename_component(_infile ${ARGS_PROTOCOL} ABSOLUTE)
    set(_server_header "${CMAKE_CURRENT_BINARY_DIR}/dd99-wayland-server-protocol-${ARGS_BASENAME}.hpp")
    set(_server_code "${CMAKE_CURRENT_BINARY_DIR}/dd99-wayland-server-protocol-${ARGS_BASENAME}.cpp")
    set_property(SOURCE ${_server_header} ${_server_code} PROPERTY SKIP_AUTOMOC ON)

    set_source_files_properties(${_server_header} GENERATED)
    set_source_files_properties(${_server_code} GENERATED)

    add_custom_command(OUTPUT "${_server_header}" "${_server_code}"
        COMMENT "[dd99_wayland_scanner] generating server files for ${_infile}"
        COMMAND dd99_wayland_scanner --server ${_infile} ${_server_header} ${_server_code}
        DEPENDS ${_infile} dd99_wayland_scanner VERBATIM)

    if (TARGET ${target_or_sources_var})
        target_sources(${target_or_sources_var} PRIVATE "${_server_header}" "${_server_code}")
    else()
        list(APPEND ${target_or_sources_var} "${_server_header}" "${_server_code}")
        set(${target_or_sources_var} ${${target_or_sources_var}} PARENT_SCOPE)
    endif()
endfunction()
