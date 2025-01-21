#pragma once

#include <array>
#include <string>
#include <format>
#include <stdexcept>

struct Point
{
    int x{};
    int y{};
};

struct Rect
{
    int x0{};
    int y0{};
    int x1{};
    int y1{};

    auto width() const
    {
        return x1 - x0;
    }

    auto height() const
    {
        return y1 - y0;
    }

    bool intersects(const Rect &other) const
    {
        return !(other.x0 > x1 ||
                 other.x1 < x0 ||
                 other.y0 > y1 ||
                 other.y1 < y0);
    }

    Rect move(const int &other) const
    {
        return {x0 + other, y0 + other, x1 + other, y1 + other};
    }

    Rect expand(const int &delta) const
    {
        return {x0 - delta, y0 - delta, x1 + delta, y1 + delta};
    }

    Rect expand_x(const int &delta) const
    {
        return {x0 - delta, y0, x1 + delta, y1};
    }

    Rect expand_y(const int &delta) const
    {
        return {x0, y0 - delta, x1, y1 + delta};
    }

    Rect shrink(const int &delta) const
    {
        return {x0 + delta, y0 + delta, x1 - delta, y1 - delta};
    }

    Rect shrink_x(const int &delta) const
    {
        return {x0 + delta, y0, x1 - delta, y1};
    }

    Rect shrink_y(const int &delta) const
    {
        return {x0, y0 + delta, x1, y1 - delta};
    }

    Rect operator|(const Rect &other) const
    {
        if (is_empty())
        {
            return other;
        }
        if (other.is_empty())
        {
            return *this;
        }
        return {std::min(x0, other.x0), std::min(y0, other.y0), std::max(x1, other.x1), std::max(y1, other.y1)};
    }

    Rect &operator|=(const Rect &other)
    {
        return *this = *this | other;
    }

    Rect operator|(const Point &p) const
    {
        return {std::min(x0, p.x), std::min(y0, p.y), std::max(x1, p.x), std::max(y1, p.y)};
    }

    Rect &operator|=(const Point &p)
    {
        return *this = *this | p;
    }

    Point p0() const
    {
        return {x0, y0};
    }

    Point p1() const
    {
        return {x1, y1};
    }

    Rect operator&(const Rect &other) const
    {
        if (is_empty() || other.is_empty())
        {
            return {};
        }
        return {std::max(x0, other.x0), std::max(y0, other.y0), std::min(x1, other.x1), std::min(y1, other.y1)};
    }

    Rect &operator&=(const Rect &other)
    {
        return *this = *this & other;
    }

    bool is_empty() const
    {
        return x0 >= x1 || y0 >= y1;
    }

    static Rect from_array(const std::array<int, 4> &arr)
    {
        return {arr[0], arr[1], arr[2], arr[3]};
    }

    constexpr auto to_string() const
    {
        return std::format("Rect({}, {}, {}, {})", x0, y0, x1, y1);
    }

    std::array<int, 4> to_array() const
    {
        return {x0, y0, x1, y1};
    }

    bool operator==(const Rect &other) const
    {
        return x0 == other.x0 && y0 == other.y0 && x1 == other.x1 && y1 == other.y1;
    }

    int &operator[](int index)
    {
        switch (index)
        {
        case 0:
            return x0;
        case 1:
            return y0;
        case 2:
            return x1;
        case 3:
            return y1;
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
            return std::hash<int>()(rect.x0) ^ std::hash<int>()(rect.y0) ^ std::hash<int>()(rect.x1) ^ std::hash<int>()(rect.y1);
        }
    };
}