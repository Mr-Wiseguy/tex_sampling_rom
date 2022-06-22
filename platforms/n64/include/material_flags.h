#ifndef __MATERIAL_FLAGS_H__
#define __MATERIAL_FLAGS_H__

#include <cstdint>
#include <type_traits>

enum class MaterialFlags : uint16_t {
    none                = 0,
    set_rendermode      = 1,
    set_combiner        = 2,
    set_env             = 4,
    set_prim            = 8,
    tex0                = 16,
    tex1                = 32,
    set_geometry_mode   = 64,
    two_cycle           = 128,
    point_filter        = 256,
};

constexpr MaterialFlags operator&(MaterialFlags lhs, MaterialFlags rhs)
{
    return static_cast<MaterialFlags>(
        static_cast<std::underlying_type_t<MaterialFlags>>(lhs) &
        static_cast<std::underlying_type_t<MaterialFlags>>(rhs));
}

constexpr MaterialFlags operator|(MaterialFlags lhs, MaterialFlags rhs)
{
    return static_cast<MaterialFlags>(
        static_cast<std::underlying_type_t<MaterialFlags>>(lhs) |
        static_cast<std::underlying_type_t<MaterialFlags>>(rhs));
}

constexpr MaterialFlags operator^(MaterialFlags lhs, MaterialFlags rhs)
{
    return static_cast<MaterialFlags>(
        static_cast<std::underlying_type_t<MaterialFlags>>(lhs) ^
        static_cast<std::underlying_type_t<MaterialFlags>>(rhs));
}

constexpr MaterialFlags operator~(MaterialFlags x)
{
    return static_cast<MaterialFlags>(
        ~static_cast<std::underlying_type_t<MaterialFlags>>(x));
}

constexpr MaterialFlags& operator&=(MaterialFlags& lhs, MaterialFlags rhs)
{
    lhs = lhs & rhs;
    return lhs;
}

constexpr MaterialFlags& operator|=(MaterialFlags& lhs, MaterialFlags rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

constexpr MaterialFlags& operator^=(MaterialFlags& lhs, MaterialFlags rhs)
{
    lhs = lhs ^ rhs;
    return lhs;
}

#endif
