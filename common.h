#pragma once

#include <array>
#include <string>
#include <format>
#include <stdexcept>

struct Rect
{
    int left{};
    int top{};
    int right{};
    int bottom{};

    auto width() const
    {
        return right - left;
    }

    auto height() const
    {
        return bottom - top;
    }

    static Rect from_array(const std::array<int, 4> &arr)
    {
        return {arr[0], arr[1], arr[2], arr[3]};
    }

    constexpr auto to_string() const
    {
        return std::format("Rect({}, {}, {}, {})", left, top, right, bottom);
    }

    std::array<int, 4> to_array() const
    {
        return {left, top, right, bottom};
    }

    bool operator==(const Rect &other) const
    {
        return left == other.left && top == other.top && right == other.right && bottom == other.bottom;
    }

    int &operator[](int index)
    {
        switch (index)
        {
        case 0:
            return left;
        case 1:
            return top;
        case 2:
            return right;
        case 3:
            return bottom;
        default:
            throw std::out_of_range("Index out of range");
        }
    }
};

namespace std
{
    template <>
    struct hash<Rect>
    {
        size_t operator()(const Rect &rect) const
        {
            return std::hash<int>()(rect.left) ^ std::hash<int>()(rect.top) ^ std::hash<int>()(rect.right) ^ std::hash<int>()(rect.bottom);
        }
    };
}