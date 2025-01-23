#pragma once

#include <array>
#include <string>
#include <format>
#include <stdexcept>
#include <cmath>
#include <vector>

#include <opencv2/opencv.hpp>

template <typename T>
struct _Point
{
    T x{};
    T y{};
};
using Pointi = _Point<int>;
using Pointi8 = _Point<int8_t>;
using Pointi16 = _Point<int16_t>;
using Pointi32 = _Point<int32_t>;
using Pointi64 = _Point<int64_t>;
using Pointu8 = _Point<uint8_t>;
using Pointu16 = _Point<uint16_t>;
using Pointu32 = _Point<uint32_t>;
using Pointu64 = _Point<uint64_t>;
using Pointf32 = _Point<float>;
using Pointf64 = _Point<double>;
using Point = Pointi;

template <typename T>
using _Points = std::vector<_Point<T>>;
using Pointsi8 = _Points<int8_t>;
using Pointsi16 = _Points<int16_t>;
using Pointsi32 = _Points<int32_t>;
using Pointsi64 = _Points<int64_t>;
using Pointsu8 = _Points<uint8_t>;
using Pointsu16 = _Points<uint16_t>;
using Pointsu32 = _Points<uint32_t>;
using Pointsu64 = _Points<uint64_t>;
using Pointsf32 = _Points<float>;
using Pointsf64 = _Points<double>;
using Points = _Points<int>;

template <typename T>
struct _Rect
{
    T x0{};
    T y0{};
    T x1{};
    T y1{};

    bool is_horizontal_line() const
    {
        return y0 == y1;
    }

    bool is_vertical_line() const
    {
        return x0 == x1;
    }

    bool is_horizontal() const
    {
        return abs(x1 - x0) > abs(y1 - y0);
    }

    bool is_vertical() const
    {
        return abs(y1 - y0) > abs(x1 - x0);
    }

    auto width() const
    {
        return x1 - x0;
    }

    auto height() const
    {
        return y1 - y0;
    }

    bool intersects(const _Rect &other) const
    {
        return (std::max(x0, other.x0) < std::min(x1, other.x1)) && (std::max(y0, other.y0) < std::min(y1, other.y1));
    }

    _Rect intersect(const _Rect &other) const
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

    static constexpr auto minx = std::numeric_limits<T>::min();
    static constexpr auto miny = std::numeric_limits<T>::min();
    static constexpr auto maxx = std::numeric_limits<T>::max();
    static constexpr auto maxy = std::numeric_limits<T>::max();

    // 以指定点为原点，返回自身与指定象限的交集
    _Rect clip(const Point &origin, Quadrant quadrant) const
    {

        switch (quadrant)
        {
        case Q1:
            return *this & _Rect{origin.x, miny, maxx, origin.y};
        case Q2:
            return *this & _Rect{minx, miny, origin.x, origin.y};
        case Q3:
            return *this & _Rect{minx, origin.y, origin.x, maxy};
        case Q4:
            return *this & _Rect{origin.x, origin.y, maxx, maxy};
        default:
            throw std::invalid_argument("Invalid quadrant");
        }
    }

    _Rect clip_right(T minx) const
    {
        return {std::max(minx, x0), y0, x1, y1};
    }

    _Rect clip_left(T maxx) const
    {
        return {x0, y0, std::min(maxx, x1), y1};
    }

    _Rect clip_bottom(T miny) const
    {
        return {x0, std::max(miny, y0), x1, y1};
    }

    _Rect clip_top(T maxy) const
    {
        return {x0, y0, x1, std::min(maxy, y1)};
    }

    _Rect clip_y(T miny, T maxy) const
    {
        return {x0, std::max(miny, y0), x1, std::min(maxy, y1)};
    }

    _Rect clip_x(T minx, T maxx) const
    {
        return {std::max(minx, x0), y0, std::min(maxx, x1), y1};
    }

    // 以指定线为竖线，返回自身与指定象限的交集
    _Rect clip_x(T x, Quadrant quadrant) const
    {
        switch (quadrant)
        {
        case Q14:
            return *this & _Rect{x, miny, maxx, maxy};
        case Q23:
            return *this & _Rect{minx, miny, x, maxy};
        default:
            throw std::invalid_argument("Invalid quadrant");
        }
    }

    // 以指定线为横线，返回自身与指定象限的交集
    _Rect clip_y(T y, Quadrant quadrant) const
    {
        switch (quadrant)
        {
        case Q12:
            return *this & _Rect{minx, miny, maxx, y};
        case Q34:
            return *this & _Rect{minx, y, maxx, maxy};
        default:
            throw std::invalid_argument("Invalid quadrant");
        }
    }

    // 判断是否与另一个矩形相邻
    bool adjacent(const _Rect &other) const
    {
        return x0 == other.x1 || x1 == other.x0 || y0 == other.y1 || y1 == other.y0;
    }

    bool nearby(const _Rect &other, int delta) const
    {
        return (std::max(x0, other.x0) - std::min(x1, other.x1) <= delta) && (std::max(y0, other.y0) - std::min(y1, other.y1) <= delta);
    }

    bool at_left_of(const _Rect &other) const
    {
        return x1 <= other.x0;
    }

    bool at_right_of(const _Rect &other) const
    {
        return x0 >= other.x1;
    }

    bool at_top_of(const _Rect &other) const
    {
        return y1 <= other.y0;
    }

    bool at_bottom_of(const _Rect &other) const
    {
        return y0 >= other.y1;
    }

    bool contains(const _Rect &other) const
    {
        return contains(other.p0()) && contains(other.p1());
    }

    bool contains(const Point &p) const
    {
        return p.x >= x0 && p.x <= x1 && p.y >= y0 && p.y <= y1;
    }

    _Rect move(T other) const
    {
        return {x0 + other, y0 + other, x1 + other, y1 + other};
    }

    _Rect expand(T delta) const
    {
        return {x0 - delta, y0 - delta, x1 + delta, y1 + delta};
    }

    _Rect expand_x(T delta) const
    {
        return {x0 - delta, y0, x1 + delta, y1};
    }

    _Rect expand_y(T delta) const
    {
        return {x0, y0 - delta, x1, y1 + delta};
    }

    _Rect shrink(T delta) const
    {
        return {x0 + delta, y0 + delta, x1 - delta, y1 - delta};
    }

    _Rect shrink_x(T delta) const
    {
        return {x0 + delta, y0, x1 - delta, y1};
    }

    _Rect shrink_y(T delta) const
    {
        return {x0, y0 + delta, x1, y1 - delta};
    }

    bool is_infinite() const
    {
        return x0 == minx && x1 == maxx && y0 == miny && y1 == maxy;
    }

    _Rect operator|(const _Rect &other) const
    {
        if (is_infinite())
        {
            return *this;
        }
        if (other.is_infinite())
        {
            return other;
        }
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

    _Rect &operator|=(const _Rect &other)
    {
        return *this = *this | other;
    }

    _Rect operator|(const Point &p) const
    {
        return {std::min(x0, p.x), std::min(y0, p.y), std::max(x1, p.x), std::max(y1, p.y)};
    }

    _Rect &operator|=(const Point &p)
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

    _Rect operator&(const _Rect &other) const
    {
        if (is_empty() || other.is_empty())
        {
            return {};
        }
        return {std::max(x0, other.x0), std::max(y0, other.y0), std::min(x1, other.x1), std::min(y1, other.y1)};
    }

    _Rect &operator&=(const _Rect &other)
    {
        return *this = *this & other;
    }

    bool is_empty() const
    {
        return x0 >= x1 || y0 >= y1;
    }

    static _Rect from_array(const std::array<int, 4> &arr)
    {
        return {arr[0], arr[1], arr[2], arr[3]};
    }

    constexpr auto to_string() const
    {
        return std::format("_Rect({}, {}, {}, {})", x0, y0, x1, y1);
    }

    std::array<int, 4> to_array() const
    {
        return {x0, y0, x1, y1};
    }

    bool operator==(const _Rect &other) const
    {
        return x0 == other.x0 && y0 == other.y0 && x1 == other.x1 && y1 == other.y1;
    }

    int operator[](int index) const
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

    template <typename C>
    static _Rect from(const _Rect<C> &other)
    {
        return {(T)(other.x0), (T)(other.y0), (T)(other.x1), (T)(other.y1)};
    }

    cv::Rect_<T> to_cv_rect() const
    {
        return {x0, y0, x1 - x0, y1 - y0};
    }
};

using Recti = _Rect<int>;
using Recti8 = _Rect<int8_t>;
using Recti16 = _Rect<int16_t>;
using Recti32 = _Rect<int32_t>;
using Recti64 = _Rect<int64_t>;
using Rectu8 = _Rect<uint8_t>;
using Rectu16 = _Rect<uint16_t>;
using Rectu32 = _Rect<uint32_t>;
using Rectu64 = _Rect<uint64_t>;
using Rectf32 = _Rect<float>;
using Rectf64 = _Rect<double>;
using Rect = Recti;

template <typename T>
using _Rects = std::vector<_Rect<T>>;
using Rectsi8 = _Rects<int8_t>;
using Rectsi16 = _Rects<int16_t>;
using Rectsi32 = _Rects<int32_t>;
using Rectsi64 = _Rects<int64_t>;
using Rectsu8 = _Rects<uint8_t>;
using Rectsu16 = _Rects<uint16_t>;
using Rectsu32 = _Rects<uint32_t>;
using Rectsu64 = _Rects<uint64_t>;
using Rectsf32 = _Rects<float>;
using Rectsf64 = _Rects<double>;
using Rects = _Rects<int>;

namespace std
{
    template <typename T>
    struct hash<_Rect<T>>
    {
        size_t operator()(const _Rect<T> &rect) const
        {
            return std::hash<T>()(rect.x0) ^ std::hash<T>()(rect.y0) ^ std::hash<T>()(rect.x1) ^ std::hash<T>()(rect.y1);
        }
    };
}