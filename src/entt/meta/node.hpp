#ifndef ENTT_META_NODE_HPP
#define ENTT_META_NODE_HPP


#include <cstddef>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/attribute.h"
#include "../core/fwd.hpp"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "type_traits.hpp"


namespace entt {


class meta_any;
class meta_type;
struct meta_handle;


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


struct meta_type_node;


struct meta_prop_node {
    meta_prop_node * next;
    const meta_any &id;
    meta_any &value;
};


struct meta_base_node {
    meta_base_node * next;
    meta_type_node * const type;
    const void *(* const cast)(const void *) ENTT_NOEXCEPT;
};


struct meta_conv_node {
    meta_conv_node * next;
    meta_type_node * const type;
    meta_any(* const conv)(const void *);
};


struct meta_ctor_node {
    using size_type = std::size_t;
    meta_ctor_node * next;
    meta_prop_node * prop;
    const size_type arity;
    meta_type(* const arg)(const size_type) ENTT_NOEXCEPT;
    meta_any(* const invoke)(meta_any * const);
};


struct meta_data_node {
    id_type id;
    meta_data_node * next;
    meta_prop_node * prop;
    const bool is_const;
    const bool is_static;
    meta_type_node * const type;
    bool(* const set)(meta_handle, meta_any);
    meta_any(* const get)(meta_handle);
};


struct meta_func_node {
    using size_type = std::size_t;
    id_type id;
    meta_func_node * next;
    meta_prop_node * prop;
    const size_type arity;
    const bool is_const;
    const bool is_static;
    meta_type_node * const ret;
    meta_type(* const arg)(const size_type) ENTT_NOEXCEPT;
    meta_any(* const invoke)(meta_handle, meta_any * const);
};


struct meta_template_node {
    using size_type = std::size_t;
    const size_type arity;
    meta_type_node * const type;
    meta_type_node *(* const arg)(const size_type) ENTT_NOEXCEPT;
};


struct meta_type_node {
    using size_type = std::size_t;
    const type_info info;
    id_type id;
    meta_type_node * next;
    meta_prop_node * prop;
    const size_type size_of;
    const bool is_integral;
    const bool is_floating_point;
    const bool is_array;
    const bool is_enum;
    const bool is_union;
    const bool is_class;
    const bool is_pointer;
    const bool is_function_pointer;
    const bool is_member_object_pointer;
    const bool is_member_function_pointer;
    const bool is_pointer_like;
    const bool is_sequence_container;
    const bool is_associative_container;
    const size_type rank;
    size_type(* const extent)(const size_type) ENTT_NOEXCEPT ;
    meta_type_node *(* const remove_pointer)() ENTT_NOEXCEPT;
    meta_type_node *(* const remove_extent)() ENTT_NOEXCEPT;
    const meta_template_node *const templ;
    meta_ctor_node * const def_ctor;
    meta_ctor_node *ctor{nullptr};
    meta_base_node *base{nullptr};
    meta_conv_node *conv{nullptr};
    meta_data_node *data{nullptr};
    meta_func_node *func{nullptr};
    void(* dtor)(void *){nullptr};
};


template<typename... Args>
meta_type_node * meta_arg_node(type_list<Args...>, const std::size_t index) ENTT_NOEXCEPT;


template<typename Type>
class ENTT_API meta_node {
    static_assert(std::is_same_v<Type, std::remove_cv_t<std::remove_reference_t<Type>>>, "Invalid type");

    template<std::size_t... Index>
    [[nodiscard]] static auto extent(const meta_type_node::size_type dim, std::index_sequence<Index...>) ENTT_NOEXCEPT {
        meta_type_node::size_type ext{};
        ((ext = (dim == Index ? std::extent_v<Type, Index> : ext)), ...);
        return ext;
    }

    [[nodiscard]] static meta_ctor_node * meta_default_constructor([[maybe_unused]] meta_type_node *type) ENTT_NOEXCEPT {
        if constexpr(std::is_default_constructible_v<Type>) {
            static meta_ctor_node node{
                nullptr,
                nullptr,
                0u,
                nullptr,
                [](meta_any * const) { return meta_any{std::in_place_type<Type>}; }
            };

            return &node;
        } else {
            return nullptr;
        }
    }

    [[nodiscard]] static meta_template_node * meta_template_info() ENTT_NOEXCEPT {
        if constexpr(is_complete_v<meta_template_traits<Type>>) {
            static meta_template_node node{
                meta_template_traits<Type>::args_type::size,
                meta_node<typename meta_template_traits<Type>::class_type>::resolve(),
                [](const std::size_t index) ENTT_NOEXCEPT {
                    return meta_arg_node(typename meta_template_traits<Type>::args_type{}, index);
                }
            };

            return &node;
        } else {
            return nullptr;
        }
    }

public:
    [[nodiscard]] static meta_type_node * resolve() ENTT_NOEXCEPT {
        static meta_type_node node{
            type_id<Type>(),
            {},
            nullptr,
            nullptr,
            size_of_v<Type>,
            std::is_integral_v<Type>,
            std::is_floating_point_v<Type>,
            std::is_array_v<Type>,
            std::is_enum_v<Type>,
            std::is_union_v<Type>,
            std::is_class_v<Type>,
            std::is_pointer_v<Type>,
            std::is_pointer_v<Type> && std::is_function_v<std::remove_pointer_t<Type>>,
            std::is_member_object_pointer_v<Type>,
            std::is_member_function_pointer_v<Type>,
            is_meta_pointer_like_v<Type>,
            is_complete_v<meta_sequence_container_traits<Type>>,
            is_complete_v<meta_associative_container_traits<Type>>,
            std::rank_v<Type>,
            [](meta_type_node::size_type dim) ENTT_NOEXCEPT { return extent(dim, std::make_index_sequence<std::rank_v<Type>>{}); },
            &meta_node<std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<Type>>>>::resolve,
            &meta_node<std::remove_cv_t<std::remove_reference_t<std::remove_extent_t<Type>>>>::resolve,
            meta_template_info(),
            meta_default_constructor(&node),
            meta_default_constructor(&node)
        };

        return &node;
    }
};


template<typename Type>
struct meta_info: meta_node<std::remove_cv_t<std::remove_reference_t<Type>>> {};


template<typename... Args>
meta_type_node * meta_arg_node(type_list<Args...>, const std::size_t index) ENTT_NOEXCEPT {
    meta_type_node *args[sizeof...(Args) + 1u]{nullptr, internal::meta_info<Args>::resolve()...};
    return args[index + 1u];
}


}


/**
 * Internal details not to be documented.
 * @endcond
 */


}


#endif
