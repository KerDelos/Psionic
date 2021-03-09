#pragma once

#include <cstdint>
#include <functional>

struct PSVector2i
{
    int x;
    int y;

    PSVector2i(int p_x, int p_y):x(p_x),y(p_y){}
    PSVector2i():x(0),y(0){}

    bool operator==(const PSVector2i& other) const {
        return x == other.x && y == other.y;
    }
};

namespace std {
    template<> struct hash<PSVector2i>
    {
        uint64_t operator()(const PSVector2i& p) const noexcept
        {
            return ((uint64_t)p.x<<32) | (uint64_t)p.y;
        }
    };
}
