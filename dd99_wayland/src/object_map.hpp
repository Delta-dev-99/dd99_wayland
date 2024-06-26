#pragma once

// #include "dd99/wayland/engine.hpp"
#include <dd99/wayland/types.hpp>
// #include "dd99/wayland/interface.hpp"
#include <cassert>
#include <cstddef>
#include <limits>
#include <memory>
#include <stack>
#include <stdexcept>
#include <sys/types.h>
#include <type_traits>
#include <vector>

// private header
// to be used only in library implementation code



namespace dd99::wayland::detail
{

    template <class Tp, class Sequence = std::vector<Tp>>
    struct clearable_stack : std::stack<Tp, Sequence>
    {
        using base = std::stack<Tp, Sequence>;

        void clear() { base::c.clear(); }
    };


    // A wrapper around std::vector<T*>.
    // Acts as a map<Id_T, T*> where keys are automatically allocated sequentially.
    // Erasing elements uses a freelist and does not affect other elements (constant time, no invalidation except iterator to erased element).
    // Template argument `Base_ID` is the Id of the first element. All element Ids are offset by this value.
    template <class T, class ID_T, ID_T Base_ID>
    struct object_map
    {
        using key_type = ID_T;
        using mapped_type = T*;
        using value_type = T*;
        using size_type = std::make_unsigned<key_type>;
        using difference_type = std::make_signed<key_type>;
        using reference = value_type &;
        using const_reference = const value_type &;
        using underlying_t = std::vector<value_type>;

        static constexpr key_type base_key = Base_ID;

        struct iterator
        {
            object_map * container;
            underlying_t::iterator current;

            iterator(object_map * _container, underlying_t::iterator _current)
                : container{_container}
                , current{_current}
            { }
            iterator(object_map * _container, key_type key)
                : container{_container}
                , current{container->m_objects.begin() + key - base_key}
            { }

            using difference_type = std::ptrdiff_t;

            constexpr iterator & operator++() { advance_to_next(); return *this; }
            constexpr iterator operator++(int) { auto tmp = *this; advance_to_next(); return tmp; }
            constexpr reference operator*() const { return *current; }
            constexpr key_type get_key() const { return base_key + static_cast<key_type>(current - container->m_objects.begin()); }


        private:
            constexpr void advance_to_next()
            {
                do if (++current == container->m_objects.end()) return;
                while (!*current);
            }
        };
        

    public:
        // check if key is within container bounds
        constexpr bool is_in_range(key_type key)
        {
            return key_bounds_check(key);
        }

        // constexpr const_reference operator[](key_type key) const
        // {
        //     assert(key >= Base);
        //     assert(key < m_objects.size());
        //     return m_objects[key - Base];
        // }
        constexpr reference operator[](key_type key)
        {
            assert(key_bounds_check(key));
            return *iterator{this, key};
        }

        constexpr reference at(key_type key)
        {
            auto & r = operator[](key);

            // check if object is present in map
            // !r translates to a call to std::unique_ptr<T>::operator bool
            if (!key_bounds_check(key) || !r)
                throw std::out_of_range{"dd99::wayland::object_map: access out of bounds"};

            return r;
        }

        constexpr bool empty() { return m_objects.empty(); }
        constexpr size_type size() { return m_objects.size(); }
        constexpr size_type max_size() { return std::min(m_objects.max_size(), std::numeric_limits<key_type>::max()); }

        constexpr void clear() { m_objects.clear(); m_freelist.clear(); }
        constexpr iterator insert(value_type && x)
        {
            key_type new_key;

            if (!m_freelist.empty())
            {
                new_key = m_freelist.top();
                m_freelist.pop();
                assert(!(*this)[new_key]); // check the id is not in use
                (*this)[new_key] = std::move(x);
            }
            else
            {
                new_key = static_cast<key_type>(base_key + m_objects.size());
                m_objects.push_back(std::move(x));
            }
            return iterator{this, new_key};
        }
        // template <class U = T, class ... Args>
        // constexpr iterator emplace(Args && ... args)
        // {
        //     key_type new_key;

        //     if (!m_freelist.empty())
        //     {
        //         new_key = m_freelist.top();
        //         m_freelist.pop();
        //         assert(!m_objects[new_key]); // check the id is not in use
        //         m_objects[new_key] = std::make_unique<U>(std::forward<Args>(args)...);
        //     }
        //     else
        //     {
        //         new_key = m_objects.size();
        //         m_objects.emplace_back(std::make_unique<U>(std::forward<Args>(args)...));
        //     }
        //     return iterator{this, new_key};
        // }
        constexpr void erase(key_type key)
        {
            operator[](key) = nullptr;
            m_freelist.push(key);
        }
        // swap

        // find
        // contains
        
        // begin
        // cbegin
        // end
        // cend
        // rbegin
        // crbegin
        // rend
        // crend


    private: // checks
        constexpr bool key_bounds_check(key_type key)
        {
            auto key_begin = base_key;
            auto key_end = base_key + m_objects.size();

            return (key >= key_begin) && (key < key_end);
        }
        constexpr bool is_object_present(key_type key)
        {
            // contextual conversion from pointer to bool
            return (operator[](key));
        }


    private:
        clearable_stack<key_type, std::vector<key_type>> m_freelist{};
        // std::stack<key_type, std::vector<key_type>> m_freelist;
        underlying_t m_objects{};
    };

}
