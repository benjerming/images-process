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
        return !(x0 >= other.x1 || x1 <= other.x0 || y0 >= other.y1 || y1 <= other.y0);
    }

    Rect intersect(const Rect &other) const
    {
        return *this & other;
    }

    enum Quadrant
    {
        Q1 = 1,
        Q2 = 2,
        Q3 = 4,
        Q4 = 8,
        Q12 = Q1 | Q2,
        Q34 = Q3 | Q4,
        Q14 = Q1 | Q4,
        Q23 = Q2 | Q3,
        TopRight = Q1,
        TopLeft = Q2,
        BottomLeft = Q3,
        BottomRight = Q4,
        Top = Q12,
        Bottom = Q34,
        Left = Q23,
        Right = Q14,
    };

    static constexpr auto minx = std::numeric_limits<decltype(x0)>::min();
    static constexpr auto miny = std::numeric_limits<decltype(y0)>::min();
    static constexpr auto maxx = std::numeric_limits<decltype(x1)>::max();
    static constexpr auto maxy = std::numeric_limits<decltype(y1)>::max();

    // 以指定点为原点，返回自身与指定象限的交集
    Rect clip(const Point &origin, Quadrant quadrant) const
    {

        switch (quadrant)
        {
        case Q1:
            return *this & Rect{origin.x, miny, maxx, origin.y};
        case Q2:
            return *this & Rect{minx, miny, origin.x, origin.y};
        case Q3:
            return *this & Rect{minx, origin.y, origin.x, maxy};
        case Q4:
            return *this & Rect{origin.x, origin.y, maxx, maxy};
        default:
            throw std::invalid_argument("Invalid quadrant");
        }
    }

    Rect clip_right(decltype(x0) minx) const
    {
        return {std::max(minx, x0), y0, x1, y1};
    }

    Rect clip_left(decltype(x1) maxx) const
    {
        return {x0, y0, std::min(maxx, x1), y1};
    }

    Rect clip_bottom(decltype(y0) miny) const
    {
        return {x0, std::max(miny, y0), x1, y1};
    }

    Rect clip_top(decltype(y1) maxy) const
    {
        return {x0, y0, x1, std::min(maxy, y1)};
    }

    Rect clip_y(decltype(y0) miny, decltype(y1) maxy) const
    {
        return {x0, std::max(miny, y0), x1, std::min(maxy, y1)};
    }

    Rect clip_x(decltype(x0) minx, decltype(x1) maxx) const
    {
        return {std::max(minx, x0), y0, std::min(maxx, x1), y1};
    }

    // 以指定线为竖线，返回自身与指定象限的交集
    Rect clip_x(decltype(x0) x, Quadrant quadrant) const
    {
        switch (quadrant)
        {
        case Q14:
            return *this & Rect{x, miny, maxx, maxy};
        case Q23:
            return *this & Rect{minx, miny, x, maxy};
        default:
            throw std::invalid_argument("Invalid quadrant");
        }
    }

    // 以指定线为横线，返回自身与指定象限的交集
    Rect clip_y(decltype(y0) y, Quadrant quadrant) const
    {
        switch (quadrant)
        {
        case Q12:
            return *this & Rect{minx, miny, maxx, y};
        case Q34:
            return *this & Rect{minx, y, maxx, maxy};
        default:
            throw std::invalid_argument("Invalid quadrant");
        }
    }

    // 判断是否与另一个矩形相邻
    bool adjacent(const Rect &other) const
    {
        return x0 == other.x1 || x1 == other.x0 || y0 == other.y1 || y1 == other.y0;
    }

    bool nearby(const Rect &other, int delta) const
    {
        return !(x0 >= other.x1 + delta || x1 <= other.x0 - delta || y0 >= other.y1 + delta || y1 <= other.y0 - delta);
    }

    bool at_left_of(const Rect &other) const
    {
        return x1 <= other.x0;
    }

    bool at_right_of(const Rect &other) const
    {
        return x0 >= other.x1;
    }

    bool at_top_of(const Rect &other) const
    {
        return y1 <= other.y0;
    }

    bool at_bottom_of(const Rect &other) const
    {
        return y0 >= other.y1;
    }

    bool contains(const Rect &other) const
    {
        return contains(other.p0()) && contains(other.p1());
    }

    bool contains(const Point &p) const
    {
        return p.x >= x0 && p.x <= x1 && p.y >= y0 && p.y <= y1;
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