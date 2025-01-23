#pragma once

#include <algorithm>
#include <ranges>
#include <unordered_set>
#include <vector>
#include <deque>
#include <span>

#include "common.h"

namespace algo
{

    class Algo
    {
    public:
        using Indexes = std::unordered_set<size_t>;
        using IndexesGroup = std::vector<Indexes>;

        using RectContext = std::tuple<size_t, _Rect<float>, float>;

        static void _report_pair(size_t i, size_t j, IndexesGroup &index_groups)
        {
            index_groups[i].insert(j);
            index_groups[j].insert(i);
        }

        static void _stab(std::vector<RectContext> &S1, std::vector<RectContext> &S2, IndexesGroup &index_groups)
        {
            // '''Check interval intersection in y-direction.

            // Procedure ``stab(A, B)``::
            //     i := 1; j := 1
            //     while i ≤ |A| and j ≤ |B|
            //         if ai.y0 < bj.y0 then
            //         k := j
            //         while k ≤ |B| and bk.y0 < ai.y1
            //             reportPair(air, bks)
            //             k := k + 1
            //         i := i + 1
            //         else
            //         k := i
            //         while k ≤ |A| and ak.y0 < bj.y1
            //             reportPair(bjs, akr)
            //             k := k + 1
            //         j := j + 1
            // '''
            if (S1.empty() || S2.empty())
            {
                return;
            }

            // sort
            std::ranges::sort(S1, [](const RectContext &lhs, const RectContext &rhs)
                               { return std::get<1>(lhs)[1] < std::get<1>(rhs)[1]; });
            std::ranges::sort(S2, [](const RectContext &lhs, const RectContext &rhs)
                               { return std::get<1>(lhs)[1] < std::get<1>(rhs)[1]; });

            size_t i = 0, j = 0;
            while (i < S1.size() && j < S2.size())
            {
                const auto &[m, a, _] = S1[i];
                const auto &[n, b, _] = S2[j];
                if (a[1] < b[1])
                {
                    size_t k = j;
                    while (k < S2.size() && std::get<1>(S2[k])[1] < a[3])
                    {
                        _report_pair(int(m / 2), int(std::get<0>(S2[k]) / 2), index_groups);
                        k += 1;
                    }
                    i += 1;
                }
                else
                {
                    size_t k = i;
                    while (k < S1.size() && std::get<1>(S1[k])[1] < b[3])
                    {
                        _report_pair(int(std::get<0>(S1[k]) / 2), int(n / 2), index_groups);
                        k += 1;
                    }
                    j += 1;
                }
            }
        }

        static void solve_rects_intersection(const std::span<RectContext> &V, size_t num, IndexesGroup &index_groups)
        {
            // '''Implementation of solving Rectangle-Intersection Problem.

            // Performance::

            //     O(nlog n + k) time and O(n) space, where k is the count of intersection pairs.

            // Args:
            //     V (list): Rectangle-related x-edges data, [(index, Rect, x), (...), ...].
            //     num (int): Count of V instances, equal to len(V).
            //     index_groups (list): Target adjacent list for connectivity between rects.

            // Procedure ``detect(V, H, m)``::

            //     if m < 2 then return else
            //     - let V1 be the first ⌊m/2⌋ and let V2 be the rest of the vertical edges in V in the sorted order;
            //     - let S11 and S22 be the set of rectangles represented only in V1 and V2 but not spanning V2 and V1, respectively;
            //     - let S12 be the set of rectangles represented only in V1 and spanning V2;
            //     - let S21 be the set of rectangles represented only in V2 and spanning V1
            //     - let H1 and H2 be the list of y-intervals corresponding to the elements of V1 and V2 respectively
            //     - stab(S12, S22); stab(S21, S11); stab(S12, S21)
            //     - detect(V1, H1, ⌊m/2⌋); detect(V2, H2, m − ⌊m/2⌋)
            // '''
            if (num < 2)
            {
                return;
            }

            // start/end points of left/right intervals
            const auto center_pos = num / 2;
            const auto X0 = std::get<2>(V.front());
            const auto X = std::get<2>(V[center_pos - 1]);
            const auto X1 = std::get<2>(V.back());

            // split into two groups
            auto left = V.subspan(0, center_pos);
            auto right = V.subspan(center_pos, num - center_pos);

            // filter rects according to their position to each intervals
            auto S11 = std::ranges::filter_view(left, [&](const auto &item)
                                                { return std::get<2>(item) <= X; }) |
                       std::ranges::to<std::vector>();
            auto S12 = std::ranges::filter_view(left, [&](const auto &item)
                                                { return std::get<2>(item) >= X1; }) |
                       std::ranges::to<std::vector>();
            auto S22 = std::ranges::filter_view(right, [&](const auto &item)
                                                { return std::get<2>(item) > X; }) |
                       std::ranges::to<std::vector>();
            auto S21 = std::ranges::filter_view(right, [&](const auto &item)
                                                { return std::get<2>(item) <= X0; }) |
                       std::ranges::to<std::vector>();

            // intersection in x-direction is fulfilled, so check y-direction further
            _stab(S12, S22, index_groups);
            _stab(S21, S11, index_groups);
            _stab(S12, S21, index_groups);

            // recursive process
            solve_rects_intersection(left, center_pos, index_groups);
            solve_rects_intersection(right, num - center_pos, index_groups);
        }

        static std::vector<_Rects<float>> group_by_connectivity(const _Rects<float> &segs, float dx = 0, float dy = 0)
        {
            const auto size = segs.size();
            IndexesGroup index_groups;
            index_groups.reserve(size);

            std::vector<std::tuple<size_t, _Rect<float>, float>> i_rect_x;
            i_rect_x.reserve(size * 2);

            size_t i = 0;

            _Rect<float> d_rect{-dx, -dy, dx, dy};

            for (const auto &seg : segs)
            {
                _Rect<float> points = {seg.x0 + d_rect.x0, seg.y0 + d_rect.y0, seg.x1 + d_rect.x1, seg.y1 + d_rect.y1};
                i_rect_x.push_back(std::make_tuple(i, points, seg.x0));
                i_rect_x.push_back(std::make_tuple(i + 1, points, seg.x1));

                i += 2;
            }

            std::ranges::sort(i_rect_x, [&](const auto &lhs, const auto &rhs)
                              { return std::get<2>(lhs) < std::get<2>(rhs); });

            solve_rects_intersection(i_rect_x, 2 * size, index_groups);

            const auto groups = graph_bfs(index_groups);

            std::vector<_Rects<float>> rects_groups;

            for (const auto &group : groups)
            {
                auto &rects = rects_groups.emplace_back();
                for (const auto &index : group)
                {
                    rects.push_back(segs[index]);
                }
            }

            return rects_groups;
        }

        static IndexesGroup graph_bfs(const IndexesGroup &graph)
        {
            // '''Breadth First Search graph (may be disconnected graph).

            // Args:
            //     graph (list): GRAPH represented by adjacent list, [set(1,2,3), set(...), ...]

            // Returns:
            //     list: A list of connected components
            // '''
            // search graph
            // NOTE: generally a disconnected graph
            std::unordered_set<size_t> counted_indexes;
            IndexesGroup groups;
            for (size_t i = 0; i < graph.size(); ++i)
            {
                if (counted_indexes.find(i) != counted_indexes.end())
                {
                    continue;
                }

                // connected component starts...
                auto indexes = _graph_bfs_from_node(graph, i);
                groups.push_back(indexes);
                counted_indexes.insert(indexes.begin(), indexes.end());
            }

            return groups;
        }

        static Indexes _graph_bfs_from_node(const IndexesGroup &graph, size_t start)
        {
            // '''Breadth First Search connected graph with start node.
            // Args:
            //     graph (list): GRAPH represented by adjacent list, [set(1,2,3), set(...), ...].
            //     start (int): Index of any start vertex.
            // '''
            std::deque<size_t> search_queue;
            std::unordered_set<size_t> searched;
            Indexes indexes;

            search_queue.push_back(start);
            while (!search_queue.empty())
            {
                auto cur_node = search_queue.front();
                search_queue.pop_front();

                if (searched.find(cur_node) != searched.end())
                {
                    continue;
                }

                indexes.insert(cur_node);

                searched.insert(cur_node);
                for (auto node : graph.at(cur_node))
                {
                    search_queue.push_back(node);
                }
            }

            return indexes;
        }
    };

}