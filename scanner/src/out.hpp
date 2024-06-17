#pragma once
#include <dd99/wayland/interface.hpp>


namespace dd99::wayland::proto{


// PROTOCOL wayland
/* COPYRIGHT:

    Copyright © 2008-2011 Kristian Høgsberg
    Copyright © 2010-2011 Intel Corporation
    Copyright © 2012-2013 Collabora, Ltd.

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation files
    (the "Software"), to deal in the Software without restriction,
    including without limitation the rights to use, copy, modify, merge,
    publish, distribute, sublicense, and/or sell copies of the Software,
    and to permit persons to whom the Software is furnished to do so,
    subject to the following conditions:

    The above copyright notice and this permission notice (including the
    next paragraph) shall be included in all copies or substantial
    portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
    BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
    ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
  
*/
namespace wayland {

    struct display;
    struct registry;
    struct callback;
    struct compositor;
    struct shm_pool;
    struct shm;
    struct buffer;
    struct data_offer;
    struct data_source;
    struct data_device;
    struct data_device_manager;
    struct shell;
    struct shell_surface;
    struct surface;
    struct seat;
    struct pointer;
    struct keyboard;
    struct touch;
    struct output;
    struct region;
    struct subcompositor;
    struct subsurface;

    namespace detail { // fw declaration of enums
        enum class display__error : std::uint32_t;
        enum class shm__error : std::uint32_t;
        enum class shm__format : std::uint32_t;
        enum class data_offer__error : std::uint32_t;
        enum class data_source__error : std::uint32_t;
        enum class data_device__error : std::uint32_t;
        enum class data_device_manager__dnd_action : std::uint32_t;
        enum class shell__error : std::uint32_t;
        enum class shell_surface__resize : std::uint32_t;
        enum class shell_surface__transient : std::uint32_t;
        enum class shell_surface__fullscreen_method : std::uint32_t;
        enum class surface__error : std::uint32_t;
        enum class seat__capability : std::uint32_t;
        enum class seat__error : std::uint32_t;
        enum class pointer__error : std::uint32_t;
        enum class pointer__button_state : std::uint32_t;
        enum class pointer__axis : std::uint32_t;
        enum class pointer__axis_source : std::uint32_t;
        enum class pointer__axis_relative_direction : std::uint32_t;
        enum class keyboard__keymap_format : std::uint32_t;
        enum class keyboard__key_state : std::uint32_t;
        enum class output__subpixel : std::uint32_t;
        enum class output__transform : std::uint32_t;
        enum class output__mode : std::uint32_t;
        enum class subcompositor__error : std::uint32_t;
        enum class subsurface__error : std::uint32_t;
    }// namespace detail



    // INTERFACE wl_display
    // SUMMARY: core global object
    /* DESCRIPTION:
    
          The core global object.  This is a special singleton object.  It
          is used for internal Wayland protocol features.
        
    */
    struct display : dd99::wayland::proto::interface {
    public: // interface constants
        static constexpr std::string_view interface_name{"wl_display"};
        static constexpr version_t interface_version = 1;

    public: // virtual getters
        std::string_view get_interface_name() const override { return interface_name; }

    public: // constructor
        display(engine & eng)
            : dd99::wayland::proto::interface{eng}
        { }
  