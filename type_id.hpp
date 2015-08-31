#pragma once

//
// http://codereview.stackexchange.com/questions/48594/unique-type-id-no-rtti
//
using typeid_t = void const*;

template <typename T>
static typeid_t
    type_id() noexcept
{
    static char const type_id{};

    return &type_id;
}
